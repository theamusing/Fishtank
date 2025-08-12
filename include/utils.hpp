#pragma once

#include <FS.h>
#include <LittleFS.h>

bool loadFilePsram(const char *path, uint16_t *&data, size_t &size, bool psram = true)
{
    File file = LittleFS.open(path, "r");
    if (!file)
    {
        Serial.println("Failed to open sprite file");
        return false;
    }

    size = file.size();
    if (psram)
        data = (uint16_t *)ps_malloc(size);
    else
        data = (uint16_t *)malloc(size);
    if (!data)
    {
        Serial.println("Failed to allocate memory for sprite");
        file.close();
        return false;
    }

    file.read((uint8_t *)data, size);
    file.close();
    return true;
}

bool loadFilePsram(const char *path, uint8_t *&data, size_t &size, bool psram = true)
{
    File file = LittleFS.open(path, "r");
    if (!file)
    {
        Serial.println("Failed to open sprite file");
        return false;
    }

    size = file.size();
    if (psram)
        data = (uint8_t *)ps_malloc(size);
    else
        data = (uint8_t *)malloc(size);
    if (!data)
    {
        Serial.println("Failed to allocate memory for sprite");
        file.close();
        return false;
    }

    file.read((uint8_t *)data, size);
    file.close();
    return true;
}