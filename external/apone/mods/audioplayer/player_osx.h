#ifndef AUDIOPLAYER_OSX_H
#define AUDIOPLAYER_OSX_H

// Signals to audioplayer.cpp that this backend implements InternalPlayer::close().
#define AUDIOPLAYER_HAS_CLOSE 1

#include <AudioToolbox/AudioToolbox.h>
#include <atomic>
#include <cstring>
#include <mutex>

class InternalPlayer {
public:

	InternalPlayer(int hz = 44100) : freq(hz), quit(false) {
		init();
	}
	void init() {
		int bufSize = 32768/4;
		OSStatus status;
		AudioStreamBasicDescription fmt = { 0 };

		fmt.mSampleRate = freq;
		fmt.mFormatID = kAudioFormatLinearPCM;
		fmt.mFormatFlags = kAudioFormatFlagIsSignedInteger | kAudioFormatFlagIsPacked;
		fmt.mFramesPerPacket = 1;
		fmt.mChannelsPerFrame = 2;
		fmt.mBytesPerPacket = fmt.mBytesPerFrame = 2 * fmt.mChannelsPerFrame;
		fmt.mBitsPerChannel = 16;

		status = AudioQueueNewOutput(&fmt, fill_audio, this, NULL, NULL, 0, &aQueue);

  		//if (status == kAudioFormatUnsupportedDataFormatError)
		
		for(int i=0; i<4; i++) {
			AudioQueueBuffer *buf;
			status = AudioQueueAllocateBuffer(aQueue, bufSize, &buf);
			buf->mAudioDataByteSize = bufSize;
			fill_audio(this, aQueue, buf);
		}

 		status = AudioQueueSetParameter (aQueue, kAudioQueueParam_Volume, 1.0);
     	status = AudioQueueStart(aQueue, NULL);

	}

    void play(std::function<void(int16_t*, int)> cb) { 
        std::lock_guard<std::mutex> lock(callbackMutex);
        callback = cb; 
    }

	void pause(bool on) {
		if(!aQueue) return;
		if(on)
			AudioQueuePause(aQueue);
		else
     		AudioQueueStart(aQueue, NULL);
	}

	void set_volume(int volume) {
		if(!aQueue) return;
		float v = (float)volume / 100.f;
		AudioQueueSetParameter(aQueue, kAudioQueueParam_Volume, v);
	}
		


	static void fill_audio(void *ptr, AudioQueueRef aQueue, AudioQueueBuffer *buf) {
		InternalPlayer *player = static_cast<InternalPlayer*>(ptr);
		{
			std::lock_guard<std::mutex> lock(player->callbackMutex);
			if(!player->quit && player->callback) {
				int count = buf->mAudioDataByteSize / 2;
				int16_t *target = static_cast<int16_t*>(buf->mAudioData);
				player->callback(target, count);
			} else {
				// No callback (or shutting down): emit silence.
				memset(buf->mAudioData, 0, buf->mAudioDataByteSize);
			}
		}

		// ALWAYS hand the buffer back, teardown or not. A buffer that is handed
		// to this callback and never re-enqueued stays checked out to the client
		// forever, and AudioQueueDispose() then blocks in AwaitAllPendingCallbacks
		// until CoreAudio's ~10s timeout fires ("waiting for callbacks timed out!,
		// buffer count = 1") -- which is the beachball-on-quit itself.
		//
		// Recycling during teardown is safe because close() stops the queue with
		// immediate=true: the queue resets rather than draining, so it does not
		// matter that we keep feeding it, and no callback runs at all once Stop
		// has returned. On an already-stopped queue this call simply errors out
		// harmlessly.
		AudioQueueEnqueueBuffer(aQueue, buf, 0, NULL);
	}

	int get_delay() const { return 1; }


	// Bring the queue to a hard stop, synchronously and idempotently.
	//
	// MUST be used instead of pause() on any teardown path. AudioQueuePause()
	// only halts callback *dispatch*; a buffer callback that CoreAudio has
	// already handed to the client queue stays pending indefinitely, and the
	// later AudioQueueDispose() blocks ~10s waiting for it. Stop(immediate=true)
	// flushes that pending work and guarantees no fill_audio runs afterwards.
	void close() {
		if(closed) return;
		closed = true;

		// 1. Mark teardown under the callback mutex, so any in-flight fill_audio
		//    either already finished or emits silence.
		{
			std::lock_guard<std::mutex> lock(callbackMutex);
			quit = true;
		}

		if(aQueue) {
			// 2. Stop SYNCHRONOUSLY. immediate=true resets the queue instead of
			//    playing it out, so this returns promptly even though fill_audio
			//    keeps recycling buffers. On return, no callback can be running
			//    or pending.
			AudioQueueStop(aQueue, true);

			// 3. Drop the user callback now that nothing can invoke it.
			{
				std::lock_guard<std::mutex> lock(callbackMutex);
				callback = nullptr;
			}
		}
	}

	~InternalPlayer() {
		close();
		if(aQueue) {
			// Dispose a queue that is already stopped and owes no buffers, so
			// AwaitAllPendingCallbacks has nothing to wait for and returns at once.
			AudioQueueDispose(aQueue, true);
			aQueue = nullptr;
		}
	}

	void writeAudio(int16_t *samples, int sampleCount) {
	}

	std::function<void(int16_t *, int)> callback;
    std::mutex callbackMutex;
	std::atomic<bool> quit;
	bool closed = false;
	int freq;
	AudioQueueRef aQueue = nullptr;
};

#endif // AUDIOPLAYER_OSX_H
