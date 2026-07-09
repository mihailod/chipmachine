// chipmachine port: the upstream SoundSink was a boost::thread-driven real-time
// audio pump that also dlopen()'d a platform sink module (ALSA/JACK/DSound) and
// ran a separate "time loop" thread for tracker-row callbacks. The chipmachine
// host is pull-based (getSamples) and single-threaded per player, so none of
// that machinery is needed: we drive SoundGen synchronously (see
// SoundGen::renderSamples) and never let the sink own a thread.
//
// This shim keeps the exact SoundSink API surface SoundGen links against
// (ctor/dtor, setPlaying, performSoundCallback, applyTime, blockUntil*), but
// every method is a trivial, thread-free, boost-free no-op. loadSoundSink()
// is unused (we supply our own concrete sink in the plugin) and returns NULL.

#include <stdio.h>
#include "common.hpp"
#include "ringbuffer.hpp"
#include "soundsink.hpp"

namespace core
{
	// Empty placeholder so the header's `_soundsink_threading_t *m_threading`
	// member has a complete type to new/delete.
	struct _soundsink_threading_t
	{
	};

	SoundSink::SoundSink()
		: m_soundCallback(NULL), m_timeCallback(NULL), m_callbackData(NULL),
		  m_playing(false), m_timeidxsz(0)
	{
		m_threading = new _soundsink_threading_t;
		m_timeidx_ringbuffer = NULL;
	}

	SoundSink::~SoundSink()
	{
		delete m_threading;
	}

	void SoundSink::setPlaying(bool playing)
	{
		// No background thread to start/stop; just record the flag.
		m_playing = playing;
	}

	void SoundSink::performSoundCallback(s16 *buf, u32 sz)
	{
		// Direct, synchronous render. The time-index array is ignored because we
		// don't run a tracker-row time loop in headless playback.
		if (m_soundCallback != NULL)
			m_timeidxsz = (*m_soundCallback)(buf, sz, m_callbackData, m_timeidx);
	}

	void SoundSink::applyTime(core::s32 /*delay_us*/)
	{
		// Row/frame timing callbacks are a GUI concern; no-op here.
		m_timeidxsz = 0;
	}

	void SoundSink::blockUntilStopped()
	{
	}

	void SoundSink::blockUntilTimerEmpty()
	{
	}

	core::SoundSink * loadSoundSink(const char * /*name*/)
	{
		// Dynamic platform-sink loading is unused in chipmachine.
		return NULL;
	}

	SoundSinkPlayback::SoundSinkPlayback() {}
	SoundSinkPlayback::SoundSinkPlayback(const SoundSinkPlayback &) {}
	SoundSinkPlayback & SoundSinkPlayback::operator =(const SoundSinkPlayback &)
	{
		return *this;
	}
	SoundSinkPlayback::~SoundSinkPlayback() {}

	void SoundSinkExport::render() {}
}
