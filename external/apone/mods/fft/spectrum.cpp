#include "spectrum.h"

#include <fftw3.h>
#include <cmath>

using namespace std;

constexpr  int SpectrumAnalyzer::fft_size;
constexpr  int SpectrumAnalyzer::eq_slots;

struct SpectrumAnalyzer::Internal {
	float fftin[fft_size];
	fftwf_complex fftout[fft_size];
	fftwf_plan fftwp;
};

SpectrumAnalyzer::SpectrumAnalyzer() : eq(eq_slots), powerLeft(fft_size/2+1), powerRight(fft_size/2+1) {
	si = new Internal();
	//fftwp = rfftw_create_plan(fft_size, FFTW_REAL_TO_COMPLEX, FFTW_ESTIMATE);
	si->fftwp = fftwf_plan_dft_r2c_1d(fft_size, si->fftin, si->fftout, FFTW_ESTIMATE);
}

SpectrumAnalyzer::~SpectrumAnalyzer() {
	fftwf_destroy_plan(si->fftwp);
	delete si;
}

void SpectrumAnalyzer::popLevels() {
	lock_guard<mutex> guard(m);
	spectrum.pop_front();
}

static void calcPower(fftwf_complex *fftout, vector<float> &power) {
	power[0] = fftout[0][0]*fftout[0][0];  /* DC component */
	for(int k = 1; k < (SpectrumAnalyzer::fft_size+1)/2; ++k)  /* (k < fft_size/2 rounded up) */
		power[k] = fftout[k][0]*fftout[k][0] + fftout[SpectrumAnalyzer::fft_size-k][0]*fftout[SpectrumAnalyzer::fft_size-k][0];
	power[SpectrumAnalyzer::fft_size/2] = fftout[SpectrumAnalyzer::fft_size/2][0]*fftout[SpectrumAnalyzer::fft_size/2][0];  /* Nyquist freq. */
}

static void fillSlots(SpectrumAnalyzer::Levels &spec, const vector<float> &power) {
	float sum = 0;
	int j = 0;

	auto d =  pow(10.0, (3.0/(10.0 * 3)));

	float freq_per_slot = 44100.0/SpectrumAnalyzer::fft_size;

	float start_freq = 22.0;
	float next_freq = start_freq * d;
	float curr_freq = 0;

	for(int i=1; i<SpectrumAnalyzer::fft_size/2; i++) {

		if(curr_freq >= start_freq)
			sum += power[i];

		if(curr_freq >= next_freq) {
			spec[j++] = sqrt(sum);
			if(j == SpectrumAnalyzer::eq_slots)
				break;
			sum = 0;
			next_freq *= d;
		}
		curr_freq += freq_per_slot;
	}
}

void SpectrumAnalyzer::addAudio(int16_t *samples, int len) {

	int pos = 0;

	while(pos + fft_size * 2 <= len) {
		for(int i=0; i<fft_size; i++) {
			si->fftin[i] = (float)samples[pos+i*2]/32768.0;
		}
		fftwf_execute(si->fftwp);
		calcPower(si->fftout, powerLeft);

		for(int i=0; i<fft_size; i++) {
			si->fftin[i] = (float)samples[pos+i*2+1]/32768.0;
		}
		fftwf_execute(si->fftwp);
		calcPower(si->fftout, powerRight);

		pos += fft_size*2;

		{ lock_guard<mutex> guard(m);
			spectrum.emplace_back();
			auto &spec = spectrum.back();
			fillSlots(spec.left, powerLeft);
			fillSlots(spec.right, powerRight);
		}
	}
}
