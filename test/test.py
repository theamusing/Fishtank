import struct
import time
import numpy as np
import cv2
import serial

# ==== 参数 ====
PORT = 'COM5'
BAUD = 2000000

# ==== 帧头结构 ====
FRAME_HEADER_FMT = "<IHHBBIII"
HEADER_SIZE = struct.calcsize(FRAME_HEADER_FMT)
MAGIC = 0xDEADBEEF

# 预计算 2x2 亮度核（左上=原色，右上/左下=75%，右下=50%）
LIT_KERNEL = np.array([[1.00, 1.00],
                       [1.00, 1.00]], dtype=np.float32)

def upscale_pixel_round(bgr):
    h, w = bgr.shape[:2]
    # 先做最近邻 2x 放大（每像素扩成2x2块）
    big = np.repeat(np.repeat(bgr, 2, axis=0), 2, axis=1).astype(np.float32)
    # 为整幅图铺满亮度遮罩（Kronecker 积）
    shade = np.kron(np.ones((h, w), dtype=np.float32), LIT_KERNEL)
    shade = shade[..., None]  # -> (2h, 2w, 1)
    # 乘以亮度并裁剪回 uint8
    out = np.clip(big * shade, 0, 255).astype(np.uint8)
    return out

ser = serial.Serial(PORT, BAUD, timeout=1)
# 可选：防止打开串口触发板子复位
try:
    ser.setDTR(False)
    ser.setRTS(False)
except Exception:
    pass

last_frame_wall = time.perf_counter()

while True:
    # ===== A) 串口读取（头+负载）计时 =====
    t_read_start = time.perf_counter()

    # 1) 读帧头
    header_bytes = ser.read(HEADER_SIZE)
    if len(header_bytes) != HEADER_SIZE:
        continue
    magic, w, h, bpp, flags, payload, draw_us, frame_id = struct.unpack(FRAME_HEADER_FMT, header_bytes)
    if magic != MAGIC:
        # 失步，继续找下一帧
        continue

    # 2) 读像素数据
    frame_bytes = ser.read(payload)
    if len(frame_bytes) != payload:
        continue

    t_read_end = time.perf_counter()

    # ===== B) 解码计时 =====
    t_decode_start = time.perf_counter()
    # 大端 + RGB565
    pix = np.frombuffer(frame_bytes, dtype='>u2').reshape(h, w)
    r = ((pix >> 11) & 0x1F) << 3
    g = ((pix >> 5)  & 0x3F) << 2
    b = (pix & 0x1F) << 3
    bgr = np.dstack((b, g, r)).astype(np.uint8)
    t_decode_end = time.perf_counter()

    # ===== C) 显示计时 =====
    t_show_start = time.perf_counter()
    k = 5  # 放大倍数
    big = cv2.resize(bgr, None, fx=k, fy=k, interpolation=cv2.INTER_NEAREST)
    # big = upscale_pixel_round(bgr)
    cv2.imshow(f"ESP32 Frame x{k}", big)
    # cv2.imshow("ESP32 Frame", bgr)
    key = cv2.waitKey(1)
    t_show_end = time.perf_counter()
    if key == 27:  # ESC
        break

    # ===== D) 统计与日志 =====
    read_ms   = (t_read_end   - t_read_start) * 1000.0
    decode_ms = (t_decode_end - t_decode_start) * 1000.0
    show_ms   = (t_show_end   - t_show_start) * 1000.0
    draw_ms   = draw_us / 1000.0  # 设备端绘制时间（来自帧头）
    total_ms  = read_ms + decode_ms + show_ms

    # 整体 FPS（基于墙钟时间）
    now_wall = t_show_end
    wall_dt  = now_wall - last_frame_wall
    last_frame_wall = now_wall
    fps = (1.0 / wall_dt) if wall_dt > 0 else 0.0

    # 串口有效吞吐（只算像素负载，不含帧头）
    mbps = (payload / (1024.0 * 1024.0)) / ((t_read_end - t_read_start) if (t_read_end > t_read_start) else 1e-9)

    print(
        f"fid={frame_id:6d} "
        f"read={read_ms:6.2f}ms "
        f"decode={decode_ms:6.2f}ms "
        f"show={show_ms:6.2f}ms "
        f"| draw(dev)={draw_ms:6.2f}ms "
        f"| total={total_ms:6.2f}ms "
        f"| fps={fps:5.1f} "
        f"| thr={mbps:4.2f} MiB/s"
    )
