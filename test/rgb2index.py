import argparse
from pathlib import Path
from typing import List, Tuple, Dict, Literal

import numpy as np
from PIL import Image, ImageSequence


# -----------------------
# I/O 与预处理
# -----------------------

def load_image_rgba(path: Path) -> Image.Image:
    img = Image.open(path).convert("RGBA")
    return img


def adaptive_quantize_if_needed(img_rgba: Image.Image, max_colors: int = 255) -> Image.Image:
    """
    仅用于控制“调色板图片（colormap）”的颜色数，保证 <= max_colors。
    对源图/帧我们并不强制量化（因为有“最近色”映射），避免引入不必要的失真。
    """
    rgb = img_rgba.convert("RGB")
    a = img_rgba.getchannel("A")

    alpha_np = np.array(a, dtype=np.uint8)
    rgb_np = np.array(rgb, dtype=np.uint8)
    opaque_mask = alpha_np > 0
    if opaque_mask.any():
        uniq = np.unique(rgb_np[opaque_mask].reshape(-1, 3), axis=0)
        if uniq.shape[0] > max_colors:
            pal_img = rgb.convert("P", palette=Image.ADAPTIVE, colors=max_colors)
            quant_rgb = pal_img.convert("RGB")
            out = Image.merge("RGBA", (
                quant_rgb.getchannel("R"),
                quant_rgb.getchannel("G"),
                quant_rgb.getchannel("B"),
                a
            ))
            return out
    return img_rgba


def build_color_table(img_rgba: Image.Image) -> List[Tuple[int, int, int, int]]:
    """
    返回按“第一次出现顺序”的 RGBA 唯一颜色列表（仅非透明）。
    一般用于 1×N 的 colormap PNG。
    """
    w, h = img_rgba.size
    pix = np.array(img_rgba, dtype=np.uint8)  # (H, W, 4)
    mask = pix[..., 3] > 0
    if not mask.any():
        return []
    rgba_nonzero = pix[mask]
    seen: Dict[Tuple[int, int, int, int], None] = {}
    for r, g, b, a in rgba_nonzero:
        key = (int(r), int(g), int(b), int(a))
        if key not in seen:
            seen[key] = None
    return list(seen.keys())


# -----------------------
# 帧加载（单张 PNG / 文件夹 PNG 序列 / GIF）
# -----------------------

InputKind = Literal["single_png", "png_folder", "gif"]

def detect_input_kind(p: Path) -> InputKind:
    if p.is_dir():
        return "png_folder"
    suffix = p.suffix.lower()
    if suffix == ".gif":
        return "gif"
    # 其他一律当作 PNG（或能被 PIL 打开）单图
    return "single_png"


def load_frames_rgba(in_path: Path) -> List[Image.Image]:
    kind = detect_input_kind(in_path)
    frames: List[Image.Image] = []

    if kind == "single_png":
        frames = [load_image_rgba(in_path)]

    elif kind == "png_folder":
        pngs = sorted([f for f in in_path.iterdir()
                       if f.is_file() and f.suffix.lower() == ".png"])
        if not pngs:
            raise ValueError(f"文件夹中没有 PNG：{in_path}")
        frames = [load_image_rgba(p) for p in pngs]

    else:  # gif
        im = Image.open(in_path)
        # 简单“合帧”策略：逐帧贴到一张 RGBA 画布上，尽量还原视觉效果
        # 注：GIF 的 disposal/partial 更新比较复杂，这里采用常见可行的合成方式
        base = Image.new("RGBA", im.size, (0, 0, 0, 0))
        prev = base.copy()
        for frame in ImageSequence.Iterator(im):
            fr = frame.convert("RGBA")
            # 如果 frame 提供了局部更新区域，直接 paste 会保留既有像素
            composed = prev.copy()
            composed.alpha_composite(fr)
            frames.append(composed)
            # prev = composed

    # 尺寸一致性检查
    w0, h0 = frames[0].size
    for i, fr in enumerate(frames):
        if fr.size != (w0, h0):
            raise ValueError(f"第 {i} 帧尺寸 {fr.size} 与首帧 {(w0, h0)} 不一致。请统一尺寸后再处理。")
    return frames


# -----------------------
# 索引写出（支持就近色）
# -----------------------

def write_index_for_frame(img_rgba: Image.Image,
                          colors: List[Tuple[int, int, int, int]],
                          palette_rgb_i16: np.ndarray) -> np.ndarray:
    """
    将单帧 RGBA 转换为 (H*W,) 的 uint8 索引数组：
      - alpha==0 -> 0
      - 命中 RGBA -> 1-based
      - 未命中   -> RGB 最近色（欧氏距离）对应的 1-based
    返回一维数组，行优先。
    """
    w, h = img_rgba.size
    pix = np.array(img_rgba, dtype=np.uint8)  # (H, W, 4)
    rgba_flat = pix.reshape(-1, 4)

    color_to_index: Dict[Tuple[int, int, int, int], int] = {c: i + 1 for i, c in enumerate(colors)}

    out = np.zeros((rgba_flat.shape[0],), dtype=np.uint8)

    alpha = rgba_flat[:, 3]
    non_transparent_mask = alpha != 0
    non_transparent_idx = np.nonzero(non_transparent_mask)[0]

    non_transparent_rgba = rgba_flat[non_transparent_mask]
    tuples = [tuple(map(int, px)) for px in non_transparent_rgba]
    unknown_positions = []

    for k, pos in zip(tuples, non_transparent_idx):
        idx = color_to_index.get(k)
        if idx is not None:
            out[pos] = idx
        else:
            unknown_positions.append(pos)

    if unknown_positions:
        unknown_positions = np.asarray(unknown_positions, dtype=np.int64)
        unknown_rgba = rgba_flat[unknown_positions]                # (M, 4)
        unknown_rgb = unknown_rgba[:, :3].astype(np.int16)         # (M, 3)

        # 唯一化未知颜色，加速最近邻搜索
        uniq_unknown_rgb, inv = np.unique(unknown_rgb, axis=0, return_inverse=True)  # (K,3), (M,)

        # 距离矩阵：(K, N)
        diff = uniq_unknown_rgb[:, None, :] - palette_rgb_i16[None, :, :]
        diff32 = diff.astype(np.int32)
        dist2 = (diff32 * diff32).sum(axis=2)
        nearest_palette_idx0 = dist2.argmin(axis=1)                # (K,)

        mapped_indices_1based = (nearest_palette_idx0[inv] + 1).astype(np.uint8)
        out[unknown_positions] = mapped_indices_1based

    return out  # (H*W,)


def preview_first_frame(orig_rgba: Image.Image,
                        indices_flat: np.ndarray,
                        colors: List[Tuple[int, int, int, int]],
                        basename: str,
                        outdir: Path):
    """
    用 matplotlib 展示第一帧的原图 vs 最近色映射后的图，并各自保存 PNG。
    """
    import matplotlib.pyplot as plt

    w, h = orig_rgba.size

    # 构造 LUT：0 -> 全透明，其余 1..N 为调色板 RGBA
    lut = np.zeros((len(colors) + 1, 4), dtype=np.uint8)
    if colors:
        lut[1:] = np.asarray(colors, dtype=np.uint8)

    mapped_rgba = lut[indices_flat.reshape(h, w)]  # (H,W,4) uint8

    # 保存两张图片
    src_path = outdir / f"{basename}.preview_src.png"
    mapped_path = outdir / f"{basename}.preview_mapped.png"
    orig_rgba.save(src_path)
    Image.fromarray(mapped_rgba, mode="RGBA").save(mapped_path)

    # 展示
    plt.figure(figsize=(10, 5))
    plt.subplot(1, 2, 1)
    plt.title("Original First Frame (RGBA)")
    plt.imshow(orig_rgba)
    plt.axis("off")

    plt.subplot(1, 2, 2)
    plt.title("Mapped (Nearest Color)")
    plt.imshow(mapped_rgba)
    plt.axis("off")

    plt.tight_layout()
    plt.show()

    print(f"[preview] saved: {src_path}")
    print(f"[preview] saved: {mapped_path}")

# -----------------------
# 主流程
# -----------------------

def main():
    parser = argparse.ArgumentParser(
        description="把 PNG / GIF / PNG 序列（文件夹）转换为索引序列并拼接输出 .bin；透明索引=0。"
    )
    parser.add_argument("input", type=Path, help="输入：单张 PNG，或 GIF，或包含 PNG 的文件夹")
    parser.add_argument("colormap", type=Path, help="调色板 PNG（1×N），用于定义 1..N 的索引")
    parser.add_argument("-o", "--outdir", type=Path, default=None, help="输出目录（默认与输入同目录）")
    parser.add_argument("--basename", type=str, default=None, help="输出文件基名（默认沿用输入名或文件夹名）")
    parser.add_argument("--max-colors", type=int, default=255, help="调色板允许的最大非透明颜色数（默认 255）")
    parser.add_argument("--preview", action="store_true", help="展示并保存第一帧原图与最近色映射后的对比图")
    args = parser.parse_args()

    in_path: Path = args.input
    outdir: Path = args.outdir or in_path.parent
    basename: str = args.basename or (in_path.stem if in_path.is_file() else in_path.name)
    outdir.mkdir(parents=True, exist_ok=True)

    # 1) 加载帧序列（自动识别 PNG/GIF/文件夹）
    frames = load_frames_rgba(in_path)
    if not frames:
        raise RuntimeError("未能从输入中解析到任何帧。")

    # 2) 处理调色板
    colormap_img = load_image_rgba(args.colormap)
    colormap_img = adaptive_quantize_if_needed(colormap_img, max_colors=args.max_colors)
    colors = build_color_table(colormap_img)
    if len(colors) == 0:
        raise ValueError("调色板（非透明）颜色数为 0。")
    if len(colors) > args.max_colors:
        # 最后一重保险（几乎用不到）
        colormap_img = adaptive_quantize_if_needed(colormap_img, max_colors=args.max_colors)
        colors = build_color_table(colormap_img)

    palette = np.asarray(colors, dtype=np.uint8)     # (N,4)
    palette_rgb_i16 = palette[:, :3].astype(np.int16)

    # 3) 写出 .bin（把所有帧拼接起来）
    idx_path = outdir / f"{basename}.bin"
    indices_used: set[int] = set()   # 记录真实用到的索引（排除 0）
    w, h = frames[0].size
    frame_stride = w * h
    total_bytes = 0

    first_frame_indices = None
    if args.preview:
        first_frame_indices = write_index_for_frame(frames[0], colors, palette_rgb_i16)

    with open(idx_path, "wb") as f:
        for i, fr in enumerate(frames):
            arr = write_index_for_frame(fr, colors, palette_rgb_i16)  # (H*W,)
            f.write(arr.tobytes())
            total_bytes += arr.size
            # 统计本帧所用索引（排除 0）
            used = np.unique(arr)
            for v in used:
                if v != 0:
                    indices_used.add(int(v))
                    
    if args.preview and first_frame_indices is not None:
        for i in range(len(frames)):
            preview_first_frame(
                orig_rgba=frames[i],
                indices_flat=first_frame_indices,
                colors=colors,
                basename=basename,
                outdir=outdir
            )
        
    # 4) 写 meta
    input_kind = detect_input_kind(in_path)
    meta_lines = [
        f"input_kind: {input_kind}",
        f"source: {str(in_path)}",
        f"width: {w}",
        f"height: {h}",
        f"frames: {len(frames)}",
        f"frame_stride: {frame_stride}  # 每帧像素数（H*W）= 索引数",
        f"total_indices_written: {total_bytes}",
        f"colormap_nontransparent_colors: {len(colors)}",
        f"indices_used_count: {len(indices_used)}",
        f"indices_used_sorted: {sorted(indices_used)}",
        "index_meaning: 0=transparent; 1..n = position in colormap.png from left to right",
        f"index_bin: {idx_path.name}",
    ]
    meta_path = outdir / f"{basename}.meta.txt"
    meta_path.write_text("\n".join(meta_lines), encoding="utf-8")

    print("Done.")
    print(f"Frames: {len(frames)}  Size: {w}x{h}")
    print(f"Index bin: {idx_path}")
    print(f"Meta: {meta_path}")


if __name__ == "__main__":
    main()
