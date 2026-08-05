// A/B probe for libvgm chip-core swaps. Built only when CM_BUILD_VGM_PROBE=ON:
//
//   cmake -B build-probe-plus -DCM_VARIANT=plus -DCM_BUILD_VGM_PROBE=ON ...
//
// It links the SAME object library the plugin links, so it sees exactly the
// SNDDEV_/EC_ selection and source list that variant ships -- which is the point:
// a core that is not compiled does not fail loudly, VGMPlayer just sets
// devDef = NULL and renders that chip silent.
//
// For each file given it prints the core libvgm actually chose per device
// (PLR_DEV_INFO.core is a FourCC) and the RMS / peak / clipped-sample count of
// the rendered PCM, so `plus` and `mas` output can be diffed directly.
//
// TRAP, hit for real on the OPN work: PlayerA::Render() fills at most ONE
// internal buffer per call and returns the byte count. Ignore the return value
// and most of what you "rendered" is untouched silence.

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

#include "player/playera.hpp"
#include "player/playerbase.hpp"
#include "player/vgmplayer.hpp"
#include "utils/DataLoader.h"
#include "utils/FileLoader.h"

namespace
{

const uint32_t kSampleRate = 44100;
const uint32_t kBufferFrames = 2048;

void fcc_str(UINT32 fcc, char out[5])
{
	out[0] = (char)((fcc >> 24) & 0xFF);
	out[1] = (char)((fcc >> 16) & 0xFF);
	out[2] = (char)((fcc >> 8) & 0xFF);
	out[3] = (char)(fcc & 0xFF);
	out[4] = '\0';
	for (int i = 0; i < 4; i++)
		if (out[i] < 0x20 || out[i] > 0x7E) out[i] = '.';
}

void print_dev(const PLR_DEV_INFO& dev, int indent)
{
	char core[5];
	fcc_str(dev.core, core);
	printf("%*sdev %2u  type=0x%02X  core=%-4s  vol=0x%03X  rate=%u\n",
	       indent, "", (unsigned)dev.id, dev.type, core,
	       (unsigned)dev.volume, (unsigned)dev.smplRate);
	for (size_t i = 0; i < dev.devLink.size(); i++) print_dev(dev.devLink[i], indent + 3);
}

// Optional WAV dump, so a swap can be checked by ear and not only by RMS.
const char* g_wavDir = NULL;

FILE* wav_open(const char* path)
{
	if (g_wavDir == NULL) return NULL;
	const char* base = strrchr(path, '/');
	base = base ? base + 1 : path;
	char out[2048];
	snprintf(out, sizeof(out), "%s/%s.wav", g_wavDir, base);
	FILE* f = fopen(out, "wb");
	if (f == NULL) return NULL;
	unsigned char hdr[44] = {0};
	memcpy(hdr, "RIFF----WAVEfmt ", 16);
	hdr[16] = 16; hdr[20] = 1; hdr[22] = 2;			// PCM, 2 channels
	hdr[24] = kSampleRate & 0xFF; hdr[25] = (kSampleRate >> 8) & 0xFF;
	uint32_t byteRate = kSampleRate * 4;
	memcpy(hdr + 28, &byteRate, 4);
	hdr[32] = 4; hdr[34] = 16;						// block align, bits
	memcpy(hdr + 36, "data", 4);
	fwrite(hdr, 1, 44, f);
	return f;
}

void wav_close(FILE* f)
{
	if (f == NULL) return;
	long end = ftell(f);
	uint32_t dataSz = (uint32_t)(end - 44), riffSz = (uint32_t)(end - 8);
	fseek(f, 4, SEEK_SET);  fwrite(&riffSz, 4, 1, f);
	fseek(f, 40, SEEK_SET); fwrite(&dataSz, 4, 1, f);
	fclose(f);
}

int probe(const char* path, double seconds)
{
	DATA_LOADER* loader = FileLoader_Init(path);
	if (loader == NULL) { printf("%s: cannot open\n", path); return 1; }
	DataLoader_SetPreloadBytes(loader, 0x100);
	if (DataLoader_Load(loader) != 0)
	{
		printf("%s: cannot load\n", path);
		DataLoader_Deinit(loader);
		return 1;
	}

	PlayerA player;
	player.RegisterPlayerEngine(new VGMPlayer);
	player.SetOutputSettings(kSampleRate, 2, 16, kBufferFrames);

	PlayerA::Config cfg = player.GetConfiguration();
	cfg.masterVol = 0x10000;
	cfg.loopCount = 2;
	cfg.fadeSmpls = kSampleRate * 4;
	cfg.endSilenceSmpls = kSampleRate / 2;
	cfg.pbSpeed = 1.0;
	player.SetConfiguration(cfg);

	if (player.LoadFile(loader) != 0)
	{
		printf("%s: LoadFile failed\n", path);
		player.UnregisterAllPlayers();
		DataLoader_Deinit(loader);
		return 1;
	}

	printf("== %s\n", path);

	// Start() first: before it, GetSongDeviceInfo() reports core = 0 and
	// smplRate = 0 because the devices have not been instantiated yet.
	player.Start();

	PlayerBase* engine = player.GetPlayer();
	std::vector<PLR_DEV_INFO> devList;
	engine->GetSongDeviceInfo(devList);
	// Linked devices matter here: the OPL4's FM half is a separate YMF262 device
	// hanging off the YMF278B, and a core swap must not disturb it.
	for (size_t i = 0; i < devList.size(); i++) print_dev(devList[i], 3);

	const uint32_t wantFrames = (uint32_t)(seconds * kSampleRate);
	std::vector<int16_t> chunk(kBufferFrames * 2);
	FILE* wav = wav_open(path);
	double sumSq = 0.0;
	uint32_t frames = 0, clipped = 0;
	int32_t peak = 0;
	while (frames < wantFrames)
	{
		if ((player.GetState() & PLAYSTATE_FIN) != 0) break;
		// Honour the return value -- Render fills one internal buffer per call.
		uint32_t bytes = player.Render((uint32_t)(chunk.size() * sizeof(int16_t)),
		                               (void*)chunk.data());
		if (bytes == 0) break;
		uint32_t got = (uint32_t)(bytes / sizeof(int16_t));
		if (wav != NULL) fwrite(chunk.data(), sizeof(int16_t), got, wav);
		for (uint32_t i = 0; i < got; i++)
		{
			int32_t s = chunk[i];
			sumSq += (double)s * (double)s;
			if (s < 0) s = -s;
			if (s > peak) peak = s;
			if (s >= 32767) clipped++;
		}
		frames += got / 2;
	}

	wav_close(wav);

	// Re-read after rendering: a core that uses SetSampleRateChangeCallback (the
	// 32X PWM does, its rate is clock/cycle and the cycle register is written by
	// the log) reports a different rate here than it did at Start().
	{
		std::vector<PLR_DEV_INFO> after;
		engine->GetSongDeviceInfo(after);
		for (size_t i = 0; i < after.size() && i < devList.size(); i++)
			if (after[i].smplRate != devList[i].smplRate)
				printf("   dev %2u  rate CHANGED %u -> %u\n", (unsigned)after[i].id,
				       (unsigned)devList[i].smplRate, (unsigned)after[i].smplRate);
	}

	double rms = frames ? std::sqrt(sumSq / (double)(frames * 2)) : 0.0;
	printf("   frames=%u  rms=%.2f  peak=%d  clipped=%u\n\n", frames, rms, peak, clipped);

	player.Stop();
	player.UnloadFile();
	player.UnregisterAllPlayers();
	DataLoader_Deinit(loader);
	return 0;
}

} // namespace

int main(int argc, char** argv)
{
	double seconds = 30.0;
	int first = 1;
	while (first + 1 < argc)
	{
		if (strcmp(argv[first], "-t") == 0) { seconds = atof(argv[first + 1]); first += 2; }
		else if (strcmp(argv[first], "-w") == 0) { g_wavDir = argv[first + 1]; first += 2; }
		else break;
	}
	if (first >= argc)
	{
		fprintf(stderr, "usage: vgm_core_probe [-t seconds] [-w wavdir] file.vgm...\n");
		return 2;
	}
	int rc = 0;
	for (int i = first; i < argc; i++) rc |= probe(argv[i], seconds);
	return rc;
}
