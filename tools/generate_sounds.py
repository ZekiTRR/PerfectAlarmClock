"""Synthesises the built-in notification sounds shipped with the app."""
import math
import os
import random
import struct
import wave

SR = 44100
OUT = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "assets", "sounds")


def write_wav(name, samples):
    peak = max(1e-9, max(abs(s) for s in samples))
    norm = 0.82 / peak if peak > 0.0 else 1.0
    data = b"".join(struct.pack("<h", int(max(-1.0, min(1.0, s * norm)) * 32767)) for s in samples)
    with wave.open(os.path.join(OUT, name), "wb") as f:
        f.setnchannels(1)
        f.setsampwidth(2)
        f.setframerate(SR)
        f.writeframes(data)


def bell(freq, duration, decay=4.0, amp=0.6):
    n = int(SR * duration)
    out = [0.0] * n
    partials = [(1.0, 1.0), (2.01, 0.42), (2.99, 0.24), (4.2, 0.12), (5.43, 0.07)]
    for i in range(n):
        t = i / SR
        env = math.exp(-decay * t) * (1.0 - math.exp(-400.0 * t))
        value = 0.0
        for ratio, gain in partials:
            value += math.sin(2.0 * math.pi * freq * ratio * t) * gain
        out[i] += value * env * amp
    return out


def mix(dst, src, offset):
    start = int(offset * SR)
    if start >= len(dst):
        return
    for i, value in enumerate(src):
        if start + i >= len(dst):
            break
        dst[start + i] += value


def chime():
    duration = 4.6
    out = [0.0] * int(SR * duration)
    for t, freq in [(0.00, 659.25), (0.42, 987.77), (0.84, 783.99), (1.55, 659.25), (1.97, 987.77), (2.39, 1046.50)]:
        mix(out, bell(freq, 2.6, decay=2.1, amp=0.30), t)
    for i in range(len(out)):
        out[i] *= min(1.0, (len(out) - i) / (SR * 0.35))
    return out


def gentle():
    duration = 5.0
    out = [0.0] * int(SR * duration)
    notes = [(0.0, 523.25), (0.7, 659.25), (1.4, 783.99), (2.4, 659.25), (3.1, 783.99), (3.8, 1046.5)]
    for t, freq in notes:
        for ratio, gain in [(1.0, 1.0), (2.0, 0.18), (3.0, 0.05)]:
            start = int(t * SR)
            length = int(2.2 * SR)
            for i in range(length):
                if start + i >= len(out):
                    break
                x = i / SR
                env = min(1.0, x / 0.06) * math.exp(-1.6 * x)
                out[start + i] += math.sin(2.0 * math.pi * freq * ratio * x) * env * gain * 0.14
    return out


def classic():
    duration = 3.0
    out = [0.0] * int(SR * duration)
    for start, length in [(0.0, 0.38), (0.52, 0.38), (1.04, 0.38), (1.56, 0.38), (2.08, 0.38)]:
        n = int(length * SR)
        for i in range(n):
            x = i / SR
            env = min(1.0, x / 0.01) * min(1.0, (length - x) / 0.04)
            tone = math.sin(2.0 * math.pi * 880.0 * x) + 0.35 * math.sin(2.0 * math.pi * 1760.0 * x)
            idx = int(start * SR) + i
            if idx < len(out):
                out[idx] += tone * env * 0.22
    return out


def digital():
    duration = 2.4
    out = [0.0] * int(SR * duration)
    random.seed(7)
    for start in (0.0, 0.30, 0.60, 1.25, 1.55, 1.85):
        n = int(0.14 * SR)
        for i in range(n):
            x = i / SR
            env = min(1.0, x / 0.006) * min(1.0, (0.14 - x) / 0.02)
            idx = int(start * SR) + i
            if idx < len(out):
                out[idx] += math.sin(2.0 * math.pi * 1318.5 * x) * env * 0.28
    return out


def main():
    os.makedirs(OUT, exist_ok=True)
    write_wav("chime.wav", chime())
    write_wav("gentle.wav", gentle())
    write_wav("classic.wav", classic())
    write_wav("digital.wav", digital())


if __name__ == "__main__":
    main()
