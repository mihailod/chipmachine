#!/usr/bin/env python3
"""Verify ted_sound.c against the specification measured from the chip's
register interface. Every expectation here came from characterise.py / lfsr /
damode runs against the engine being replaced."""
import subprocess, sys
sys.path.insert(0, '.')
from measure import measure
import numpy as np

SR, R = 44100, 221681.0
fails = []

def run(args, out='v.raw'):
    with open(out, 'wb') as fh:
        subprocess.run(['./ted_test'] + [str(a) for a in args], stdout=fh)
    return out

def check(label, got, want, tol, unit=''):
    ok = abs(got - want) <= tol
    if not ok: fails.append(label)
    print("   %-42s got %10.3f  want %10.3f %-3s  %s"
          % (label, got, want, unit, 'ok' if ok else '** FAIL **'))

print("1. frequency law  f = clock / (2 * ((1022-N) & 0x3FF))")
for N in (0, 256, 512, 768, 896, 960, 992, 1006, 1008, 1014, 1023):
    D = (1022 - N) & 0x3FF
    want = R / (2.0 * D)
    r = measure(run(['tone', N, '18', 2.0]), skip=0.2)
    tol = max(1.0, want * 0.004)
    check("N=%-4d D=%-4d" % (N, D), r['f'], want, tol, 'Hz')

print("\n2. N=$3FE (1022) locks the source -- silence")
r = measure(run(['tone', 1022, '18', 1.0]), skip=0.2)
check("N=1022 peak", r['peak'], 0.0, 1.0)

print("\n3. volume is linear to 8 and flat above it")
rms = {}
for v in range(16):
    r = measure(run(['tone', 896, '%02X' % (0x10 | v), 1.5]), skip=0.2)
    rms[v] = r['rms']
step = rms[8] / 8.0
for v in range(1, 9):
    check("vol=%d rms" % v, rms[v], step * v, step * 0.06)
for v in range(9, 16):
    check("vol=%d clamps to 8" % v, rms[v], rms[8], max(1.0, rms[8] * 0.001))

print("\n4. enable bits, and two channels sum")
r0 = measure(run(['tone', 896, '08', 1.0]), skip=0.2)
check("no enable bits -> silence", r0['peak'], 0.0, 1.0)
r1 = measure(run(['tone2', 896, 1022, '18', 1.5]), skip=0.2)
r2 = measure(run(['tone2', 1022, 512, '28', 1.5]), skip=0.2)
rb = measure(run(['tone2', 896, 512, '38', 1.5]), skip=0.2)
print("   ch1 only peak=%.0f, ch2 only peak=%.0f, both peak=%.0f" % (r1['peak'], r2['peak'], rb['peak']))
check("both ~= sum of the two", rb['peak'], r1['peak'] + r2['peak'], (r1['peak'] + r2['peak']) * 0.10)

print("\n5. noise: register period 255, ~50%% duty, steps once per reload")
seq = subprocess.run(['./ted_test', 'seq'], capture_output=True, text=True).stdout.strip()
per = next(p for p in range(2, 400) if all(seq[i] == seq[i + p] for i in range(len(seq) - p)))
check("noise register period", per, 255, 0)
ones = seq[:255].count('1')
check("ones in one period", ones, 127, 2)
# the audio repeat should be 255 * D cycles
N = 0; D = (1022 - N) & 0x3FF
p = run(['noise', N, 6.0])
x = np.frombuffer(open(p, 'rb').read(), dtype='<i2').astype(float)[int(SR * 0.2):]
x = x - x.mean()
n = 1 << int(np.floor(np.log2(x.size)))
f = np.fft.rfft(x[:n]); ac = np.fft.irfft(f * np.conj(f))[:n // 2]
# The FFT autocorrelation is circular, so a lag only overlaps n-lag samples --
# normalise for that, and start the search past the lag-0 triangle (which is as
# wide as one noise step) so it cannot masquerade as the repeat.
step = D / R * SR
lags = np.arange(len(ac))
norm = ac / np.maximum(n - lags, 1) * n
lo = int(3 * step)
k = lo + int(np.argmax(norm[lo:int(SR * 2)]))
check("noise repeat (samples)", k, 255.0 * step, 0.02 * 255.0 * step)
check("  and it is an isolated peak", norm[k] / ac[0], 1.0, 0.10)

print("\n6. D/A mode: volume writes reach the output, gated by the enable bits")
half = 1400   # TED cycles per half period -> ~79 Hz square
r = measure(run(['da', 'B8', half, 2.0]), skip=0.3)
want = R / (2.0 * half)
check("bit7=1, ch1+ch2 enabled, f", r['f'], want, want * 0.05, 'Hz')
r = measure(run(['da', '88', half, 2.0]), skip=0.3)
check("bit7=1, no enable bits -> silence", r['peak'], 0.0, 1.0)
r = measure(run(['da', '38', half, 2.0]), skip=0.3)
check("bit7=0 with counters locked, f", r['f'], want, want * 0.05, 'Hz')

print()
if fails:
    print("FAILED: %d check(s): %s" % (len(fails), ', '.join(fails)))
    sys.exit(1)
print("all checks passed")
