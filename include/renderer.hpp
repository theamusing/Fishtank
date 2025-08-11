#include <Arduino.h>
#include <vector>
#include "LGFX.hpp"
#include <LovyanGFX.hpp>

#define RENDER_WIDTH 160
#define RENDER_HEIGHT 120

class Renderer
{
public:
    LGFX_Sprite m_fb;
    Renderer();
    void drawFrame(uint32_t draw_us, uint32_t frame_id);

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
    void sendFrameSerial(uint32_t draw_us, uint32_t frame_id);
};
