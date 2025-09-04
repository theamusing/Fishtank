#pragma once

#include "utils.hpp"

#define COLOR_COUNT 37
#define COLOR_TRANSPARENT 0xF81F // 透明色
class ColorMap
{
public:
    void setup(uint16_t *color, size_t size)
    {
        if (size != COLOR_COUNT * sizeof(uint16_t))
        {
            Serial.println("Color map size mismatch");
            return;
        }
        m_size = size;
        m_color = (uint16_t *)ps_malloc(size);
        if (m_color)
        {
            memcpy(m_color, color, size);
        }
    }

    void setup(const char *path)
    {
        if (!loadFilePsram(path, m_color, m_size))
        {
            Serial.println("Failed to load color map");
        }
        if (m_size != COLOR_COUNT * sizeof(uint16_t))
        {
            Serial.println("Color map size mismatch");
        }
    }

    void copy(const ColorMap &c)
    {
        if (c.m_color)
        {
            m_size = c.m_size;
            m_color = (uint16_t *)ps_malloc(m_size);
            if (m_color)
            {
                memcpy(m_color, c.m_color, m_size);
            }
        }
    }

    ~ColorMap()
    {
        if (m_color)
        {
            free(m_color);
        }
    }

    void mix(const ColorMap &c, float ratio)
    {
        if (!m_color || !c.m_color || m_size != c.m_size)
            return;

        for (size_t i = 0; i < COLOR_COUNT; i++)
        {
            uint16_t color1 = m_color[i];
            uint16_t color2 = c.m_color[i];
            uint8_t r1, g1, b1, r2, g2, b2;
            getColorRGB(color1, r1, g1, b1);
            getColorRGB(color2, r2, g2, b2);
            uint8_t r = (uint8_t)(r1 * (1 - ratio) + r2 * ratio);
            uint8_t g = (uint8_t)(g1 * (1 - ratio) + g2 * ratio);
            uint8_t b = (uint8_t)(b1 * (1 - ratio) + b2 * ratio);
            m_color[i] = __builtin_bswap16((r << 11) | (g << 5) | b);
        }
    }

    void mix(ColorMap &dst, const ColorMap &c1, const ColorMap &c2, float ratio)
    {
        if (c1.m_size != c2.m_size || c1.m_size != dst.m_size)
        {
            Serial.println("Color map size mismatch");
            return;
        }

        for (size_t i = 0; i < COLOR_COUNT; i++)
        {
            uint16_t color1 = c1.getColor(i);
            uint16_t color2 = c2.getColor(i);
            uint8_t r1, g1, b1, r2, g2, b2;
            getColorRGB(color1, r1, g1, b1);
            getColorRGB(color2, r2, g2, b2);
            uint8_t r = (uint8_t)(r1 * (1 - ratio) + r2 * ratio);
            uint8_t g = (uint8_t)(g1 * (1 - ratio) + g2 * ratio);
            uint8_t b = (uint8_t)(b1 * (1 - ratio) + b2 * ratio);
            dst.m_color[i] = __builtin_bswap16((r << 11) | (g << 5) | b);
        }
    }

    void getColorRGB(uint8_t index, uint8_t &r, uint8_t &g, uint8_t &b) const
    {
        if (!m_color)
            return;

        uint16_t color = m_color[index];
        color = __builtin_bswap16(color); // Swap byte order
        r = (color >> 11) & 0x1F;
        g = (color >> 5) & 0x3F;
        b = color & 0x1F;
    }

    uint16_t getColor(uint8_t index) const
    {
        uint16_t color = m_color[index-1];
        if (!m_color || index == 0 || index > COLOR_COUNT)
            return COLOR_TRANSPARENT;
        return color;
    }

private:
    // use rgb565
    uint16_t *m_color;
    size_t m_size = 0;
};