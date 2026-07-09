/*
 * compat.h -- minimal ScummVM compatibility shim for the vendored MaxTrax /
 * Paula sources (audio/mods/maxtrax.{h,cpp}, audio/mods/paula.{h,cpp}).
 *
 * Provides just enough of common/scummsys.h, common/util.h, common/frac.h,
 * common/mutex.h, common/stream.h, common/debug.h and common/textconsole.h
 * for the two files to compile standalone, outside the ScummVM tree.
 *
 * The original sources remain under GPLv3 (ScummVM); this shim is glue.
 */
#ifndef MAXTRAX_COMPAT_H
#define MAXTRAX_COMPAT_H

#include <cstdint>
#include <cstring>
#include <cstdio>
#include <cstdarg>
#include <cassert>
#include <cmath>

// ---------------------------------------------------------------------------
// scummsys.h: base integer types (ScummVM puts these in the global namespace)
// ---------------------------------------------------------------------------
typedef uint8_t  byte;
typedef uint8_t  uint8;
typedef int8_t   int8;
typedef uint16_t uint16;
typedef int16_t  int16;
typedef uint32_t uint32;
typedef int32_t  int32;
typedef unsigned int uint;

#define ARRAYSIZE(x) ((int)(sizeof(x) / sizeof((x)[0])))
#define ARRAYEND(x)  ((x) + ARRAYSIZE(x))

// ---------------------------------------------------------------------------
// util.h: MIN / MAX / CLIP
// ---------------------------------------------------------------------------
template<typename T> inline T MIN(T a, T b) { return (a < b) ? a : b; }
template<typename T> inline T MAX(T a, T b) { return (a > b) ? a : b; }
template<typename T> inline T CLIP(T v, T lo, T hi) {
	return (v < lo) ? lo : ((v > hi) ? hi : v);
}

// ---------------------------------------------------------------------------
// frac.h: 16.16 fixed point used by the Paula mixer
// ---------------------------------------------------------------------------
typedef int32 frac_t;
enum {
	FRAC_BITS    = 16,
	FRAC_ONE     = 1 << FRAC_BITS,
	FRAC_HALF    = 1 << (FRAC_BITS - 1),
	FRAC_LO_MASK = FRAC_ONE - 1,
	FRAC_HI_MASK = ~FRAC_LO_MASK
};

inline frac_t doubleToFrac(double value) { return (frac_t)(value * FRAC_ONE); }
inline int    fracToInt(frac_t value)    { return value >> FRAC_BITS; }

// ---------------------------------------------------------------------------
// debug.h / textconsole.h: diagnostics (no-ops, warnings to stderr)
// ---------------------------------------------------------------------------
#define gDebugLevel 0

inline void debug(const char *, ...) {}
inline void debug(int, const char *, ...) {}

inline void warning(const char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	fputs("[MaxTrax] ", stderr);
	vfprintf(stderr, fmt, ap);
	fputc('\n', stderr);
	va_end(ap);
}

namespace Common {

// -------------------------------------------------------------------------
// mutex.h: single-threaded decode path -- locks are no-ops
// -------------------------------------------------------------------------
class Mutex {
public:
	void lock() {}
	void unlock() {}
};

class StackLock {
	Mutex &_mutex;
public:
	explicit StackLock(Mutex &m) : _mutex(m) { _mutex.lock(); }
	~StackLock() { _mutex.unlock(); }
};

// -------------------------------------------------------------------------
// stream.h: concrete big-endian memory reader (only what load() touches)
// -------------------------------------------------------------------------
class SeekableReadStream {
	const byte *_data;
	uint32      _size;
	uint32      _pos;
	bool        _eos;

	byte fetch() {
		if (_pos >= _size) { _eos = true; return 0; }
		return _data[_pos++];
	}

public:
	SeekableReadStream(const byte *data, uint32 size)
		: _data(data), _size(size), _pos(0), _eos(false) {}

	uint32 size() const { return _size; }
	uint32 pos()  const { return _pos; }
	bool   err()  const { return false; }
	bool   eos()  const { return _eos; }

	byte readByte() { return fetch(); }

	uint16 readUint16BE() {
		const byte hi = fetch();
		const byte lo = fetch();
		return (uint16)((hi << 8) | lo);
	}
	int16  readSint16BE() { return (int16)readUint16BE(); }

	uint32 readUint32BE() {
		const uint32 b0 = fetch();
		const uint32 b1 = fetch();
		const uint32 b2 = fetch();
		const uint32 b3 = fetch();
		return (b0 << 24) | (b1 << 16) | (b2 << 8) | b3;
	}

	uint32 read(void *dst, uint32 len) {
		// Used only for bulk sample-PCM reads. Many modland MaxTrax rips
		// truncate the final instrument's sustain/loop tail, so the declared
		// sample length runs a little past EOF. Rather than fail the whole
		// load (as the strict eos() check would), zero-fill the missing tail
		// -- the absent bytes just play as silence, like on real hardware.
		// Structural fields are read via readByte()/readUint*(), which stay
		// strict, so genuinely corrupt headers are still rejected.
		uint32 avail = (_pos < _size) ? (_size - _pos) : 0;
		uint32 got = (len <= avail) ? len : avail;
		memcpy(dst, _data + _pos, got);
		if (got < len) {
			memset(static_cast<byte*>(dst) + got, 0, len - got);
		}
		_pos += got;
		return len;
	}

	void skip(uint32 len) {
		_pos += len;
		if (_pos > _size) { _pos = _size; _eos = true; }
	}
};

} // namespace Common

#endif // MAXTRAX_COMPAT_H
