#include "web.h"

#include <array>
#include <map>
#include <random>

namespace webutils {

// Extract the "host[:port]" authority from a URL, lowercased. Returns the whole
// string if it has no scheme/authority (defensive — keeps the host map keyed on
// *something* stable rather than crashing on odd inputs).
static std::string hostOf(const std::string &url) {
	auto schemeEnd = url.find("://");
	size_t start = (schemeEnd == std::string::npos) ? 0 : schemeEnd + 3;
	auto end = url.find('/', start);
	std::string host = url.substr(start, end == std::string::npos ? std::string::npos : end - start);
	// Drop any userinfo@ prefix.
	auto at = host.find('@');
	if (at != std::string::npos)
		host = host.substr(at + 1);
	std::transform(host.begin(), host.end(), host.begin(), ::tolower);
	return host;
}

// Return a realistic, current-ish browser User-Agent for the given host. The UA
// is chosen at random the first time we contact a host, then kept stable for the
// rest of the session (process lifetime). This is more convincing than rotating
// per request: a real browser presents one consistent identity to a given
// server, and many different UAs hammering one server from one IP is itself a
// tell. Different runs still get different identities (the RNG is seeded fresh).
static const char* pickUserAgent(const std::string &url) {
	static const std::array<const char*, 8> agents = {{
		// macOS Safari
		"Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/605.1.15 (KHTML, like Gecko) Version/17.4.1 Safari/605.1.15",
		// macOS Chrome
		"Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/124.0.0.0 Safari/537.36",
		// macOS Firefox
		"Mozilla/5.0 (Macintosh; Intel Mac OS X 10.15; rv:125.0) Gecko/20100101 Firefox/125.0",
		// Windows 10 Chrome
		"Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/124.0.0.0 Safari/537.36",
		// Windows 10 Edge
		"Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/123.0.0.0 Safari/537.36 Edg/123.0.2420.97",
		// Windows 10 Firefox
		"Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:125.0) Gecko/20100101 Firefox/125.0",
		// Linux Chrome
		"Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/123.0.0.0 Safari/537.36",
		// iPhone Safari
		"Mozilla/5.0 (iPhone; CPU iPhone OS 17_4 like Mac OS X) AppleWebKit/605.1.15 (KHTML, like Gecko) Version/17.4 Mobile/15E148 Safari/604.1",
	}};

	// host -> chosen UA index, shared across all worker threads, so guard it.
	static std::mutex hostMutex;
	static std::map<std::string, size_t> hostAgent;
	static std::mt19937 rng{std::random_device{}()};

	std::string host = hostOf(url);

	std::lock_guard<std::mutex> lock(hostMutex);
	auto it = hostAgent.find(host);
	if (it == hostAgent.end()) {
		std::uniform_int_distribution<size_t> dist(0, agents.size() - 1);
		it = hostAgent.emplace(host, dist(rng)).first;
	}
	return agents[it->second];
}

std::atomic<int> Web::runningWebJobs(0);
std::mutex Web::sm;
bool Web::initDone = false;

void WebJob::start(CURLM *curlm) {

	curl = curl_easy_init();

	tid = std::this_thread::get_id();

	if(targetFile) {
		orgFile = targetFile;
		targetFile = targetFile + ".download";
	}

	// Build the final URL string to hand to libcurl.
	//
	// HTTP/HTTPS: percent-encode spaces and shell-special chars so the HTTP
	// request line is well-formed. urlencode() is fine here.
	//
	// FTP: CURLFTPMETHOD_NOCWD requires libcurl to receive the *raw* path so
	// it can issue a single RETR without CWD traversal. We must NOT
	// percent-encode spaces (they must stay as literal spaces in the FTP
	// command). However characters like '!' are rejected by libcurl's internal
	// URL parser when they appear unescaped in the authority/path separator
	// position. The correct approach is to let libcurl escape only the path
	// component itself via curl_easy_escape(), leaving the scheme+host intact.
	std::string u;
	bool isFtp = url.size() > 4 && url.substr(0, 4) == "ftp:";
	if (isFtp) {
		// Split off scheme+host from path: ftp://host/path
		// find the third slash (after ftp://)
		auto pathStart = url.find('/', 6); // skip past "ftp://"
		if (pathStart != std::string::npos) {
			std::string hostPart = url.substr(0, pathStart);
			std::string pathPart = url.substr(pathStart + 1); // without leading /
			char* escaped = curl_easy_escape(curl, pathPart.c_str(), (int)pathPart.size());
			// curl_easy_escape encodes everything including '/' which we need
			// to preserve as path separators — so unescape %2F back to /
			std::string escapedPath(escaped);
			curl_free(escaped);
			// Restore path separators
			std::string finalPath;
			finalPath.reserve(escapedPath.size());
			size_t i = 0;
			while (i < escapedPath.size()) {
				if (escapedPath.size() - i >= 3 &&
				    escapedPath[i] == '%' &&
				    escapedPath[i+1] == '2' &&
				    (escapedPath[i+2] == 'F' || escapedPath[i+2] == 'f')) {
					finalPath += '/';
					i += 3;
				} else {
					finalPath += escapedPath[i++];
				}
			}
			u = hostPart + "/" + finalPath;
		} else {
			u = url; // fallback: pass as-is
		}
	} else {
		u = utils::urlencode(url, " #()");
	}

	// A radio stream is the only case that needs the SHOUTcast/Icecast-specific
	// headers (Icy-MetaData + the "ICY 200 OK" status-line alias) and HTTP/1.0.
	// Plain file/page downloads don't, so for those we send a header set that
	// looks like an ordinary web browser instead of a media player. That audio
	// Accept list and the Icy header are a dead giveaway that the client is an
	// automated player, so we only emit them when actually streaming radio.
	bool isStream = static_cast<bool>(streamCb);

	curl_slist *slist = NULL;

	std::string uaHeader = std::string("User-Agent: ") + pickUserAgent(url);
	slist = curl_slist_append(slist, uaHeader.c_str());

	if (isStream) {
		// Radio streaming: keep the player-style headers the servers expect.
		slist = curl_slist_append(slist, "Icy-MetaData: 1");
		slist = curl_slist_append(slist, "Accept: audio/mpeg, audio/x-mpeg, audio/mp3, audio/x-mp3, audio/mpeg3, audio/x-mpeg3, audio/mpg, audio/x-mpg, audio/x-mpegaudio, application/octet-stream, audio/mpegurl, audio/mpeg-url, audio/x-mpegurl, audio/x-scpls, audio/scpls, application/pls, application/x-scpls, */*");
	} else {
		// Ordinary download: mimic a real browser's request headers. These are
		// the headers every mainstream browser sends regardless of vendor, so
		// they stay consistent with whichever UA pickUserAgent() returned.
		// Deliberately DO NOT advertise image/avif or image/webp: content-
		// negotiating CDNs (e.g. uploads.podcloud.fr) honour those and return a
		// WebP/AVIF body even for a ".jpg" URL, which stb_image cannot decode
		// (screenshot/artwork then fails to load). Our decoder only handles
		// PNG/JPEG/GIF, so we advertise a plain "*/*" and let the server send the
		// original raster the URL points at.
		slist = curl_slist_append(slist, "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8");
		slist = curl_slist_append(slist, "Accept-Language: en-US,en;q=0.9");
	}
	header_list = std::shared_ptr<curl_slist>(slist, &curl_slist_free_all);

	slist = NULL;
	slist = curl_slist_append(slist, "ICY 200 OK");
	alias_list = std::shared_ptr<curl_slist>(slist, &curl_slist_free_all);

	LOGD("Curl Getting %s", u);
	curl_easy_setopt(curl, CURLOPT_URL, u.c_str());
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, header_list.get());
	
	// Force immediate IPv4 lookups to bypass the ~2.5 second macOS IPv6 dual-stack fallback timeout
	curl_easy_setopt(curl, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V4);

	// Optimization to eliminate sequential step-by-step CWD roundtrip stalls on deep FTP paths
	curl_easy_setopt(curl, CURLOPT_FTP_FILEMETHOD, CURLFTPMETHOD_NOCWD);
	curl_easy_setopt(curl, CURLOPT_FTP_USE_EPSV, 1L);
	// Directory-listing jobs (FTP NLST): return bare entries, not a file body.
	// With NOCWD the server echoes full paths; the caller strips to basenames.
	if (dirList) {
		curl_easy_setopt(curl, CURLOPT_DIRLISTONLY, 1L);
	}
	// Suppress the FTP SIZE command — costs ~500ms round-trip per transfer.
	// FTP ONLY: on HTTP this flag makes curl ignore Content-Length and read
	// until the connection closes. Servers that keep the connection alive (no
	// prompt close) then stall the transfer until their keep-alive timeout fires
	// (~60-75s), so every HTTP download hung for over a minute (e.g. the
	// c64takeaway.com podcast artwork). HTTP always carries a reliable body
	// length (Content-Length or chunked/HTTP2 END_STREAM), so it never needs this.
	if (u.rfind("ftp", 0) == 0)
		curl_easy_setopt(curl, CURLOPT_IGNORE_CONTENT_LENGTH, 1L);

	if (isStream) {
		// SHOUTcast servers answer with "ICY 200 OK" over HTTP/1.0.
		curl_easy_setopt(curl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_0);
		curl_easy_setopt(curl, CURLOPT_HTTP200ALIASES, alias_list.get());
	} else {
		// Force HTTP/1.1 -- do NOT negotiate up to HTTP/2.
		//
		// libcurl's HTTP/2 teardown (http2_data_done -> Curl_bufq_free, and the
		// prune_dead_connections path) corrupts the heap on this build: a single
		// fresh HTTPS/HTTP-2 download reliably poisons a nearby free block, which
		// then trace-traps much later in an unrelated allocation (usually font
		// make_text). It is a small, layout-dependent stray write -- invisible to
		// ASan/guard-malloc (layout shifts hide it) and it vanishes once every URL
		// is cached (no transfer runs). CURLOPT_FORBID_REUSE below was the earlier
		// mitigation (avoid pruning STALE cached connections) but it is NOT enough:
		// the HTTP/2 close still runs per transfer. Dropping to HTTP/1.1 removes
		// the buggy code path entirely. Downloads here are small files; the only
		// thing lost is a little multiplexing speed. (Proper cure: bump libcurl.)
		curl_easy_setopt(curl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_1);
		// FORBID connection reuse (kept as belt-and-suspenders; harmless on 1.1).
		// HTTP(S) ONLY: on FTP, FORBID_REUSE makes curl close the control
		// connection after each file, so the last response code captured is the
		// QUIT reply 221 instead of the transfer's 226 -> WebJob::finish() treats
		// !=200/226 as failure and DELETES the file, breaking every modland FTP
		// download.
		if (u.rfind("http", 0) == 0)
			curl_easy_setopt(curl, CURLOPT_FORBID_REUSE, 1L);
		// Advertise and transparently decode the compression a browser would
		// (passing "" lets libcurl send everything it was built with and inflate
		// the response for us, so callers still see plain bytes).
		curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "");
	}
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
	curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
	curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
	

	curl_easy_setopt(curl, CURLOPT_WRITEDATA, this);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeFunc);
	curl_easy_setopt(curl, CURLOPT_HEADERDATA, this);
	curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, headerFunc);
	// Track bytes-downloaded/total so the GUI can show a progress bar on slow
	// fetches (e.g. large Zophar zips). NOPROGRESS must be off for the callback
	// to fire.
	curl_easy_setopt(curl, CURLOPT_XFERINFODATA, this);
	curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, progressFunc);
	curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
	curl_multi_add_handle(curlm, curl);

	{
		std::lock_guard<std::mutex> lock(Web::sm);
		Web::runningWebJobs += 1;
	}
}

size_t WebJob::writeFunc(void *ptr, size_t size, size_t x, void *userdata) {
	WebJob* job = static_cast<WebJob*>(userdata);
	size *= x;
	
	if(job->stopped) {
		LOGD("Job stopped");
		if(job->targetFile.exists())
			job->targetFile.remove();
		return -1;
	}

	// libcurl calls this from its own C stack, so nothing here may throw: an
	// escaping exception unwinds through C frames straight into std::terminate
	// and aborts the whole app. File::write() DOES throw (io_exception) when the
	// cache file cannot be opened, which killed ChipMachine mid-shuffle. Contain
	// it and report a short write instead -- curl turns that into
	// CURLE_WRITE_ERROR, failing just this transfer, so the song reports an error
	// and playback carries on.
	try {
		if(job->targetFile) {
			job->targetFile.write(static_cast<uint8_t*>(ptr), size);
		} else if(job->streamCb) {
			if(!job->streamCb(*job, static_cast<uint8_t*>(ptr), size))
				return -1;
		} else {
			unsigned pos = job->data.size();
			job->data.resize(pos + size);
			memcpy(&job->data[pos], ptr, size);
		}
	} catch(const std::exception &e) {
		// The message carries the offending path and the errno reason (see
		// File::open) -- the only record of WHY, since the throw used to abort
		// before anything could report it.
		LOGE("Download write failed, aborting transfer of '%s': %s", job->url,
		     e.what());
		return 0;
	} catch(...) {
		LOGE("Download write failed, aborting transfer of '%s': unknown error",
		     job->url);
		return 0;
	}
	return size;
}

int WebJob::progressFunc(void *userdata, curl_off_t dltotal, curl_off_t dlnow,
                         curl_off_t /*ultotal*/, curl_off_t /*ulnow*/) {
	WebJob* job = static_cast<WebJob*>(userdata);
	job->dlTotal = static_cast<int64_t>(dltotal);
	job->dlNow = static_cast<int64_t>(dlnow);
	// Returning non-zero aborts the transfer; honour a stop() requested from
	// another thread so a cancelled download tears down promptly even if no
	// write callback is currently firing.
	return job->stopped ? 1 : 0;
}

size_t WebJob::headerFunc(char *text, size_t size, size_t n, void *userdata) {
	WebJob* job = static_cast<WebJob*>(userdata);
	size_t total = size * n;

	// IMPORTANT: curl's header buffer is NOT NUL-terminated and must be treated
	// as READ-ONLY. The previous code wrote a NUL into it (text[sz] = 0) to be
	// able to strstr() it -- two heap bugs:
	//   1. a 0-length header callback (total == 0) made sz == -1, so text[-1] = 0
	//      wrote one byte BEFORE curl's buffer (heap underflow);
	//   2. even in-bounds, mutating the buffer corrupts curl's internal HTTP/2
	//      (nghttp2/HPACK) header storage, which shares that memory.
	// Both are small, layout-dependent stray writes that poison the heap and
	// later crash in an unrelated allocation (typically font make_text). Copy
	// the header into a std::string and parse THAT; never touch curl's buffer.
	std::string line(text, total);
	while(!line.empty() && (line.back() == '\n' || line.back() == '\r'))
		line.pop_back();

	std::string name, val;
	auto colon = line.find(':');
	if(colon == std::string::npos) {
		name = line;
	} else {
		name = line.substr(0, colon);
		size_t pos = colon + 1;
		if(pos < line.size() && line[pos] == ' ') pos++;
		val = line.substr(pos);
		job->headers[name] = val;
	}

	LOGV("HEADER: '%s = %s'", name, val);
	if(name == "Content-Length") {
		try { job->cLength = std::stol(val); } catch(...) {}
	} else
	if(name== "Location") {
		std::string newUrl = val;
		LOGD("Redirecting to %s", newUrl);
		std::string newTarget = utils::urlencode(newUrl, ":/\\?;");
#ifndef _WIN32
		symlink(newTarget.c_str(), job->targetFile.getName().c_str());
#endif
	}

	return total;
}

void WebJob::finish() {
	isDone = true;
	auto rc = code();
	LOGD("CODE %d", rc);

	if(targetFile) {
		// A cancelled job (stop() via RemoteLoader::cancel, or ~Web teardown)
		// is treated like a failed transfer: drop whatever was written so a
		// partial/aborted body never lingers in the cache and gets served as a
		// truncated tune on the next selection. writeFunc removes the file when
		// IT sees the stop, but an abort raised from progressFunc happens before
		// any write, so the partial file must also be dropped here.
		if(stopped || (rc != 200 && rc != 226)) {
			if(targetFile.exists())
				targetFile.remove();
			targetFile = utils::File();
		} else {
			targetFile.close();
			if(orgFile) {
				if(targetFile.exists()) {
					if(orgFile.exists())
						orgFile.remove();
					targetFile.rename(orgFile);
				} else {
					// Job was cancelled mid-transfer: writeFunc already removed
					// the temp .download file even though the HTTP status was
					// 200. There is nothing to rename, and renaming a missing
					// file throws io_exception, which is uncaught on this worker
					// thread and terminates the whole app. Just clean up.
					targetFile = utils::File();
				}
			}
		}
	}
	if(streamCb)
		streamCb(*this, nullptr, 0);
	// A cancelled whole-file download must NOT invoke its completion callback.
	// By the time the abort lands, the player has already switched to another
	// song; firing the stale callback reaches into MusicPlayerList and forces
	// THAT song's Loading state to Error while underflowing its outstanding-file
	// counter -- which is exactly why every track selected after an interrupted
	// download failed to play (the whole cache was fine; the state was poisoned).
	// The abandoned song simply drops here. (Streaming jobs use streamCb, not
	// call_handler, so their teardown above is unaffected.)
	if(!stopped)
		call_handler();
	targetFile = utils::File();
	// NOTE: the curl easy handle is freed by the caller (Web::run) under m,
	// right after this returns. Do NOT curl_easy_cleanup() here: finish() runs
	// outside m so the user callback above can re-enter Web, but freeing the
	// handle must be serialized against curl_multi_add_handle()/perform().
}

void WebJob::destroy() {
	if(curl) {
		curl_easy_cleanup(curl);
		std::lock_guard<std::mutex> lock(Web::sm);
		Web::runningWebJobs -= 1;
	}
	curl = nullptr;
}

} // namespace webutils


