#pragma once
#include <LovyanGFX.hpp>

class LGFX : public lgfx::LGFX_Device {
  lgfx::Panel_ST7789 _panel;
  lgfx::Bus_SPI      _bus;

public:
  LGFX() {
    { // SPI
      auto c = _bus.config();
      c.spi_host   = SPI3_HOST;
      c.spi_mode   = 0;
      c.freq_write = 40000000;
      c.freq_read  = 16000000;
      c.pin_sclk   = 18;
      c.pin_mosi   = 23;
      c.pin_miso   = -1;
      c.pin_dc     = 2;
      _bus.config(c);
      _panel.setBus(&_bus);
    }
    { // 面板参数（按你的屏改，不接屏也无妨）
      auto p = _panel.config();
      p.pin_cs   = 5;
      p.pin_rst  = 4;
      p.pin_busy = -1;
      p.memory_width  = 240;
      p.memory_height = 240;
      p.panel_width   = 240;
      p.panel_height  = 240;
      p.offset_x = 0;
      p.offset_y = 0;
      _panel.config(p);
    }
    setPanel(&_panel);
  }
};
