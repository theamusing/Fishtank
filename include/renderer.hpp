#pragma once

#include <vector>
#include "LGFX.hpp"
#include <LovyanGFX.hpp>

#define RENDER_WIDTH 160
#define RENDER_HEIGHT 120

class Renderer
{
public:
    LGFX_Sprite m_fb;
    Renderer() : m_fb(&m_lcd)
    {
        // m_lcd.begin();
        m_fb.setPsram(true);
        m_fb.setColorDepth(16);
        m_fb.createSprite(RENDER_WIDTH, RENDER_HEIGHT);
        m_fb.setSwapBytes(false);
    };
    void drawFrame(uint32_t draw_us, uint32_t frame_id)
    {
        sendFrameSerial(draw_us, frame_id);
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
};
