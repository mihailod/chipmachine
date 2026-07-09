#ifndef AUDIOPLAYER_OSX_H
#define AUDIOPLAYER_OSX_H

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

		// Once teardown has begun, STOP recycling buffers. Re-enqueueing here
		// while the destructor runs AudioQueueStop/AudioQueueDispose on the main
		// thread is exactly what produced the multi-second beachball: the queue
		// could never drain because every callback fed it another buffer.
		if(player->quit) return;

		AudioQueueEnqueueBuffer(aQueue, buf, 0, NULL);
	}

	int get_delay() const { return 1; }


	~InternalPlayer() {
		// 1. Mark teardown under the callback mutex. After this, any in-flight
		//    fill_audio either already finished or will emit silence and will
		//    NOT re-enqueue a buffer.
		{
			std::lock_guard<std::mutex> lock(callbackMutex);
			quit = true;
		}

		if(aQueue) {
			// 2. Stop the queue SYNCHRONOUSLY (immediate = true). CoreAudio
			//    guarantees no further fill_audio callbacks run after this
			//    returns, and it returns promptly because fill_audio no longer
			//    recycles buffers. This bounded stop replaces relying on
			//    AudioQueueDispose to untangle an actively-fed queue (which
			//    beachballed the main thread for several seconds on quit).
			AudioQueueStop(aQueue, true);

			// 3. Drop the user callback now that nothing can invoke it.
			{
				std::lock_guard<std::mutex> lock(callbackMutex);
				callback = nullptr;
			}

			// 4. Dispose the now-stopped, idle queue and its buffers.
			AudioQueueDispose(aQueue, true);
			aQueue = nullptr;
		}
	}

	void writeAudio(int16_t *samples, int sampleCount) {
	}

	std::function<void(int16_t *, int)> callback;
    std::mutex callbackMutex;
	std::atomic<bool> quit;
	int freq;
	AudioQueueRef aQueue = nullptr;
};

#endif // AUDIOPLAYER_OSX_H
