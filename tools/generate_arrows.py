"""Generates tiny high-DPI-friendly arrow PNGs used by Office-style spin boxes."""
import os
import struct
import zlib

OUT = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "assets", "icons")
W, H = 18, 11
SS = 4


def triangle(up):
    w, h = W * SS, H * SS
    alpha = [[0] * w for _ in range(h)]
    margin_x = 3 * SS
    margin_y = 2 * SS
    for y in range(h):
        for x in range(w):
            if x < margin_x or x >= w - margin_x:
                continue
            if up:
                yy = y - margin_y
            else:
                yy = h - 1 - margin_y - y
            if yy < 0:
                continue
            width = (w - 2 * margin_x) * (yy / (h - 2 * margin_y - 1))
            if abs(x - w / 2) <= width / 2:
                alpha[y][x] = 255
    out = [[0] * W for _ in range(H)]
    for y in range(H):
        for x in range(W):
            total = 0
            for dy in range(SS):
                for dx in range(SS):
                    total += alpha[y * SS + dy][x * SS + dx]
            out[y][x] = total // (SS * SS)
    return out


def write_png(path, alpha, color):
    raw = bytearray()
    for row in alpha:
        raw.append(0)
        for a in row:
            raw.extend((color[0], color[1], color[2], a))

    def chunk(tag, data):
        return (
            struct.pack(">I", len(data))
            + tag
            + data
            + struct.pack(">I", zlib.crc32(tag + data) & 0xFFFFFFFF)
        )

    png = b"\x89PNG\r\n\x1a\n"
    png += chunk(b"IHDR", struct.pack(">IIBBBBB", W, H, 8, 6, 0, 0, 0))
    png += chunk(b"IDAT", zlib.compress(bytes(raw), 9))
    png += chunk(b"IEND", b"")
    with open(path, "wb") as f:
        f.write(png)


def main():
    os.makedirs(OUT, exist_ok=True)
    up = triangle(True)
    down = up[::-1]
    for suffix, color in (("light", (77, 94, 111)), ("dark", (218, 226, 234))):
        write_png(os.path.join(OUT, f"arrow_up_{suffix}.png"), up, color)
        write_png(os.path.join(OUT, f"arrow_down_{suffix}.png"), down, color)


if __name__ == "__main__":
    main()
