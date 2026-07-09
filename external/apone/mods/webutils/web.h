#ifndef WEBUTILS_WEB_H
#define WEBUTILS_WEB_H

#include <coreutils/file.h>
#include <coreutils/log.h>
#include <coreutils/environment.h>
#include <string>
#include <cstdio>
#include <algorithm>
#define NOGDI
#include <curl/curl.h>
#include <unistd.h>
#include <cctype>
#include <memory>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>
#include <unordered_map>

namespace webutils {

class WebJob;
using StreamFunc = std::function<bool(WebJob&, uint8_t *, size_t)>;
class Web;

class WebJob {
public:
	WebJob() = default;
	virtual ~WebJob() = default;
	// `stopped` is std::atomic, which deletes the implicit copy ctor. A WebJob is
	// still copied by value into user callbacks that take `WebJob` (see the
	// WebJobImpl<...(WebJob) const> specialization -> cb(*this)), as a read-only
	// snapshot (code()/file()/getHeader()). Restore that copy, snapshotting the
	// flag's value. Copy-assignment stays deleted (never used).
	WebJob(const WebJob& o)
	    : curl(o.curl), headers(o.headers), url(o.url), data(o.data),
	      targetFile(o.targetFile), orgFile(o.orgFile), streamCb(o.streamCb),
	      isDone(o.isDone), stopped(o.stopped.load()), dirList(o.dirList),
	      cLength(o.cLength), tid(o.tid), header_list(o.header_list),
	      alias_list(o.alias_list) {}
	bool done() const { return isDone; }
	long code() const {
		long rc = -1;
		if(curl)
			curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &rc);
		return rc;
	}
	
	int64_t contentLength() { return cLength; }
	
	std::string getHeader(const std::string &name) {
		auto it = headers.find(name);
		if (it != headers.end()) return it->second;
		auto wanted = name;
		std::transform(wanted.begin(), wanted.end(), wanted.begin(), ::tolower);
		for (const auto& header : headers) {
			auto key = header.first;
			std::transform(key.begin(), key.end(), key.begin(), ::tolower);
			if (key == wanted) return header.second;
		}
		return "";
	}

	utils::File file() {
		return targetFile;
	}

	void wait(int timeout = -1);

	void stop() {
		stopped = true;
	}

	void setTarget(const utils::File &file) { targetFile = file; }

	void setUrl(const std::string &url) { this->url = url; }

	void setStreamCallback(StreamFunc cb) { streamCb = cb; }

	// Request an FTP directory listing (NLST) instead of a file transfer. The
	// listing text is delivered via textResult()/the string callback.
	void setDirList(bool b) { dirList = b; }

	std::string textResult() const {
		return std::string(data.begin(), data.end());
	}

protected:
	void start(CURLM *curlm);
	static size_t writeFunc(void *ptr, size_t size, size_t x, void *userdata);
	static size_t headerFunc(char *text, size_t size, size_t n, void *userdata);
	void finish();
	void destroy();

	virtual void call_handler() {}

	CURL *curl = nullptr;
	std::unordered_map<std::string, std::string> headers;
	std::string url;
	std::vector<uint8_t> data;
	utils::File targetFile;
	utils::File orgFile;
	StreamFunc streamCb;
	bool isDone = false;
	// Written by stop() (player thread via RemoteLoader::cancel) and read by
	// writeFunc() (web thread) — must be atomic to avoid a data race on abort.
	std::atomic<bool> stopped{false};
	bool dirList = false;
	int64_t cLength = 0;
	std::thread::id tid;

	std::shared_ptr<curl_slist> header_list;
	std::shared_ptr<curl_slist> alias_list;

	friend Web;
};

class Web {
public:

	template <typename... ARGS> struct WebJobImpl;

	template <typename FX, typename A0> struct WebJobImpl<void (FX::*)(A0) const>;

	template <typename FX> struct WebJobImpl<FX, void (FX::*)() const> : public WebJob {
		WebJobImpl(FX fx) : cb(fx) {}
		void call_handler() override { cb(); }
		FX cb;
	};

	template <typename FX> struct WebJobImpl<FX, void (FX::*)(WebJob) const> : public WebJob {
		WebJobImpl(FX fx) : cb(fx) {}
		void call_handler() override { cb(*this); }
		FX cb;
	};

	template <typename FX>
	struct WebJobImpl<FX, void (FX::*)(const std::string &contents) const> : public WebJob {
		WebJobImpl(FX fx) : cb(fx) {}
		void call_handler() override { cb(textResult()); }
		FX cb;
	};

	template <typename FX>
	struct WebJobImpl<FX, void (FX::*)(WebJob &, const std::string &contents) const> : public WebJob {
		WebJobImpl(FX fx) : cb(fx) {}
		void call_handler() override { cb(*this, textResult()); }
		FX cb;
	};

	template <typename FX> struct WebJobImpl<FX, void (FX::*)(utils::File) const> : public WebJob {
		WebJobImpl(FX fx) : cb(fx) {}
		void call_handler() override { cb(targetFile); }
		FX cb;
	};

	Web(const std::string &cacheDir = "", const std::string &baseUrl = "")
	    : baseUrl(baseUrl), cacheDir(cacheDir) {
		std::lock_guard<std::mutex> lock(sm);
		if(!initDone) {
			curl_global_init(CURL_GLOBAL_ALL);
			initDone = true;
		}
		utils::makedirs(cacheDir);
		curlm = curl_multi_init();
		// Cap concurrent connections. A single song can enqueue dozens of
		// companion fetches at once (e.g. an IFF-SMUS "Instruments/" folder has
		// ~60 files); firing them all in parallel overwhelms the FTP server and
		// many fail with response code 0. libcurl queues the overflow and starts
		// each transfer as a slot frees, so all of them complete.
		curl_multi_setopt(curlm, CURLMOPT_MAX_TOTAL_CONNECTIONS, 6L);
		curl_multi_setopt(curlm, CURLMOPT_MAX_HOST_CONNECTIONS, 6L);
		webThread = std::thread{&Web::run, this};
	}

	~Web() {
		// Signal quit FIRST, before acquiring m. This allows the run() thread —
		// if it is currently holding m inside curl_multi_perform — to see the
		// quit flag and exit its loop quickly once it releases m. Without this
		// ordering, if a streaming write-callback is blocked in a FIFO put()
		// while holding m, and that FIFO has already been quit() externally
		// (e.g. by MusicPlayerList::~MusicPlayerList()), the put() will return
		// promptly, the thread will release m, and the lock below will succeed.
		quit = true;
		if (curlm) {
			curl_multi_wakeup(curlm);
		}

		// Stop all in-flight jobs. The run() thread releases m between
		// curl_multi_perform iterations so this will complete without deadlock
		// provided no streaming callback is permanently blocked (ensured by
		// quitting the audio FIFOs before destroying RemoteLoader).
		{
			std::lock_guard<std::mutex> lock(m);
			for (auto& job : jobs) {
				job->stop();
				if (job->curl) {
					curl_multi_remove_handle(curlm, job->curl);
				}
			}
		}

		// JOIN — not detach. Detaching causes curl/audio races on destruction.
		if (webThread.joinable()) {
			webThread.join();
		}

		// Clean up curl only after the thread is fully stopped.
		if (curlm) {
			{
				std::lock_guard<std::mutex> lock(m);
				for (auto& job : jobs) {
					if (job->curl) {
						job->curl = nullptr;
						std::lock_guard<std::mutex> lock2(sm);
						runningWebJobs -= 1;
					}
				}
				jobs.clear();
			}
			curl_multi_cleanup(curlm);
			curlm = nullptr;
		}
	}

	// Advanced background execution thread: mitigates nested lock recursion 
	// by isolating user callback execution entirely outside of critical sections.
	void run() {
		while(!quit) {
			int handleCount;
			CURLMcode rc = CURLM_CALL_MULTI_PERFORM;
			std::vector<std::shared_ptr<WebJob>> completedJobs;

			{
				std::lock_guard<std::mutex> lock(m);
				while(rc == CURLM_CALL_MULTI_PERFORM)
					rc = curl_multi_perform(curlm, &handleCount);
				lastCount = handleCount;

				// Harvest completions without running complex nested code under lock
				CURLMsg *msg;
				int msgs_left;
				while ((msg = curl_multi_info_read(curlm, &msgs_left))) {
					if (msg->msg == CURLMSG_DONE) {
						auto it = jobs.begin();
						while (it != jobs.end()) {
							if ((*it)->curl == msg->easy_handle) {
								// Detach the finished/aborted handle from the multi
								// NOW, while we hold m. The easy handle itself is
								// freed further below (destroy(), also under m); the
								// user callbacks run in between WITHOUT m and may
								// re-enter Web. If the handle were still attached
								// when freed, curl_easy_cleanup() would mutate the
								// multi's transfer list concurrently with a
								// curl_multi_add_handle() from streamData()/getFile()
								// on the player thread -> heap corruption (reliably
								// hit by aborting a large in-flight stream while the
								// next track's transfer is starting).
								curl_multi_remove_handle(curlm, msg->easy_handle);
								completedJobs.push_back(*it);
								it = jobs.erase(it);
								break;
							} else {
								it++;
							}
						}
					}
				}
			}

			// Execute user and hardware callbacks completely safe from cross-thread mutex recursion
			for (auto& finishedJob : completedJobs) {
				finishedJob->finish();
			}
			// Free the curl easy handles only now, back under m. finish() ran the
			// user callbacks WITHOUT m (they may queue more transfers), but
			// curl_easy_cleanup() touches connection-cache/global state shared with
			// curl_multi_add_handle()/perform() on other threads and must be
			// serialized. The handles were already detached from the multi above,
			// so this is a pure easy-handle free.
			if (!completedJobs.empty()) {
				std::lock_guard<std::mutex> lock(m);
				for (auto& finishedJob : completedJobs) {
					finishedJob->destroy();
				}
			}
			completedJobs.clear();

			if (lastCount > 0) {
				utils::sleepms(2);
			} else {
				utils::sleepms(20);
			}

			std::this_thread::yield();
		}
	}

	static int inProgress() {
		std::lock_guard<std::mutex> lock(sm);
		return runningWebJobs;
	}

	template <typename FX> std::shared_ptr<WebJob> get(const std::string &url, FX cb) {
		auto job = std::make_shared<WebJobImpl<FX, decltype(&FX::operator())>>(cb);
		job->setUrl(url);
		job->start(curlm);
		jobs.push_back(job);
		return job;
	}

	// Fetch an FTP directory listing (bare NLST). The listing text is delivered
	// to the string callback. The result is held in memory (no cache file) since
	// listings are small and volatile.
	template <typename FX> std::shared_ptr<WebJob> listDir(const std::string &url, FX cb) {
		auto job = std::make_shared<WebJobImpl<FX, decltype(&FX::operator())>>(cb);
		job->setUrl(url);
		job->setDirList(true);
		std::lock_guard<std::mutex> lock(m);
		job->start(curlm);
		jobs.push_back(job);
		return job;
	}

	void poll() {
		if(!m.try_lock())
			return;
		auto it = jobs.begin();
		while(it != jobs.end()) {
			auto *curl = it->get()->curl;
			if(it->get()->tid != std::this_thread::get_id()) {
				it++;
				continue;
			}
			if(curl && it->get()->stopped) {
				LOGD("Removing stopped job");
				curl_multi_remove_handle(curlm, curl);
				it->get()->destroy();
			}
			if(!curl) {
				it = jobs.erase(it);
			} else
				it++;
		}
		m.unlock();
	}

	void removeJob(std::shared_ptr<WebJob> job) {
		std::lock_guard<std::mutex> lock(m);
		auto it = std::find(jobs.begin(), jobs.end(), job);
		if(it != jobs.end()) {
			jobs.erase(it);
			curl_multi_remove_handle(curlm, job->curl);
			job->destroy();
		}
	}

	template <typename FX> std::shared_ptr<WebJob> getFile(const std::string &url, FX cb) {
		auto job = std::make_shared<WebJobImpl<FX, decltype(&FX::operator())>>(cb);
		auto slash = url.find_last_of('/');
		auto fileName = url.substr(slash+1);
		auto pathName = url.substr(0, slash);
		auto urlPart = utils::urlencode(baseUrl + pathName, ":/\\?;!");
		auto fileNameEncoded = utils::urlencode(fileName, ":/\\?;!");
		utils::makedirs(cacheDir / urlPart);
		auto target = cacheDir / urlPart / fileNameEncoded;
		job->setTarget(target);
		job->setUrl(url);
		LOGD("target: %s", target.getName());
		if(target.exists()) {
			job->call_handler();
			job->targetFile = utils::File();
			job->isDone = true;
			return job;
		}
		std::lock_guard<std::mutex> lock(m);
		job->start(curlm);
		jobs.push_back(job);
		return job;
	}

	utils::File getFileBlocking(const std::string &url) {
		std::atomic<bool> done(false);
		utils::File retFile;
		auto job = getFile(url, [&](utils::File f) {
			retFile = f;
			done = true;
		});
		while(!done) {
			poll();
			utils::sleepms(100);
		}
		return retFile;
	}

	static std::string getBlocking(const std::string &url) {
		std::atomic<bool> done(false);
		std::string result;
		auto job = get_url(url, [&](const std::string &res) {
			LOGD("DONE");
			result = res;
			done = true;
		});
		LOGD("DONE: %s", done ? "yes" : "false");
		while(!done) {
			LOGD("POLL");
			getInstance().poll();
			utils::sleepms(500);
		}
		return result;
	}

	bool inCache(const std::string &url) const {
		auto slash = url.find_last_of('/');
		auto fileName = url.substr(slash+1);
		// Must match the exact encode set used by getFile() when the file was
		// stored (note the trailing '!'); otherwise URLs containing '!' resolve
		// to a different cache name here and a cached file is reported missing.
		fileName = utils::urlencode(fileName, ":/\\?;!");
		auto pathName = url.substr(0, slash);
		auto urlPart = utils::urlencode(baseUrl + pathName, ":/\\?;!");
		auto target = cacheDir / urlPart / fileName;
		return utils::File::exists(target);
	}

	std::shared_ptr<WebJob> streamData(const std::string &url, StreamFunc cb) {
		auto job = std::make_shared<WebJob>();
		job->setStreamCallback(cb);
		job->setUrl(url);
		std::lock_guard<std::mutex> lock(m);
		job->start(curlm);
		jobs.push_back(job);
		return job;
	}

	std::shared_ptr<WebJob> createWebJob(const std::string &url) {
		auto job = std::make_shared<WebJob>();
		job->setUrl(url);
		std::lock_guard<std::mutex> lock(m);
		job->start(curlm);
		jobs.push_back(job);
		return job;
	}

	static Web &getInstance() {
		static Web w((Environment::getCacheDir() / "_webfiles").string());
		return w;
	}

	template <typename FX> static std::shared_ptr<WebJob> get_url(const std::string &url, FX cb) {
		return getInstance().get(url, cb);
	}

	static void pollAll() {
		Web &w = getInstance();
		w.poll();
	}

private:
	static std::mutex sm;
	std::mutex m;
	std::thread webThread;
	std::atomic<bool> quit{false};
	std::string baseUrl;
	utils::File cacheDir;

	static bool initDone;

	CURLM *curlm = nullptr;

	std::vector<std::shared_ptr<WebJob>> jobs;
	int lastCount;
	static std::atomic<int> runningWebJobs;

	friend WebJob;
};

} // namespace webutils

#endif // WEBUTILS_WEB_H
