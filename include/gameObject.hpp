#pragma once

#include <FS.h>  
#include <LittleFS.h>
#include <vector>
#include "renderer.hpp"
#include "colorMap.hpp"
#include "spriteData.hpp"
#include <LovyanGFX.hpp>
#include "esp_heap_caps.h"

template <size_t WIDTH, size_t HEIGHT>
class GameObject
{
public:
    void setup()
    {
        if (m_buffer == nullptr) {
            m_buffer = (uint16_t*)ps_malloc(WIDTH * HEIGHT * 2);
        }
    }
    void update()
    {
        // Update the game object's state
    }
    void draw(LGFX_Sprite& sprite, SpriteData& spriteData, ColorMap& colorMap)
    {
        size_t offset = m_spriteOffset + m_currentFrame * WIDTH * HEIGHT;
        uint8_t* ptr = spriteData.getPtr(offset, WIDTH * HEIGHT);
        if (ptr == nullptr)
            return;
            
        for (int i = 0; i < WIDTH * HEIGHT; i++) {
            uint8_t colorIndex = ptr[i];
            uint16_t color = colorMap.getColor(colorIndex);
            m_buffer[i] = color;
        }

        // draw
        sprite.pushImageRotateZoom(m_posX, m_posY, WIDTH / 2, HEIGHT / 2, m_rotation, m_scale, m_scale, WIDTH, HEIGHT, m_buffer);
    }

    void setPos(int x, int y)
    {
        m_posX = x;
        m_posY = y;
    }

    void setScale(float scale)
    {
        m_scale = scale;
    }

    void setRotation(float rotation)
    {
        m_rotation = rotation;
    }

    void check()
    {
        if (esp_ptr_external_ram(m_buffer)) {
            Serial.println("Pointer is in PSRAM");
        } else {
            Serial.println("Pointer is in internal RAM");
        }
    }

protected:
    size_t FRAME_COUNT = 1;
    int m_posX = 0;
    int m_posY = 0;
    float m_scale = 1.0f;
    float m_rotation = 0.0f;

    size_t m_spriteOffset = 0;
    int m_currentFrame = 0;

    static uint16_t* m_buffer;
};

template <size_t WIDTH, size_t HEIGHT>
uint16_t* GameObject<WIDTH, HEIGHT>::m_buffer = nullptr;