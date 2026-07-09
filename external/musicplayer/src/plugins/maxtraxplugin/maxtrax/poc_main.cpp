// poc_main.cpp -- standalone proof-of-concept decoder for MaxTrax (.mxtx).
//
// Loads a .mxtx module via the vendored ScummVM MaxTrax/Paula player, renders
// a fixed span of audio, writes a 16-bit stereo WAV, and prints peak/RMS so we
// can confirm the output is real (non-silent) audio.
//
//   build:  see Makefile in this directory
//   run:    ./maxtrax_poc <file.mxtx> [out.wav] [seconds] [songIndex]

#include "compat.h"
#include "maxtrax.h"

#include <cstdio>
#include <cstdlib>
#include <vector>
#include <cmath>

static std::vector<byte> readFile(const char *path) {
	FILE *f = fopen(path, "rb");
	if (!f) { fprintf(stderr, "cannot open %s\n", path); exit(2); }
	fseek(f, 0, SEEK_END);
	long n = ftell(f);
	fseek(f, 0, SEEK_SET);
	std::vector<byte> buf(n);
	if (fread(buf.data(), 1, n, f) != (size_t)n) { fprintf(stderr, "read error\n"); exit(2); }
	fclose(f);
	return buf;
}

static void writeLE32(FILE *f, uint32 v) { byte b[4] = { (byte)v, (byte)(v >> 8), (byte)(v >> 16), (byte)(v >> 24) }; fwrite(b, 1, 4, f); }
static void writeLE16(FILE *f, uint16 v) { byte b[2] = { (byte)v, (byte)(v >> 8) }; fwrite(b, 1, 2, f); }

static void writeWav(const char *path, const std::vector<int16> &pcm, int rate, int channels) {
	FILE *f = fopen(path, "wb");
	if (!f) { fprintf(stderr, "cannot write %s\n", path); exit(2); }
	const uint32 dataBytes = (uint32)(pcm.size() * sizeof(int16));
	fwrite("RIFF", 1, 4, f); writeLE32(f, 36 + dataBytes); fwrite("WAVE", 1, 4, f);
	fwrite("fmt ", 1, 4, f); writeLE32(f, 16); writeLE16(f, 1); writeLE16(f, channels);
	writeLE32(f, rate); writeLE32(f, rate * channels * 2); writeLE16(f, channels * 2); writeLE16(f, 16);
	fwrite("data", 1, 4, f); writeLE32(f, dataBytes);
	fwrite(pcm.data(), 1, dataBytes, f);
	fclose(f);
}

int main(int argc, char **argv) {
	if (argc < 2) { fprintf(stderr, "usage: %s <file.mxtx> [out.wav] [seconds] [songIndex]\n", argv[0]); return 1; }
	const char *inPath  = argv[1];
	const char *outPath = (argc > 2) ? argv[2] : "out.wav";
	const double seconds = (argc > 3) ? atof(argv[3]) : 10.0;
	const int songIndex  = (argc > 4) ? atoi(argv[4]) : 0;

	const int rate = 44100;
	const bool stereo = true;

	std::vector<byte> file = readFile(inPath);
	Common::SeekableReadStream stream(file.data(), (uint32)file.size());

	Audio::MaxTrax player(rate, stereo);
	if (!player.load(stream, true, true)) {
		fprintf(stderr, "load() failed -- not a valid MaxTrax module\n");
		return 3;
	}
	printf("loaded OK (%zu bytes)\n", file.size());

	if (!player.playSong(songIndex, /*loop*/ false)) {
		fprintf(stderr, "playSong(%d) failed\n", songIndex);
		return 4;
	}

	const int totalFrames = (int)(seconds * rate);
	const int chans = stereo ? 2 : 1;
	std::vector<int16> pcm;
	pcm.reserve((size_t)totalFrames * chans);

	const int blockFrames = 4096;
	std::vector<int16> block(blockFrames * chans);
	int rendered = 0;
	double sumSq = 0.0;
	int peak = 0;
	while (rendered < totalFrames) {
		int wantFrames = std::min(blockFrames, totalFrames - rendered);
		int wantSamples = wantFrames * chans;
		// Paula::readBuffer takes the number of samples (channels interleaved).
		player.readBuffer(block.data(), wantSamples);
		for (int i = 0; i < wantSamples; ++i) {
			int16 s = block[i];
			pcm.push_back(s);
			int a = (s < 0) ? -s : s;
			if (a > peak) peak = a;
			sumSq += (double)s * s;
		}
		rendered += wantFrames;
	}

	double rms = std::sqrt(sumSq / (pcm.empty() ? 1 : pcm.size()));
	writeWav(outPath, pcm, rate, chans);

	printf("rendered %.1fs (%d frames, %d ch) -> %s\n", seconds, rendered, chans, outPath);
	printf("peak=%d (%.1f%% FS)  rms=%.1f\n", peak, 100.0 * peak / 32767.0, rms);
	printf("%s\n", (peak > 200) ? "RESULT: non-silent audio produced \xE2\x9C\x93"
	                            : "RESULT: output is silent/near-silent \xE2\x9C\x97");
	return (peak > 200) ? 0 : 5;
}
