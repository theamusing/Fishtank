#!/usr/bin/env python3
import argparse
from pathlib import Path
from typing import List, Tuple, Dict

import numpy as np
from PIL import Image


def load_image_rgba(path: Path) -> Image.Image:
    img = Image.open(path).convert("RGBA")
    return img


def adaptive_quantize_if_needed(img_rgba: Image.Image, max_colors: int = 255) -> Image.Image:
    """
    如果非透明颜色超过 max_colors，则进行自适应量化到 <= max_colors。
    保留 alpha：做法是对 RGB 做量化，然后把原 alpha 粘回来（alpha>0 视作不透明）。
    量化会引入颜色近似。
    """
    # 分离通道
    rgb = img_rgba.convert("RGB")
    a = img_rgba.getchannel("A")

    # 只看非透明区域做颜色判断
    alpha_np = np.array(a, dtype=np.uint8)
    rgb_np = np.array(rgb, dtype=np.uint8)
    opaque_mask = alpha_np > 0
    # 统计非透明像素的唯一颜色数量
    if opaque_mask.any():
        uniq = np.unique(rgb_np[opaque_mask].reshape(-1, 3), axis=0)
        if uniq.shape[0] > max_colors:
            # 使用 Pillow 的自适应调色板量化
            pal_img = rgb.convert("P", palette=Image.ADAPTIVE, colors=max_colors)
            # 把量化后的索引再映射回 RGB 以获得“代表色”，然后与原 alpha 合并
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
    """
    w, h = img_rgba.size
    pix = np.array(img_rgba, dtype=np.uint8)  # (H, W, 4)
    # mask: 非透明
    mask = pix[..., 3] > 0
    if not mask.any():
        return []  # 全透明
    # 只取非透明像素
    rgba_nonzero = pix[mask]
    # 用首次出现顺序去重
    # 将 RGBA 映射为 tuple 便于放到 dict
    seen: Dict[Tuple[int, int, int, int], None] = {}
    for r, g, b, a in rgba_nonzero:
        key = (int(r), int(g), int(b), int(a))
        if key not in seen:
            seen[key] = None
    return list(seen.keys())


def write_colormap_image(colors: List[Tuple[int, int, int, int]], out_path: Path):
    """
    生成 1×n 的 Color Map PNG（横向）。
    """
    n = len(colors)
    if n == 0:
        # 若没有非透明颜色，输出一个 1x1 全透明像素，避免文件缺失
        img = Image.new("RGBA", (1, 1), (0, 0, 0, 0))
        img.save(out_path)
        return
    strip = Image.new("RGBA", (n, 1))
    strip.putdata(colors)
    strip.save(out_path)


def write_index_binary(img_rgba: Image.Image, colors: List[Tuple[int, int, int, int]], out_path: Path):
    """
    写出与原图同形状的 uint8 二进制索引文件（行优先）。
    透明像素=0；非透明像素=colors 的 1-based 索引。
    """
    w, h = img_rgba.size
    pix = np.array(img_rgba, dtype=np.uint8)  # (H, W, 4)
    alpha = pix[..., 3]
    rgbas = pix.reshape(-1, 4)

    # 建立颜色到索引的映射（1-based）
    color_to_index: Dict[Tuple[int, int, int, int], int] = {
        c: i + 1 for i, c in enumerate(colors)
    }

    out = np.zeros((h * w,), dtype=np.uint8)

    # 填充索引
    for idx, (r, g, b, a) in enumerate(rgbas):
        if a == 0:
            out[idx] = 0
        else:
            key = (int(r), int(g), int(b), int(a))
            try:
                out[idx] = color_to_index[key]
            except KeyError:
                # 保险：若出现“未在表中”的颜色（理论不该发生），回退到最接近色策略或 0。
                # 这里选择置 0 并可选地抛出警告。
                out[idx] = 0

    # 按行优先写出
    out.tofile(out_path)


def main():
    parser = argparse.ArgumentParser(description="从图片生成 color map (1×n PNG) 与 uint8 索引二进制文件。透明像素索引为 0。")
    parser.add_argument("input", type=Path, help="输入图片路径（建议含透明通道 PNG 等）")
    parser.add_argument("-o", "--outdir", type=Path, default=None, help="输出目录（默认与输入同目录）")
    parser.add_argument("--basename", type=str, default=None, help="输出文件基名（默认沿用输入文件名，不含扩展名）")
    parser.add_argument("--max-colors", type=int, default=255, help="允许的最大非透明颜色数，超出将自适应量化（默认 255）")
    args = parser.parse_args()

    in_path: Path = args.input
    outdir: Path = args.outdir or in_path.parent
    basename: str = args.basename or in_path.stem

    outdir.mkdir(parents=True, exist_ok=True)

    img = load_image_rgba(in_path)
    img = adaptive_quantize_if_needed(img, max_colors=args.max_colors)

    colors = build_color_table(img)
    if len(colors) > args.max_colors:
        # 极端情况下仍超限（基本不会），再强制量化一次
        img = adaptive_quantize_if_needed(img, max_colors=args.max_colors)
        colors = build_color_table(img)

    # 写 color map：横向 1×n
    cmap_path = outdir / f"{basename}.colormap.png"
    write_colormap_image(colors, cmap_path)

    # 写索引二进制（行优先，H*W 个 uint8）
    idx_path = outdir / f"{basename}.index.u8.bin"
    write_index_binary(img, colors, idx_path)

    # 顺便写个简短的 metadata，便于读取（可选）
    meta = {
        "width": img.size[0],
        "height": img.size[1],
        "num_colors": len(colors),
        "index_meaning": "0=transparent; 1..n = position in colormap.png from left to right",
        "colormap_png": str(cmap_path.name),
        "index_bin": str(idx_path.name),
    }
    (outdir / f"{basename}.meta.txt").write_text(
        "\n".join(f"{k}: {v}" for k, v in meta.items()),
        encoding="utf-8"
    )

    print("Done.")
    print(f"Color map: {cmap_path}")
    print(f"Index bin: {idx_path}")
    print(f"Meta: {outdir / f'{basename}.meta.txt'}")


if __name__ == "__main__":
    main()
