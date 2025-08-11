#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import argparse
import os
from pathlib import Path
from PIL import Image, ImageSequence
import numpy as np
import struct
import sys

def image_to_rgb565_bytes(img: Image.Image) -> bytes:
    """Convert a PIL RGB image to little-endian [w,h]+RGB565 bytes."""
    if img.mode != "RGB":
        img = img.convert("RGB")
    w, h = img.size
    rgb = np.array(img, dtype=np.uint8)
    # RGB888 -> RGB565
    r = (rgb[:, :, 0] >> 3).astype(np.uint16) << 11
    g = (rgb[:, :, 1] >> 2).astype(np.uint16) << 5
    b = (rgb[:, :, 2] >> 3).astype(np.uint16)
    rgb565 = (r | g | b).flatten()

    header = struct.pack("<HH", w, h)
    return header + rgb565.tobytes()

def save_single_png(png_path: Path, out_path: Path):
    """Save one PNG -> .bin. out_path can be file or directory."""
    with Image.open(png_path) as im:
        data = image_to_rgb565_bytes(im)

    if out_path.is_dir():
        out_file = out_path / (png_path.stem + ".bin")
    else:
        # ensure parent exists
        out_path.parent.mkdir(parents=True, exist_ok=True)
        out_file = out_path

    with open(out_file, "wb") as f:
        f.write(data)
    print(f"[OK] {png_path} -> {out_file}")

def save_gif_frames(gif_path: Path, out_dir: Path):
    """Save each GIF frame -> numbered .bin files inside out_dir."""
    out_dir.mkdir(parents=True, exist_ok=True)
    with Image.open(gif_path) as im:
        # Use full frames if possible; simple per-frame convert
        # (Most GIFs work fine with direct convert for each frame.)
        frame_idx = 0
        for frame in ImageSequence.Iterator(im):
            frame_rgb = frame.convert("RGB")
            data = image_to_rgb565_bytes(frame_rgb)
            out_file = out_dir / f"{gif_path.stem}_{frame_idx:03d}.bin"
            with open(out_file, "wb") as f:
                f.write(data)
            print(f"[OK] {gif_path} [frame {frame_idx}] -> {out_file}")
            frame_idx += 1

        if frame_idx == 0:
            print(f"[WARN] {gif_path} contains no frames?", file=sys.stderr)

def save_folder_pngs(folder: Path, out_dir: Path):
    """Convert all PNGs directly under folder (non-recursive)."""
    out_dir.mkdir(parents=True, exist_ok=True)
    pngs = sorted([p for p in folder.iterdir() if p.is_file() and p.suffix.lower() == ".png"])
    if not pngs:
        print(f"[WARN] No PNG found in folder: {folder}", file=sys.stderr)
        return
    for p in pngs:
        save_single_png(p, out_dir)

def main():
    parser = argparse.ArgumentParser(description="Convert PNG/GIF to RGB565 .bin")
    parser.add_argument("-i", "--input", required=True, help="Input path (PNG/GIF file or folder)")
    parser.add_argument("-o", "--output", required=True, help="Output path (file for single PNG, or folder)")
    args = parser.parse_args()

    in_path = Path(args.input)
    out_path = Path(args.output)

    if not in_path.exists():
        print(f"[ERR] Input path does not exist: {in_path}", file=sys.stderr)
        sys.exit(1)

    if in_path.is_dir():
        # Folder: process all PNGs into output folder
        if out_path.exists() and not out_path.is_dir():
            print(f"[ERR] Output must be a directory when input is a folder: {out_path}", file=sys.stderr)
            sys.exit(1)
        save_folder_pngs(in_path, out_path)

    elif in_path.is_file():
        ext = in_path.suffix.lower()
        if ext == ".png":
            # Single PNG: output can be file or dir
            if out_path.exists() and out_path.is_dir():
                save_single_png(in_path, out_path)
            else:
                # If output ends with .bin treat as file; otherwise, if doesn't exist, decide:
                if out_path.suffix.lower() == ".bin":
                    save_single_png(in_path, out_path)
                else:
                    # Treat as directory
                    out_path.mkdir(parents=True, exist_ok=True)
                    save_single_png(in_path, out_path)

        elif ext == ".gif":
            # GIF: output must be a directory
            if out_path.exists() and not out_path.is_dir():
                print(f"[ERR] Output must be a directory when input is a GIF: {out_path}", file=sys.stderr)
                sys.exit(1)
            save_gif_frames(in_path, out_path)
        else:
            print(f"[ERR] Unsupported input type: {ext}. Only .png, .gif, or folder.", file=sys.stderr)
            sys.exit(1)
    else:
        print(f"[ERR] Unsupported input path type: {in_path}", file=sys.stderr)
        sys.exit(1)

if __name__ == "__main__":
    main()
