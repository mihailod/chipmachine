// Feasibility spike: load a .gsf/.minigsf and render audio through mGBA.
// GSF loader written from the public PSF spec (Neill Corlett's psf_format.txt):
//   magic "PSF" + version byte (0x22 for GSF)
//   u32 reserved_size, u32 program_size, u32 program_crc
//   reserved area, zlib-deflated program, optional "[TAG]" block
// The GSF program section itself is: u32 entry, u32 load_offset, u32 size, data.
// _lib chains load first and are overlaid by the referencing file.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>
#include <mgba/core/core.h>
#include <mgba/gba/core.h>
#include <mgba-util/vfs.h>
#include <mgba-util/audio-buffer.h>
#include <mgba/core/config.h>
#include <mgba/core/log.h>
// mGBA calls through its default logger during core->init(); leaving it unset
// is an immediate wild jump, not a silent no-op. A sink that drops everything
// is enough for a headless decoder.
static void _quietLog(struct mLogger* l, int c, enum mLogLevel lv, const char* f, va_list a) {
    (void)l; (void)c; (void)lv; (void)f; (void)a;
}
static struct mLogger gQuietLogger = { .log = _quietLog };
#define CK(x) do{ fprintf(stderr,"  [ok] %s\n",x); fflush(stderr);}while(0)

#define ROM_MAX (32u * 1024 * 1024)
static unsigned char rom[ROM_MAX];
static size_t romHigh = 0;
static int romLoaded = 0;

static unsigned rd32(const unsigned char* p) {
    return p[0] | (p[1] << 8) | (p[2] << 16) | ((unsigned)p[3] << 24);
}

// Returns malloc'd tag block (NUL-terminated) or NULL; fills *exe/*exeLen.
static int loadPsf(const char* path, int depth, char** tagsOut);

static void applyExe(const unsigned char* exe, size_t n) {
    if (n < 12) return;
    unsigned load = rd32(exe + 4) & 0x01FFFFFFu;   // strip 0x08000000 cart base
    unsigned sz   = rd32(exe + 8);
    if (sz > n - 12) sz = (unsigned)(n - 12);
    if (load + sz > ROM_MAX) return;
    memcpy(rom + load, exe + 12, sz);
    if (load + sz > romHigh) romHigh = load + sz;
    romLoaded = 1;
}

static char* findTag(const char* tags, const char* key) {
    if (!tags) return NULL;
    size_t kl = strlen(key);
    const char* p = tags;
    while (*p) {
        const char* eol = strchr(p, '\n'); if (!eol) eol = p + strlen(p);
        if (!strncasecmp(p, key, kl) && p[kl] == '=') {
            size_t vl = (size_t)(eol - (p + kl + 1));
            char* v = malloc(vl + 1); memcpy(v, p + kl + 1, vl); v[vl] = 0; return v;
        }
        p = (*eol) ? eol + 1 : eol;
    }
    return NULL;
}

static int loadPsf(const char* path, int depth, char** tagsOut) {
    if (depth > 10) return 0;
    FILE* f = fopen(path, "rb"); if (!f) return 0;
    fseek(f, 0, SEEK_END); long fsz = ftell(f); fseek(f, 0, SEEK_SET);
    unsigned char* buf = malloc((size_t)fsz);
    if (fread(buf, 1, (size_t)fsz, f) != (size_t)fsz) { fclose(f); free(buf); return 0; }
    fclose(f);
    if (fsz < 16 || memcmp(buf, "PSF", 3) != 0) { free(buf); return 0; }
    unsigned resSz = rd32(buf + 4), progSz = rd32(buf + 8);
    size_t tagOff = 16 + resSz + progSz;
    char* tags = NULL;
    if (tagOff + 5 <= (size_t)fsz && memcmp(buf + tagOff, "[TAG]", 5) == 0) {
        size_t tl = (size_t)fsz - tagOff - 5;
        tags = malloc(tl + 1); memcpy(tags, buf + tagOff + 5, tl); tags[tl] = 0;
    }
    // _lib chain FIRST (spec: libs load before the referencing file's own data)
    if (tags) {
        char key[16];
        for (int i = 1; i <= 9; i++) {
            if (i == 1) strcpy(key, "_lib"); else sprintf(key, "_lib%d", i);
            char* lib = findTag(tags, key);
            if (!lib) { if (i > 1) break; else continue; }
            char dir[2048]; snprintf(dir, sizeof dir, "%s", path);
            char* slash = strrchr(dir, '/'); if (slash) *(slash + 1) = 0; else dir[0] = 0;
            char full[4096]; snprintf(full, sizeof full, "%s%s", dir, lib);
            loadPsf(full, depth + 1, NULL);
            free(lib);
        }
    }
    // inflate the program section and overlay it
    if (progSz) {
        uLongf out = ROM_MAX; unsigned char* exe = malloc(out);
        if (uncompress(exe, &out, buf + 16 + resSz, progSz) == Z_OK) applyExe(exe, out);
        free(exe);
    }
    if (tagsOut) *tagsOut = tags; else free(tags);
    free(buf);
    return 1;
}

int main(int argc, char** argv) {
    mLogSetDefaultLogger(&gQuietLogger);
    for (int a = 1; a < argc; a++) {
        CK("main-enter"); memset(rom, 0, ROM_MAX); romHigh = 0; romLoaded = 0; CK("memset");
        char* tags = NULL;
        if (!loadPsf(argv[a], 0, &tags) || !romLoaded) { printf("%s\tLOADFAIL\n", argv[a]); continue; }
        CK("psf-loaded"); fprintf(stderr,"      romHigh=%zu\n",romHigh);
        size_t romSize = romHigh < 0x1000 ? 0x1000 : romHigh;

        struct mCore* core = GBACoreCreate(); CK("corecreate");
        if (!core) { printf("%s\tNOCORE\n", argv[a]); continue; }
        fprintf(stderr,"      core=%p init=%p reset=%p sizeof(mCore)=%zu\n",(void*)core,(void*)core->init,(void*)core->reset,sizeof(struct mCore));
        core->init(core);
        CK("init");
        // mGBA renders video unconditionally; without a buffer the scanline
        // writes fault. Allocate one and throw it away -- we only want audio.
        unsigned vw, vh;
        core->baseVideoSize(core, &vw, &vh);
        mColor* vbuf = calloc((size_t)vw * vh, 4);
        core->setVideoBuffer(core, vbuf, vw);
        CK("videobuf");
        mCoreConfigInit(&core->config, NULL);
        core->setAudioBufferSize(core, 4096);
        CK("audiobuf");
        struct VFile* vf = VFileFromConstMemory(rom, romSize);
        CK("preloadROM"); if (!core->loadROM(core, vf)) { printf("%s\tROMFAIL\n", argv[a]); mCoreConfigDeinit(&core->config); core->deinit(core); free(vbuf); continue; }
        mCoreLoadConfig(core);
        CK("loadconfig");
        core->reset(core);
        CK("reset");

        unsigned rate = core->audioSampleRate(core);
        struct mAudioBuffer* ab = core->getAudioBuffer(core);
        int16_t chunk[4096 * 2];
        double sumSq = 0; long total = 0; long active = 0, blocks = 0;
        int seconds = 8;
        while (total < (long)rate * seconds) {
            core->runFrame(core); if(total==0) CK("frame1");
            size_t got;
            while ((got = mAudioBufferRead(ab, chunk, 2048)) > 0) {
                double bs = 0;
                for (size_t i = 0; i < got * 2; i++) { double v = chunk[i]; sumSq += v * v; bs += v * v; }
                if (got) { blocks++; if (bs / (got * 2) > 64.0) active++; }
                total += (long)got;
            }
        }
        double rms = total ? sqrt(sumSq / (total * 2)) : 0;
        char* title = findTag(tags, "title"); char* game = findTag(tags, "game");
        printf("%s\tOK\trate=%u\trms=%.1f\tactive=%ld/%ld\t%s\t%s\n", argv[a], rate, rms,
               active, blocks, title ? title : "", game ? game : "");
        free(title); free(game); free(tags);
        mCoreConfigDeinit(&core->config);
        core->deinit(core);
        free(vbuf);
    }
    return 0;
}
