"""Generates a crisp white checkmark used by styled checkboxes."""
import os
import struct
import zlib

OUT = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "assets", "icons")
W, H = 16, 16
SS = 4


def point_segment_distance(px, py, ax, ay, bx, by):
    dx, dy = bx - ax, by - ay
    length = dx * dx + dy * dy
    if length == 0:
        return ((px - ax) ** 2 + (py - ay) ** 2) ** 0.5
    t = max(0.0, min(1.0, ((px - ax) * dx + (py - ay) * dy) / length))
    cx, cy = ax + t * dx, ay + t * dy
    return ((px - cx) ** 2 + (py - cy) ** 2) ** 0.5


def main():
    os.makedirs(OUT, exist_ok=True)
    alpha = [[0] * (W * SS) for _ in range(H * SS)]
    points = [((3.0, 8.3), (6.6, 12.0)), ((6.6, 12.0), (13.0, 4.6))]
    for y in range(H * SS):
        for x in range(W * SS):
            px = (x + 0.5) / SS
            py = (y + 0.5) / SS
            distance = min(point_segment_distance(px, py, a[0], a[1], b[0], b[1]) for a, b in points)
            alpha[y][x] = 255 if distance <= 1.05 else 0
    out = [[0] * W for _ in range(H)]
    for y in range(H):
        for x in range(W):
            total = 0
            for dy in range(SS):
                for dx in range(SS):
                    total += alpha[y * SS + dy][x * SS + dx]
            out[y][x] = total // (SS * SS)

    raw = bytearray()
    for row in out:
        raw.append(0)
        for a in row:
            raw.extend((255, 255, 255, a))

    def chunk(tag, data):
        return struct.pack(">I", len(data)) + tag + data + struct.pack(">I", zlib.crc32(tag + data) & 0xFFFFFFFF)

    png = b"\x89PNG\r\n\x1a\n"
    png += chunk(b"IHDR", struct.pack(">IIBBBBB", W, H, 8, 6, 0, 0, 0))
    png += chunk(b"IDAT", zlib.compress(bytes(raw), 9))
    png += chunk(b"IEND", b"")
    with open(os.path.join(OUT, "check_white.png"), "wb") as f:
        f.write(png)


if __name__ == "__main__":
    main()
