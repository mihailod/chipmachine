#ifndef SPECTRUM_H
#define SPECTRUM_H

#include <vector>
#include <deque>
#include <coreutils/thread.h>
#include <array>

class SpectrumAnalyzer {
public:
	constexpr static int fft_size = 1024;
	constexpr static int eq_slots = 24;
	using Levels = std::array<uint16_t, eq_slots>;

	struct StereoLevels {
		Levels left{};
		Levels right{};
	};

	struct Internal;

private:

	std::deque<StereoLevels> spectrum;
	std::mutex m;
	std::vector<uint8_t> eq;
	std::vector<float> powerLeft;
	std::vector<float> powerRight;

	struct Internal *si;
public:

	SpectrumAnalyzer();
	~SpectrumAnalyzer();

	const Levels getLevels() {
		std::lock_guard<std::mutex> guard(m);
		Levels mixed{};
		for (int i = 0; i < eq_slots; i++) {
			mixed[i] = (spectrum.front().left[i] + spectrum.front().right[i]) / 2;
		}
		return mixed;
	}

	const StereoLevels getStereoLevels() {
		std::lock_guard<std::mutex> guard(m);
		return spectrum.front();
	}

	int size() {
		std::lock_guard<std::mutex> guard(m);
		return spectrum.size();
	}

	void popLevels();

	void addAudio(int16_t *samples, int len);

};

#endif // SPECTRUM_H
