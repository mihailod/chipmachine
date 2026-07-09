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
		slist = curl_slist_append(slist, "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/avif,image/webp,*/*;q=0.8");
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
	curl_easy_setopt(curl, CURLOPT_IGNORE_CONTENT_LENGTH, 1L);

	if (isStream) {
		// SHOUTcast servers answer with "ICY 200 OK" over HTTP/1.0.
		curl_easy_setopt(curl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_0);
		curl_easy_setopt(curl, CURLOPT_HTTP200ALIASES, alias_list.get());
	} else {
		// Browsers speak HTTP/1.1+; HTTP/1.0 alone is a tell. Let curl negotiate
		// up to HTTP/2 when the server offers it (keeps e.g. web.archive.org
		// screenshots fast).
		curl_easy_setopt(curl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_2TLS);
		// ...but FORBID connection reuse. libcurl's HTTP/2 connection-cache
		// pruning (prune_dead_connections -> http2_data_done -> Curl_bufq_free)
		// corrupts the heap and crashes (EXC_BAD_ACCESS) when a STALE cached HTTP/2
		// connection is pruned as a new transfer starts -- reliably hit by an
		// HTTPS fetch (zxart.ee, fi.zophar.net, ...) followed by an FTP modland
		// fetch on the same multi handle. Closing each connection after its
		// transfer (no caching) means there is never a stale connection to prune,
		// so the buggy path can't fire -- on ANY host, not just one we special-case.
		// (The proper cure is a libcurl bump; this is the safe mitigation that
		// keeps HTTP/2 working.)
		// HTTP(S) ONLY: on FTP, FORBID_REUSE makes curl close the control
		// connection after each file, so the last response code captured is the
		// QUIT reply 221 instead of the transfer's 226 -> WebJob::finish() treats
		// !=200/226 as failure and DELETES the file, breaking every modland FTP
		// download. FTP has no HTTP/2 cache to corrupt, so it doesn't need this.
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
	return size;
}

size_t WebJob::headerFunc(char *text, size_t size, size_t n, void *userdata) {
	WebJob* job = static_cast<WebJob*>(userdata);
	size *= n;
	int sz = size-1;
	while(sz > 0 && (text[sz-1] == '\n' || text[sz-1] == '\r'))
		sz--;
	text[sz] = 0;
	char *split = strstr(text, ":");
	std::string name, val;
	if(!split)	
		name = std::string(text, sz);
	else {
		int pos = split-text;
		name = std::string(text, 0, pos);
		pos++;
		if(text[pos] == ' ') pos++;
		val = std::string(text, pos, sz-pos);
		job->headers[name] = val;
	}	

	LOGV("HEADER: '%s = %s'", name, val);
	if(name == "Content-Length") {
		job->cLength = std::stol(val);
	} else
	if(name== "Location") {
		std::string newUrl = val;
		LOGD("Redirecting to %s", newUrl);
		std::string newTarget = utils::urlencode(newUrl, ":/\\?;");
#ifndef _WIN32
		symlink(newTarget.c_str(), job->targetFile.getName().c_str());
#endif
	}

	return size;
}

void WebJob::finish() {
	isDone = true;
	auto rc = code();
	LOGD("CODE %d", rc);

	if(targetFile) {
		if(rc != 200 && rc != 226) {
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


