#pragma once

#include "utils.hpp"

class SpriteData
{
public:
    SpriteData() : m_data(nullptr), m_size(0) {}

    SpriteData(const char *path)
    {
        if (!loadFilePsram(path, m_data, m_size))
        {
            Serial.println("Failed to load sprite data");
        }
    }

    ~SpriteData()
    {
        if (m_data)
        {
            free(m_data);
        }
    }

    bool get(uint8_t &data, size_t index, size_t length)
    {
        if (!m_data || index + length > m_size)
        {
            return false;
        }
        memcpy(&data, &m_data[index], length);
        return true;
    }

    uint8_t *getPtr(size_t index, size_t length)
    {
        if (!m_data || index + length > m_size)
        {
            return nullptr;
        }
        return &m_data[index];
    }

private:
    // store color index. lookup color map for real color.
    uint8_t *m_data = nullptr;
    size_t m_size = 0;
};