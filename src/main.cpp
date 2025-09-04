#include <Arduino.h>
#include <FS.h>
#include <LittleFS.h>
#include <LovyanGFX.hpp>

#include <vector>

#include "renderer.hpp"
#include "spriteData.hpp"
#include "colorMap.hpp"
#include "gameObject.hpp"
#include "fish.hpp"

Renderer renderer;
SpriteData bgData, fgData, clownfishData, longfishData, guppyData;
ColorMap colorMap;
GameObject<160, 120> bg;
GameObject<160, 40> fg;
ClownFish clownfish;
std::vector<Guppy> guppies;
LongFish longfish;

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
    LittleFS.begin();

    renderer.setup();

    bgData.setup("/bg.bin");
    fgData.setup("/fg.bin");

    clownfishData.setup("/fish/clownfish.bin");
    longfishData.setup("/fish/longfish.bin");
    guppyData.setup("/fish/guppy.bin");

    colorMap.setup("/colormaps/colormap.bin");
    bg.setup();
    fg.setup();

    clownfish.setup();
    longfish.setup();
    for (int i = 0; i < 5; i++)
    {
        guppies.emplace_back();
        guppies.back().setup();
    }
    clownfish.setPos(40, 40);
    longfish.setPos(120, 80);
    for (int i = 0; i < 5; i++)
    {
        guppies[i].setPos(80 + i * 10, random(20, 100));
    }
}

void loop()
{
    static uint32_t frame_id = 0;

    // ===== 绘制开始计时 =====
    uint32_t t0 = micros();

    // fill with blue
    renderer.m_fb.fillScreen(renderer.m_fb.color565(128, 0, 0));
    bg.setPos(80, 60);
    bg.draw(renderer.m_fb, bgData, colorMap);

    clownfish.update(frame_id);
    clownfish.draw(renderer.m_fb, clownfishData, colorMap);

    longfish.update(frame_id);
    longfish.draw(renderer.m_fb, longfishData, colorMap);

    for (auto &guppy : guppies)
    {
        guppy.update(frame_id);
        guppy.draw(renderer.m_fb, guppyData, colorMap);
    }

    fg.setPos(80, 100);
    fg.draw(renderer.m_fb, fgData, colorMap);

    float t3 = (millis() % 10000) / 10000.0f; // 0~1

    // 亮度曲线（白天亮，晚上暗）
    float brightness = max(1.0f + 0.2f * sin(t3 * 2 * M_PI), 1.0);

    // 色温曲线（傍晚偏暖，夜晚偏冷）
    float r_scale = 1.0f;
    float g_scale = 1.0f - 0.2f * sin(t3 * 2 * M_PI + M_PI / 2);
    float b_scale = 1.0f - 0.3f * sin(t3 * 2 * M_PI + M_PI / 2);

    applyDayNight(renderer.m_fb, brightness, r_scale, g_scale, b_scale);

    uint32_t draw_us = micros() - t0;
    renderer.drawFrame(draw_us, frame_id++);
}
