#pragma once

#include <vector>
#include "LGFX.hpp"
#include <LovyanGFX.hpp>

#define RENDER_WIDTH 160
#define RENDER_HEIGHT 120
#define PHYSICAL_WIDTH 320
#define PHYSICAL_HEIGHT 240
#define FPS 10.0f

class Renderer
{
public:
    LGFX_Sprite m_fb;
    Renderer() : m_sp(&m_lcd){};

    void setup()
    {
        m_lcd.init();
        m_lcd.setRotation(1); // 0~3
        if (PIN_LCD_BL >= 0)
            m_lcd.setBrightness(255);

        m_fb.setPsram(true);
        m_fb.setColorDepth(16);
        m_fb.createSprite(RENDER_WIDTH, RENDER_HEIGHT);
        m_fb.setSwapBytes(false);

        m_sp.setPsram(true);
        m_sp.setColorDepth(16);
        m_sp.createSprite(PHYSICAL_WIDTH, PHYSICAL_HEIGHT);
        m_sp.setSwapBytes(false);

        Serial.println("Sprite buffer created");
    }

    void drawFrame(uint32_t draw_us, uint32_t frame_id)
    {
        // sendFrameSerial(draw_us, frame_id);
        uint32_t start = micros();
        render2lcd();
        Serial.println("render time: " + String(draw_us / 1000.0f) + " ms, frame id: " + String(frame_id));
        Serial.println("push time: " + String((micros() - start) / 1000.0f) + " ms");
    }

private:
    struct __attribute__((packed)) FrameHeader
    {
        uint32_t magic;    // 4 bytes
        uint16_t w;        // 2 bytes
        uint16_t h;        // 2 bytes
        uint8_t bpp;       // 1 byte
        uint8_t flags;     // 1 byte
        uint32_t payload;  // 4 bytes
        uint32_t draw_us;  // 4 bytes
        uint32_t frame_id; // 4 bytes
    };

    LGFX m_lcd;
    LGFX_Sprite m_sp;
    void sendFrameSerial(uint32_t draw_us, uint32_t frame_id)
    {
        static std::vector<uint16_t> line(RENDER_WIDTH);
        FrameHeader hdr;
        hdr.magic = 0xDEADBEEF;
        hdr.w = RENDER_WIDTH;
        hdr.h = RENDER_HEIGHT;
        hdr.bpp = 2;
        hdr.flags = 0;
        hdr.payload = RENDER_WIDTH * RENDER_HEIGHT * 2;
        hdr.draw_us = draw_us;
        hdr.frame_id = frame_id;

        Serial.write((uint8_t *)&hdr, sizeof(hdr));
        Serial.write((uint8_t *)m_fb.getBuffer(), RENDER_WIDTH * RENDER_HEIGHT * 2);
    }

    void render2lcd()
    {
        m_fb.drawString("Hello Sprite", 10, 10);
        m_sp.pushImageRotateZoom(0, 0, 0, 0, 0, PHYSICAL_WIDTH/RENDER_WIDTH, PHYSICAL_HEIGHT/RENDER_HEIGHT, RENDER_WIDTH, RENDER_HEIGHT, (uint16_t*)m_fb.getBuffer());
        m_sp.pushSprite(0, 0);
    }
};
