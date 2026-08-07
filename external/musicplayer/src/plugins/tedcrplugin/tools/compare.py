#!/usr/bin/env python3
"""Differential comparison: render each .prg through the tedplay engine and
through the clean-room machine, align them (the engine spends ~1s booting and
running BASIC's RUN first), and score how alike they are.

Score is the mean cosine similarity of log-magnitude spectra over time, which is
robust to phase and level differences but not to playing the wrong notes.
"""
import subprocess, sys, os, glob
import numpy as np

SR = 44100
FRAME = 1024
HOP = 512

def render(cmd, path, secs):
    r = subprocess.run(cmd + [path, str(secs)], capture_output=True)
    return np.frombuffer(r.stdout, dtype='<i2').astype(np.float64)

def spectra(x):
    n = 1 + (x.size - FRAME) // HOP
    if n < 4:
        return None
    w = np.hanning(FRAME)
    idx = np.arange(FRAME)[None, :] + HOP * np.arange(n)[:, None]
    S = np.abs(np.fft.rfft(x[idx] * w, axis=1))
    return np.log1p(S)

def envelope(x):
    n = x.size // HOP
    return np.sqrt((x[:n * HOP].reshape(n, HOP) ** 2).mean(axis=1)) + 1e-9

def best_lag(a, b, maxlag):
    """Lag (in hops) to apply to b so it lines up with a."""
    a = (a - a.mean()) / (a.std() + 1e-9)
    b = (b - b.mean()) / (b.std() + 1e-9)
    best, bl = -2.0, 0
    for l in range(-maxlag, maxlag + 1):
        if l >= 0:
            u, v = a[l:], b[:b.size - l] if l else b
        else:
            u, v = a[:a.size + l], b[-l:]
        m = min(u.size, v.size)
        if m < 40:
            continue
        c = float((u[:m] * v[:m]).mean())
        if c > best:
            best, bl = c, l
    return bl, best

def score(engine, core):
    ea, ca = envelope(engine), envelope(core)
    lag, envcorr = best_lag(ea, ca, maxlag=int(3.0 * SR / HOP))
    if lag >= 0:
        e = engine[lag * HOP:]
        c = core
    else:
        e = engine
        c = core[-lag * HOP:]
    m = min(e.size, c.size)
    e, c = e[:m], c[:m]
    Se, Sc = spectra(e), spectra(c)
    if Se is None or Sc is None:
        return 0.0, envcorr, 0
    k = min(len(Se), len(Sc))
    Se, Sc = Se[:k], Sc[:k]
    num = (Se * Sc).sum(axis=1)
    den = np.linalg.norm(Se, axis=1) * np.linalg.norm(Sc, axis=1) + 1e-9
    cos = num / den
    # frames where both are essentially silent carry no information
    live = (np.linalg.norm(Se, axis=1) > 1.0) & (np.linalg.norm(Sc, axis=1) > 1.0)
    if live.sum() < 10:
        return 0.0, envcorr, int(live.sum())
    return float(cos[live].mean()), envcorr, int(live.sum())

if __name__ == '__main__':
    files = sorted(glob.glob(sys.argv[1] + '/*'))
    secs = float(sys.argv[2]) if len(sys.argv) > 2 else 10.0
    rows = []
    for f in files:
        eng = render(['probe/render'], f, secs)
        cor = render(['./ted_run'], f, secs)
        ep = float(np.abs(eng).max()) if eng.size else 0.0
        cp = float(np.abs(cor).max()) if cor.size else 0.0
        if eng.size < SR or cor.size < SR:
            rows.append((f, 0.0, 0.0, ep, cp, 'short'))
            continue
        s, ec, live = score(eng, cor)
        rows.append((f, s, ec, ep, cp, ''))
        print("%-58s cos=%.3f env=%+.3f engpeak=%6.0f corepeak=%6.0f %s"
              % (os.path.basename(f)[:58], s, ec, ep, cp, rows[-1][5]), flush=True)

    good = [r for r in rows if r[1] >= 0.90]
    mid = [r for r in rows if 0.75 <= r[1] < 0.90]
    bad = [r for r in rows if r[1] < 0.75]
    print()
    print("files: %d   cos>=0.90: %d   0.75-0.90: %d   <0.75: %d"
          % (len(rows), len(good), len(mid), len(bad)))
    if rows:
        print("median cos: %.3f" % float(np.median([r[1] for r in rows])))
    print()
    print("worst 25:")
    for r in sorted(rows, key=lambda t: t[1])[:25]:
        print("   %-52s cos=%.3f engpeak=%6.0f corepeak=%6.0f %s"
              % (os.path.basename(r[0])[:52], r[1], r[3], r[4], r[5]))
