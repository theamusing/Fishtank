#include <Arduino.h>
#include <vector>
#include "LGFX.hpp"
#include "renderer.hpp"
#include <LovyanGFX.hpp>

Renderer::Renderer() : m_fb(&m_lcd)
{
    // m_lcd.begin();
    m_fb.setPsram(true);
    m_fb.setColorDepth(16);
    m_fb.createSprite(RENDER_WIDTH, RENDER_HEIGHT);
    m_fb.setSwapBytes(false);
}

void Renderer::drawFrame(uint32_t draw_us, uint32_t frame_id)
{
    sendFrameSerial(draw_us, frame_id);
}

void Renderer::sendFrameSerial(uint32_t draw_us, uint32_t frame_id)
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
