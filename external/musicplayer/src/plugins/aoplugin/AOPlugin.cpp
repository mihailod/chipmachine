
#include "AOPlugin.h"
#include "../../chipplayer.h"

#include <coreutils/split.h>
#include <coreutils/utils.h>
#include <psf/PSFFile.h>

#include <coreutils/file.h>
#include <array>
#include <cstring>
#include <set>
#include <string>
#include <vector>

extern "C"
{
#include "ao.h"
    int32 ssf_start(uint8*, uint32 length);
    int32 ssf_gen(int16*, uint32);
    int32 ssf_stop(void);
    int32 ssf_command(int32, int32);
    int32 ssf_fill_info(ao_display_info*);

    int32 qsf_start(uint8*, uint32 length);
    int32 qsf_gen(int16*, uint32);
    int32 qsf_stop(void);
    int32 qsf_command(int32, int32);
    int32 qsf_fill_info(ao_display_info*);

    int32 spu_start(uint8*, uint32 length);
    int32 spu_gen(int16*, uint32);
    int32 spu_stop(void);
    int32 spu_command(int32, int32);
    int32 spu_fill_info(ao_display_info*);

    /* PlayStation 1/2. These two engines were always compiled into this plugin
       (aosdk/eng_psf/{eng_psf.c,eng_psf2.c,psx.c,psx_hw.c}) but nothing ever
       called them, because supported_ext did not list the PSF extensions.
       They are the BIOS-FREE path: psx_hw.c emulates the PS1 BIOS A0/B0/C0
       vectors and the PS2 IOP kernel in software (HLE), so unlike Highly
       Experimental it needs no Sony BIOS image on disk. */
    int32 psf_start(uint8*, uint32 length);
    int32 psf_gen(int16*, uint32);
    int32 psf_stop(void);
    int32 psf_command(int32, int32);
    int32 psf_fill_info(ao_display_info*);

    int32 psf2_start(uint8*, uint32 length);
    int32 psf2_gen(int16*, uint32);
    int32 psf2_stop(void);
    int32 psf2_command(int32, int32);
    int32 psf2_fill_info(ao_display_info*);

    uint8 qsf_memory_read(uint16 addr);
    uint8 qsf_memory_readop(uint16 addr);
    uint8 qsf_memory_readport(uint16 addr);
    void qsf_memory_write(uint16 addr, uint8 byte);
    void qsf_memory_writeport(uint16 addr, uint8 byte);

    /* redirect stubs to interface the Z80 core to the QSF engine */
    uint8 memory_read(uint16 addr) { return qsf_memory_read(addr); }

    uint8 memory_readop(uint16 addr) { return memory_read(addr); }

    uint8 memory_readport(uint16 addr) { return qsf_memory_readport(addr); }

    void memory_write(uint16 addr, uint8 byte) { qsf_memory_write(addr, byte); }

    void memory_writeport(uint16 addr, uint8 byte)
    {
        qsf_memory_writeport(addr, byte);
    }

    static std::string baseDir;

    /* ao_get_lib: called to load secondary files */
    int ao_get_lib(char* filename, uint8** buffer, uint64* length)
    {
        uint8* filebuf = nullptr;
        uint32 size = 0;
        FILE* auxfile = nullptr;

        // Try the "_lib" tag exactly as written first, and only then the
        // lower-cased form this always used. The tag routinely carries mixed
        // case ("Pop'n Taisen Puzzle-dama Online.psf2lib", "W00.ssflib") and
        // that is the name getSecondaryFiles/psfLibFiles downloads it under, so
        // lower-casing unconditionally only ever worked because the local cache
        // happens to sit on a case-insensitive volume.
        auto openIn = [&](std::string const& leaf) -> FILE* {
            std::string full = baseDir.empty() ? leaf : baseDir + "/" + leaf;
            return fopen(full.c_str(), "rb");
        };
        std::string fullName = filename;

        auxfile = openIn(filename);
        if (auxfile == nullptr) {
            fullName = utils::toLower(filename);
            auxfile = openIn(fullName);
        }
        if (auxfile == nullptr) {
            // stderr, NOT stdout: `cm --dump-metadata` writes its TSV to
            // stdout, and these lines interleaved straight into it, corrupting
            // the rows of exactly the files that failed.
            fprintf(stderr, "Unable to find auxiliary file %s\n",
                    fullName.c_str());
            return AO_FAIL;
        }

        fseek(auxfile, 0, SEEK_END);
        size = ftell(auxfile);
        fseek(auxfile, 0, SEEK_SET);

        filebuf = static_cast<uint8*>(malloc(size));

        if (filebuf == nullptr) {
            fclose(auxfile);
            fprintf(stderr, "ERROR: could not allocate %d bytes\n", size);
            return AO_FAIL;
        }

        auto rc = fread(filebuf, size, 1, auxfile);
        fclose(auxfile);

        *buffer = filebuf;
        *length = (uint64)size;

        return rc > 0 ? AO_SUCCESS : AO_FAIL;
    }
}

enum
{
    SIG_QSF = 0x50534641,
    SIG_SSF = 0x50534611,
    SIG_SPU = 0x53505500,
    SIG_PSF = 0x50534601,
    SIG_PSF2 = 0x50534602,
    SIG_DSF = 0x50534612
};

namespace musix {

class AOPlayer : public ChipPlayer
{
public:
    explicit AOPlayer(const std::string& fileName)
    {

        baseDir = utils::path_directory(fileName);

        // MUST outlive the constructor. corlett_decode() does not copy the
        // "reserved" section -- it sets corlett_t::res_section to a pointer
        // INTO this buffer -- and eng_psf2 keeps that pointer in filesys[0] and
        // reads through it for the whole song, every time the emulated IOP
        // opens a file. As a constructor local it dangled the moment the player
        // was handed to the caller. It happened to survive for full ".psf2"
        // (a large, immediately-reused-by-nobody allocation) and reliably did
        // NOT for ".minipsf2", whose own filesystem is ~90 bytes: every lookup
        // in it missed, "psf2.ini" was never found, the driver never learned
        // which sequence to play, and the tune rendered pure silence.
        buffer = utils::File(fileName).readAll();
        if (buffer.size() < 4) { throw player_exception(); }

        filesig =
            buffer[0] << 24 | buffer[1] << 16 | buffer[2] << 8 | buffer[3];
        // ".spu" is a raw SPU RAM + register dump and its fourth magic byte is
        // a format revision, not a zero: SIG_SPU spells "SPU\0" but the rips in
        // the wild say "SPU1". So the switch below never matched one, no engine
        // ran, and getSamples() handed back whatever was already in the
        // caller's buffer -- which the fixture test read as "playback OK"
        // because uninitialised memory is rarely all zeroes. eng_spu.c itself
        // only ever compares three bytes (see spu_start), so match three.
        if (memcmp(&buffer[0], "SPU", 3) == 0) { filesig = SIG_SPU; }
        ao_display_info info;
        int rc = 0;
        std::string format;
        switch (filesig) {
        case SIG_SSF:
            if (ssf_start(&buffer[0], buffer.size()) != AO_SUCCESS) {
                throw player_exception();
            }
            rc = ssf_fill_info(&info);
            format = "Sega Saturn";
            break;
        case SIG_SPU:
            if (spu_start(&buffer[0], buffer.size()) != AO_SUCCESS) {
                throw player_exception();
            }
            rc = spu_fill_info(&info);
            format = "Sony Playstation";
            break;
        case SIG_QSF:
            if (qsf_start(&buffer[0], buffer.size()) != AO_SUCCESS) {
                throw player_exception();
            }
            rc = qsf_fill_info(&info);
            format = "Capcom QSound";
            break;
        case SIG_PSF:
            if (psf_start(&buffer[0], buffer.size()) != AO_SUCCESS) {
                throw player_exception();
            }
            rc = psf_fill_info(&info);
            format = "Playstation";
            break;
        case SIG_PSF2:
            if (psf2_start(&buffer[0], buffer.size()) != AO_SUCCESS) {
                throw player_exception();
            }
            rc = psf2_fill_info(&info);
            format = "Playstation2";
            break;
        }

        if (filesig == SIG_PSF || filesig == SIG_PSF2) {
            // Take PlayStation metadata from PSFFile, exactly as HEPlugin did
            // before these extensions moved here, so the catalog sees the same
            // strings and the same length as it always has. AOSDK's own
            // fill_info would regress it: psf_fill_info reports the raw
            // "Length:" tag and the parser below only understands "m:ss", so
            // every rip that writes plain seconds ("16") or "m:ss.fff" came out
            // as length 0 -- measured on 5 of 15 sampled modland rips.
            bool tagged = false;
            try {
                PSFFile psf{ fileName };
                if (psf.valid()) {
                    auto& tags = psf.tags();
                    // songLength() runs stod() over a tag an arbitrary ripper
                    // wrote, so it can throw -- do not let a bad "Length:"
                    // string take down a tune that plays perfectly well.
                    setMeta("composer", tags["artist"], "sub_title",
                            tags["title"], "game", tags["game"], "format",
                            format, "length",
                            static_cast<int>(psf.songLength()));
                    tagged = true;
                }
            } catch (std::exception const&) {
            }
            if (!tagged) { setMeta("format", format); }
        } else if (rc == AO_SUCCESS) {
            std::string title = info.info[1];
            std::string composer = info.info[3];
            int len = 0;
            auto p = utils::split(std::string(info.info[6]), ":");
            if (p.size() == 2) {
                len = std::stol(p[0]) * 60 + std::stol(p[1]);
            }
            setMeta("sub_title", title, "composer", composer, "format", format,
                    "length", len);
        }
    }
    ~AOPlayer() override
    {
        // Was an unconditional ssf_stop() -- which tore down the Saturn SCSP no
        // matter what had actually been started, leaking the other engines'
        // state (and, for PSF2, its lib buffer). Stop the one we opened.
        switch (filesig) {
        case SIG_SSF: ssf_stop(); break;
        case SIG_SPU: spu_stop(); break;
        case SIG_QSF: qsf_stop(); break;
        case SIG_PSF: psf_stop(); break;
        case SIG_PSF2: psf2_stop(); break;
        default: break;
        }
    }

    int getSamples(int16_t* target, int noSamples) override
    {

        if (filesig == SIG_PSF || filesig == SIG_PSF2) {
            return genPSX(target, noSamples);
        }

        int rc = 0;
        int t = noSamples / 2;
        while (t > 0) {
            int n = 1024;
            if (t < n) {
                n = t;
            }
            switch (filesig) {
            case SIG_SSF:
                rc = ssf_gen(target, n);
                break;
            case SIG_SPU:
                rc = spu_gen(target, n);
                break;
            case SIG_QSF:
                rc = qsf_gen(target, n);
                break;
            default:
                // No engine for this signature: render silence rather than
                // handing back whatever was already in the caller's buffer,
                // which in the live app is the audio fifo's scratch buffer --
                // i.e. the previous song's tail, played on loop. This is how
                // every ".spu" behaved until the 3-byte magic fix above, and it
                // is why the fixture test used to report "playback OK" for a
                // file no engine had touched.
                memset(target, 0, static_cast<size_t>(n) * 2 * sizeof(int16_t));
                break;
            }
            target += (n * 2);
            t -= n;
        }

        return noSamples;
    }

    bool seekTo(int /*song*/, int /*seconds*/) override { return false; }

private:
    // Both PSX engines only produce sound in whole video frames of
    // PSX_FRAME_SAMPLES, and neither tolerates a partial one:
    //
    //  * psf_gen() accumulates into PEOpS' internal 32K mixing buffer and only
    //    memcpy()s it out through SPU_flushboot(), which is a no-op until more
    //    than 1024 bytes (256 frames) have piled up. Ask it for fewer and you
    //    get nothing written this call and a double-length copy -- past the end
    //    of the caller's chunk -- on a later one.
    //  * psf2_gen() is stricter still: PEOpS2 flushes on an exact
    //    "== 735*4 bytes" test and ps2_update() memcpy()s to the buffer pointer
    //    unadvanced, so any other block size either never flushes or keeps
    //    overwriting sample 0.
    //
    // So generate in fixed 735-frame blocks (44100/60, the size the upstream AO
    // frontend used) into our own staging buffer and hand out slices of it. The
    // GUI's request size is whatever is left in the audio fifo and is NOT a
    // multiple of 735 -- see MusicPlayer::update().
    static constexpr int PSX_FRAME_SAMPLES = 735;

    int genPSX(int16_t* target, int noSamples)
    {
        // Never hand back half a stereo frame: the staging buffer carries L/R
        // pairs across calls, and an odd slice would swap the channels for the
        // rest of the song.
        noSamples &= ~1;
        int produced = 0;
        while (produced < noSamples) {
            if (stagePos >= stageFill) {
                if (filesig == SIG_PSF) {
                    psf_gen(stage.data(), PSX_FRAME_SAMPLES);
                } else {
                    psf2_gen(stage.data(), PSX_FRAME_SAMPLES);
                }
                stageFill = PSX_FRAME_SAMPLES * 2;
                stagePos = 0;
            }
            int n = stageFill - stagePos;
            if (n > noSamples - produced) { n = noSamples - produced; }
            memcpy(target + produced, stage.data() + stagePos,
                   static_cast<size_t>(n) * sizeof(int16_t));
            stagePos += n;
            produced += n;
        }
        return produced;
    }

    uint32_t filesig;
    std::vector<uint8_t> buffer; // see the note in the constructor -- the
                                 // engines retain pointers into this
    std::vector<int16_t> stage =
        std::vector<int16_t>(PSX_FRAME_SAMPLES * 2, 0);
    int stagePos = 0;
    int stageFill = 0;
    // string baseDir;
};

static const std::set<std::string> supported_ext = {
    "ssf", "minissf", "qsf", "miniqsf", "spu",
    // PlayStation 1/2. These four used to belong to heplugin (Highly
    // Experimental), which cannot start without a 512K Sony PS2 BIOS image --
    // data/hebios.bin, now deleted, since we have no right to redistribute it.
    // AOSDK's eng_psf/eng_psf2 play the same files through an HLE BIOS
    // (psx_hw.c) and need no such image; they were already being compiled here.
    "psf", "minipsf", "psf2", "minipsf2"};

// The PSF extensions -- the subset canHandle content-gates, since ".psf" is
// shared with vgmstream and with the modland "SoundFactory" Amiga corpus.
static const std::set<std::string> psf_ext = {"psf", "minipsf", "psf2",
                                              "minipsf2"};

bool AOPlugin::canHandle(const std::string& name)
{
    auto ext = utils::path_extension(name);
    if (supported_ext.count(ext) == 0) { return false; }
    if (psf_ext.count(ext) == 0) { return true; }

    // modland's "SoundFactory/" tree is an Amiga format that also uses ".psf"
    // and has nothing to do with the PlayStation; leave those to UADE. Same
    // quick path check HEPlugin did.
    if (utils::toLower(name).find("/soundfactory") != std::string::npos) {
        return false;
    }

    // Require the "PSF" magic AND a version byte this plugin implements: 0x01
    // (PSF1) or 0x02 (PSF2). Other versions share the container -- 0x11 SSF,
    // 0x12 DSF, 0x41 QSF -- and a rip mislabelled ".psf" must fall through to
    // whoever really owns it rather than be started as a PS1 executable.
    FILE* f = fopen(name.c_str(), "rb");
    if (f == nullptr) { return false; }
    std::array<char, 4> magic{};
    auto rc = fread(magic.data(), 1, magic.size(), f);
    fclose(f);
    if (rc != magic.size() || memcmp(magic.data(), "PSF", 3) != 0) {
        return false;
    }
    return magic[3] == 0x01 || magic[3] == 0x02;
}

std::set<std::string> AOPlugin::getSupportedExtensions() const
{
    return supported_ext;
}

std::vector<std::string> AOPlugin::getSecondaryFiles(const std::string& name)
{
    // .miniqsf/.minissf/.minipsf/.minipsf2 reference a shared library
    // (.qsflib/.ssflib/.psflib/.psf2lib) via PSF "_lib" tags; fetch it
    // alongside when streaming so ao_get_lib() finds it on a clean machine.
    return psfLibFiles(name);
}

ChipPlayer* AOPlugin::fromFile(const std::string& name)
{
    return new AOPlayer{name};
}

} // namespace musix
//
extern "C" void aoplugin_register()
{
    musix::ChipPlugin::addPluginConstructor([](std::string const& config) {
        return std::make_shared<musix::AOPlugin>();
    });
}
