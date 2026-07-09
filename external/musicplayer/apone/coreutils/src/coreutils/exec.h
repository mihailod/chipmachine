#pragma once

#include "log.h"
#include "utils.h"

#include <cstdint>
#include <string>
#include <vector>
#include <iostream>
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

namespace utils {

#ifdef _WIN32
typedef void* HANDLE;
#endif

struct ExecPipe
{
    ExecPipe() = default;
    explicit ExecPipe(const std::string& cmd);
    ~ExecPipe();

    ExecPipe(ExecPipe&& other) noexcept;
    ExecPipe(const ExecPipe& other) = delete;
    ExecPipe& operator=(const ExecPipe& other) = delete;
    ExecPipe& operator=(ExecPipe&& other) noexcept;

    bool hasEnded();
    void Kill();
    int read(uint8_t* target, int size);
    int write(uint8_t* source, int size);
    // Put the read (stdout) end into non-blocking mode so read() returns -1 on
    // EAGAIN instead of blocking. Used by the streaming player so the audio
    // thread can poll for "buffering" without stalling.
    void setReadNonBlocking();
    // Close the write (stdin) end so the child sees EOF on stdin, flushes and
    // exits. Read end stays open to drain remaining output.
    void closeWrite();
    explicit operator std::string();

#ifdef _WIN32
    HANDLE hPipeRead{nullptr};
    HANDLE hPipeWrite{nullptr};
    HANDLE hProcess{nullptr};
#else
    pid_t pid = -1;
    int outfd{-1};
    int infd{-1};
#endif
};

#ifdef _WIN32
inline ExecPipe::ExecPipe(const std::string& cmd) {}
inline int ExecPipe::read(uint8_t* target, int size) { return -1; }
inline int ExecPipe::write(uint8_t* source, int size) { return 0; }
ExecPipe::ExecPipe(ExecPipe&& other) noexcept {}
ExecPipe& ExecPipe::operator=(ExecPipe&& other) noexcept { return *this; }
inline bool ExecPipe::hasEnded() { return true; }
inline ExecPipe::~ExecPipe() {}
inline void ExecPipe::Kill() {}
inline void ExecPipe::setReadNonBlocking() {}
inline void ExecPipe::closeWrite() {}
#else

#    include <fcntl.h>
#    include <signal.h>
#    include <sys/types.h>
#    include <sys/wait.h>
#    include <unistd.h>
#    include <cerrno>
#    include <cstdio>

__attribute__((noinline)) inline pid_t popen2_simple(const char* command, int* infp, int* outfp)
{
    enum { READ, WRITE };
    int p_stdin[2], p_stdout[2];
    pid_t pid;

    write(STDERR_FILENO, "[DEBUG] Forking shell command execution...\n", 43);

    if (pipe(p_stdin) != 0 || pipe(p_stdout) != 0) return -1;

    // Mark all pipe ends close-on-exec so they cannot leak into unrelated child
    // processes spawned elsewhere. A leaked copy of an output pipe's write end
    // would keep the reader from ever seeing EOF. The child below dup2()s the
    // ends it needs onto stdin/stdout, which clears O_CLOEXEC on those targets,
    // so the spawned command still gets its standard streams.
    for (int fd : {p_stdin[0], p_stdin[1], p_stdout[0], p_stdout[1]}) {
        fcntl(fd, F_SETFD, fcntl(fd, F_GETFD) | FD_CLOEXEC);
    }

    pid = fork();
    if (pid < 0) return pid;

    if (pid == 0) {
        dup2(p_stdin[READ], STDIN_FILENO);
        dup2(p_stdout[WRITE], STDOUT_FILENO);
        
        dup2(STDOUT_FILENO, STDERR_FILENO);

        close(p_stdin[READ]);
        close(p_stdin[WRITE]);
        close(p_stdout[READ]);
        close(p_stdout[WRITE]);

        execl("/bin/sh", "sh", "-c", command, NULL);
        perror("execl failed");
        _exit(1);
    }

    close(p_stdin[READ]);
    close(p_stdout[WRITE]);

    if (infp == nullptr) close(p_stdin[WRITE]); else *infp = p_stdin[WRITE];
    if (outfp == nullptr) close(p_stdout[READ]); else *outfp = p_stdout[READ];

    return pid;
}

__attribute__((noinline)) inline ExecPipe::ExecPipe(const std::string& cmd)
{
    std::fprintf(stderr, "[DEBUG] ExecPipe Constructor Command Line: '%s'\n", cmd.c_str());
    std::fflush(stderr);

    pid = popen2_simple(cmd.c_str(), &infd, &outfd);

    if (outfd >= 0) {
        int flags = fcntl(outfd, F_GETFL, 0);
        fcntl(outfd, F_SETFL, flags & ~O_NONBLOCK);
    }

    // Writing to the child's stdin (infd) after the child has exited would raise
    // SIGPIPE, whose default action silently kills the whole app (no crash
    // report). This is exactly what happened when a track switch tore down the
    // ffmpeg stream player while the feeder thread was still mid-write to a dying
    // ffmpeg. Ask the kernel to fail those writes with EPIPE instead of a signal;
    // the feeder already treats a <=0 write as "child gone" and stops cleanly.
#ifdef F_SETNOSIGPIPE
    if (infd >= 0) fcntl(infd, F_SETNOSIGPIPE, 1);
    if (outfd >= 0) fcntl(outfd, F_SETNOSIGPIPE, 1);
#endif
}

__attribute__((noinline)) inline ExecPipe::~ExecPipe() { Kill(); }

ExecPipe::ExecPipe(ExecPipe&& other) noexcept
{
    pid = other.pid;
    outfd = other.outfd;
    infd = other.infd;
    other.pid = -1;
    other.outfd = -1;
    other.infd = -1;
}

inline ExecPipe& ExecPipe::operator=(ExecPipe&& other) noexcept
{
    if (this != &other) {
        Kill();
        pid = other.pid;
        outfd = other.outfd;
        infd = other.infd;
        other.pid = -1;
        other.outfd = -1;
        other.infd = -1;
    }
    return *this;
}

__attribute__((noinline)) inline void ExecPipe::Kill()
{
    if (pid != -1) {
        int result = 0;
        if (waitpid(pid, &result, WNOHANG) == 0) {
            kill(pid, SIGKILL);
            waitpid(pid, &result, 0);
        }
        pid = -1;
    }
    if (outfd >= 0) { close(outfd); outfd = -1; }
    if (infd >= 0) { close(infd); infd = -1; }
}

__attribute__((noinline)) inline int ExecPipe::read(uint8_t* target, int size)
{
    if (outfd < 0) return -2;
    
    while (true) {
        int rc = ::read(outfd, target, size);
        if (rc >= 0) return rc;
        if (errno == EINTR) continue;
        // Only happens when the fd is non-blocking (setReadNonBlocking): tell the
        // caller "no data yet" rather than conflating it with a real error/EOF.
        if (errno == EAGAIN || errno == EWOULDBLOCK) return -1;
        return -2;
    }
}

inline int ExecPipe::write(uint8_t* source, int size)
{
    if (infd < 0) return -2;
    return ::write(infd, source, size);
}

inline void ExecPipe::setReadNonBlocking()
{
    if (outfd >= 0) {
        int flags = fcntl(outfd, F_GETFL, 0);
        fcntl(outfd, F_SETFL, flags | O_NONBLOCK);
    }
}

inline void ExecPipe::closeWrite()
{
    if (infd >= 0) {
        close(infd);
        infd = -1;
    }
}

__attribute__((noinline)) inline bool ExecPipe::hasEnded()
{
    if (pid == -1) return true;
    int rc = 0;
    pid_t res = waitpid(pid, &rc, WNOHANG);
    if (res == pid) {
        pid = -1;
        return true;
    } else if (res < 0 && errno == ECHILD) {
        pid = -1;
        return true;
    }
    return false;
}

#endif

inline ExecPipe::operator std::string()
{
    char buf[1024];
    std::string result;
    bool ended = false;
    while (true) {
        int sz = read(reinterpret_cast<uint8_t*>(&buf[0]), sizeof(buf));
        if (sz > 0) {
            result += std::string(buf, 0, sz);
        } else if (sz != -1 || ended)
            return result;
        ended = hasEnded();
        sleepms(100);
    }
    return result;
}

inline int shellExec(const std::string& cmd, const std::string& binDir)
{
    return system((binDir + "/" + cmd).c_str());
}

inline ExecPipe execPipe(const std::string& cmd)
{
    return ExecPipe{cmd};
}

} // namespace utils
