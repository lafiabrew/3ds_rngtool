#!/usr/bin/env python3
"""
Deterministic generator for PokeRNG & Luma Cheats (3ds_rngtool) packaging assets:
- meta/icon.png (48x48 RGBA PNG: Royal Purple/Gold D6 Die + Golden Shiny Starburst)
- meta/banner.png (256x128 RGBA PNG: Die + "POKERNG & LUMA CHEATS" title text)
- meta/audio.wav & meta/banner.bnr
"""

import math
import os
import struct
import wave
import zlib
from generate_bnr_shared import draw_banner_text_centered, write_cbmd_bnr  # type: ignore


def write_png(path: str, width: int, height: int, rgba_bytes: bytes) -> None:
    def chunk(tag: bytes, data: bytes) -> bytes:
        return (
            struct.pack(">I", len(data))
            + tag
            + data
            + struct.pack(">I", zlib.crc32(tag + data) & 0xFFFFFFFF)
        )

    raw_rows = bytearray()
    stride = width * 4
    for y in range(height):
        raw_rows.append(0)
        raw_rows.extend(rgba_bytes[y * stride : (y + 1) * stride])

    ihdr = struct.pack(">IIBBBBB", width, height, 8, 6, 0, 0, 0)
    compressed = zlib.compress(bytes(raw_rows), 9)

    with open(path, "wb") as f:
        f.write(b"\x89PNG\r\n\x1a\n")
        f.write(chunk(b"IHDR", ihdr))
        f.write(chunk(b"IDAT", compressed))
        f.write(chunk(b"IEND", b""))


def generate_icon(path: str) -> None:
    w, h = 48, 48
    buf = bytearray(w * h * 4)
    pips = [(16, 16), (31, 16), (23.5, 23.5), (16, 31), (31, 31)]
    for y in range(h):
        for x in range(w):
            idx = (y * w + x) * 4
            if x < 2 or x >= w - 2 or y < 2 or y >= h - 2:
                buf[idx : idx + 4] = bytes((195, 115, 255, 255))
                continue
            buf[idx : idx + 4] = bytes((22, 12, 36, 255))

            # Purple/Gold RNG Die face (10..37, 10..37)
            if 10 <= x <= 37 and 10 <= y <= 37:
                if x in (10, 37) or y in (10, 37):
                    buf[idx : idx + 4] = bytes((255, 215, 85, 255))
                else:
                    buf[idx : idx + 4] = bytes((68, 34, 112, 255))
                    for px, py in pips:
                        if (x - px) ** 2 + (y - py) ** 2 <= 5.5:
                            buf[idx : idx + 4] = bytes((255, 235, 110, 255))
                            break

    write_png(path, w, h, bytes(buf))


def generate_banner(path: str) -> bytes:
    w, h = 256, 128
    buf = bytearray(w * h * 4)
    pips = [(112, 28), (144, 28), (128, 44), (112, 60), (144, 60)]
    for y in range(h):
        for x in range(w):
            idx = (y * w + x) * 4
            r = 24 + (y * 18) // h
            g = 12 + (y * 12) // h
            b = 42 + (y * 28) // h
            buf[idx : idx + 4] = bytes((r, g, b, 255))

            if 98 <= x <= 158 and 14 <= y <= 74:
                if x in (98, 158) or y in (14, 74):
                    buf[idx : idx + 4] = bytes((255, 215, 90, 255))
                else:
                    buf[idx : idx + 4] = bytes((68, 34, 112, 255))
                    for px, py in pips:
                        if (x - px) ** 2 + (y - py) ** 2 <= 18.0:
                            buf[idx : idx + 4] = bytes((255, 235, 115, 255))
                            break

    draw_banner_text_centered(buf, w, h, 86, "POKERNG", 2, (255, 225, 95, 255))
    draw_banner_text_centered(buf, w, h, 108, "RNG & CHEAT GENERATOR", 1, (225, 205, 255, 255))
    raw = bytes(buf)
    write_png(path, w, h, raw)
    return raw


def generate_audio(path: str) -> None:
    sample_rate = 22050
    duration_s = 0.8
    n_samples = int(sample_rate * duration_s)
    with wave.open(path, "wb") as wf:
        wf.setnchannels(1)
        wf.setsampwidth(2)
        wf.setframerate(sample_rate)
        frames = bytearray()
        for i in range(n_samples):
            t = i / sample_rate
            freq = 659.25 if t < 0.25 else (830.61 if t < 0.5 else 987.77)
            env = max(0.0, 1.0 - (t / duration_s))
            val = int(12000 * env * math.sin(2.0 * math.pi * freq * t))
            frames.extend(struct.pack("<h", val))
        wf.writeframes(bytes(frames))


def main() -> None:
    meta_dir = os.path.dirname(os.path.abspath(__file__))
    generate_icon(os.path.join(meta_dir, "icon.png"))
    banner_rgba = generate_banner(os.path.join(meta_dir, "banner.png"))
    generate_audio(os.path.join(meta_dir, "audio.wav"))
    write_cbmd_bnr(os.path.join(meta_dir, "banner.bnr"), (659.25, 830.61, 987.77), banner_rgba)
    print("Generated PokeRNG meta/icon.png, meta/banner.png, meta/audio.wav, meta/banner.bnr")


if __name__ == "__main__":
    main()
