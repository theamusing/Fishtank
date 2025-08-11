#include <Arduino.h>
#include <vector>
#include "renderer.hpp"
#include <LovyanGFX.hpp>

Renderer renderer;

void setup() {
  // 高波特率，带宽更高
  Serial.begin(2000000);
  delay(800);
}

void loop() {
  static uint32_t frame_id = 0;

  // ===== 绘制开始计时 =====
  uint32_t t0 = micros();
  uint32_t now = millis();

  // 计算渐变颜色（用正弦波平滑变化）
  uint8_t r = (sin(now * 0.002f) * 0.5f + 0.5f) * 255;
  uint8_t g = (sin(now * 0.002f + 2.094f) * 0.5f + 0.5f) * 255; // +120°
  uint8_t b = (sin(now * 0.002f + 4.188f) * 0.5f + 0.5f) * 255; // +240°
  renderer.m_fb.fillScreen(renderer.m_fb.color888(r, g, b));

  // 示例：一堆小圆点 + 一个移动圆，模拟负载
  for (int i = 0; i < 50; ++i) {
    int x = esp_random() % RENDER_WIDTH;
    int y = esp_random() % RENDER_HEIGHT;
    renderer.m_fb.fillCircle(x, y, 3, renderer.m_fb.color888(64, 128, 255));
  }
  int t = millis();
  int cx = (RENDER_WIDTH / 2) + (int)(sin(t * 0.003f) * (RENDER_WIDTH / 3));
  int cy = (RENDER_HEIGHT / 2) + (int)(cos(t * 0.003f) * (RENDER_HEIGHT / 3));
  renderer.m_fb.fillCircle(cx, cy, 50, renderer.m_fb.color888(255, 255, 255));

  uint32_t draw_us = micros() - t0;
  renderer.drawFrame(draw_us, frame_id++);
}
