#!/usr/bin/env python3
"""Edge-counting frequency measurement -- accurate for square waves."""
import sys, numpy as np
SR = 44100

def load(p):
    return np.frombuffer(open(p, 'rb').read(), dtype='<i2').astype(np.float64)

def measure(p, skip=0.5):
    x = load(p)
    if x.size <= int(SR * skip) + 1024:
        return None
    x = x[int(SR * skip):]
    x = x - x.mean()
    peak = float(np.abs(x).max())
    rms = float(np.sqrt((x * x).mean()))
    if peak < 8:
        return dict(f=0.0, peak=peak, rms=rms, dur=x.size / SR)
    # hysteresis around zero to reject filter ringing
    hi, lo = 0.25 * peak, -0.25 * peak
    state = 0
    crossings = 0
    first = last = None
    for i, v in enumerate(x):
        if state <= 0 and v > hi:
            state = 1; crossings += 1
            if first is None: first = i
            last = i
        elif state >= 0 and v < lo:
            state = -1; crossings += 1
            if first is None: first = i
            last = i
    if first is None or last <= first or crossings < 3:
        return dict(f=0.0, peak=peak, rms=rms, dur=x.size / SR)
    dur = (last - first) / SR
    f = (crossings - 1) / 2.0 / dur
    return dict(f=f, peak=peak, rms=rms, dur=dur)

if __name__ == '__main__':
    for p in sys.argv[1:]:
        r = measure(p)
        if r is None:
            print("%-30s <too short>" % p.split('/')[-1]); continue
        print("%-30s f=%10.2f Hz  peak=%6.0f  rms=%7.1f" %
              (p.split('/')[-1], r['f'], r['peak'], r['rms']))
