#include <Arduino.h>
#include <FS.h>
#include <LittleFS.h>
#include <LovyanGFX.hpp>

#include <vector>

#include "renderer.hpp"
#include "spriteData.hpp"
#include "colorMap.hpp"
#include "gameObject.hpp"

Renderer renderer;
SpriteData spriteData("/eye/test.bin");
ColorMap colorMap;

void listDir(fs::FS &fs, const char *dirname, uint8_t levels)
{
    Serial.printf("Listing directory: %s\n", dirname);

    File root = fs.open(dirname);
    if (!root)
    {
        Serial.println("Failed to open directory");
        return;
    }
    if (!root.isDirectory())
    {
        Serial.println("Not a directory");
        return;
    }

    File file = root.openNextFile();
    while (file)
    {
        if (file.isDirectory())
        {
            Serial.print("  DIR : ");
            Serial.println(file.name());
            if (levels)
            {
                listDir(fs, '/' + file.name(), levels - 1);
            }
        }
        else
        {
            Serial.print("  FILE: ");
            Serial.print(file.name());
            Serial.print("  SIZE: ");
            Serial.println(file.size());
        }
        file = root.openNextFile();
    }
}

void applyDayNight(LGFX_Sprite &spr, float brightness, float r_scale, float g_scale, float b_scale)
{
    uint16_t *buf = (uint16_t *)spr.getBuffer();
    int total_pixels = spr.width() * spr.height();

    for (int i = 0; i < total_pixels; i++)
    {
        uint16_t c = buf[i];
        uint16_t new_c = __builtin_bswap16(c);
        uint8_t r = ((new_c >> 11) & 0x1F) << 3;
        uint8_t g = ((new_c >> 5) & 0x3F) << 2;
        uint8_t b = (new_c & 0x1F) << 3;
        // 调整亮度 & 色温
        r = std::min(255, int(r * r_scale * brightness));
        g = std::min(255, int(g * g_scale * brightness));
        b = std::min(255, int(b * b_scale * brightness));
        buf[i] = __builtin_bswap16(spr.color565(r, g, b));
    }
}

void setup()
{
    // 高波特率，带宽更高
    Serial.begin(2000000);
    delay(3000);
    Serial.println("Serial started");
    if (!LittleFS.begin())
    {
        Serial.println("LittleFS mount failed, try formatting...");
        if (!LittleFS.begin(true))
        {
            Serial.println("LittleFS format failed");
            return;
        }
    }
    listDir(LittleFS, "/", 3);
    if (psramInit())
    {
        Serial.printf("PSRAM size: %d bytes\n", ESP.getPsramSize());
    }
    else
    {
        Serial.println("PSRAM not available");
    }
    GameObject<64, 64> gameObject;
    gameObject.setup();
    gameObject.check();
}

void loop()
{
    static uint32_t frame_id = 0;

    // ===== 绘制开始计时 =====
    uint32_t t0 = micros();
    uint32_t now = millis();

    // 计算渐变颜色（用正弦波平滑变化）
    uint8_t r = (sin(now * 0.002f) * 0.5f + 0.5f) * 255;
    uint8_t g = (sin(now * 0.002f + 2.094f) * 0.5f + 0.5f) * 255; // +120°
    uint8_t b = (sin(now * 0.002f + 4.188f) * 0.5f + 0.5f) * 255; // +240°
    renderer.m_fb.fillScreen(renderer.m_fb.color565(255, 128, 128));

    // 示例：一堆小圆点 + 一个移动圆，模拟负载
    for (int i = 0; i < 50; ++i)
    {
        int x = esp_random() % RENDER_WIDTH;
        int y = esp_random() % RENDER_HEIGHT;
        renderer.m_fb.fillCircle(x, y, 3, renderer.m_fb.color565(64, 128, 255));
    }
    int t = millis();
    int cx = (RENDER_WIDTH / 2) + (int)(sin(t * 0.003f) * (RENDER_WIDTH / 3));
    int cy = (RENDER_HEIGHT / 2) + (int)(cos(t * 0.003f) * (RENDER_HEIGHT / 3));
    renderer.m_fb.fillCircle(cx, cy, 50, renderer.m_fb.color565(255, 255, 255));

    // draw a jpg file
    // renderer.m_fb.drawJpgFile(LittleFS, "/eye/eye-0000.jpg", 0, 0);
    File f = LittleFS.open("/eye/test.bin", "r");
    if (!f)
    {
        Serial.println("Failed to open image file");
        return;
    }
    t0 = micros();
    // 分配内存
    uint16_t *buf = (uint16_t *)ps_malloc(64 * 64 * 2);
    if (!buf)
    {
        Serial.println("Malloc failed");
        return;
    }
    f.read((uint8_t *)buf, 64 * 64 * 2);
    renderer.m_fb.pushImageRotateZoom(0, 0, 0, 0, 0, 2, 2, 64, 64, buf);
    //   renderer.m_fb.pushImage(0, 0, 64, 64, buf);
    float t3 = (millis() % 10000) / 10000.0f; // 0~1

    // 亮度曲线（白天亮，晚上暗）
    float brightness = 0.6f + 0.4f * sin(t3 * 2 * M_PI);

    // 色温曲线（傍晚偏暖，夜晚偏冷）
    float r_scale = 1.0f;
    float g_scale = 1.0f - 0.2f * sin(t3 * 2 * M_PI + M_PI / 2);
    float b_scale = 1.0f - 0.3f * sin(t3 * 2 * M_PI + M_PI / 2);

    applyDayNight(renderer.m_fb, brightness, r_scale, g_scale, b_scale);

    uint32_t draw_us = micros() - t0;
    // uint32_t draw_us = micros() - t0;
    //   renderer.drawFrame(draw_us, frame_id++);
    f.close();
    free(buf);
}
