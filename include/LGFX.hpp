#pragma once
#include <Arduino.h>
#include <LovyanGFX.hpp>

/********* Waveshare ESP32-S3-Zero 连接 *********/
static const int PIN_LCD_SCK  = 12;  // GP12
static const int PIN_LCD_MOSI = 11;  // GP11
static const int PIN_LCD_MISO = 13;  // GP13（不接就设为 -1，并把 readable=false）
static const int PIN_LCD_CS   = 5;   // GP5
static const int PIN_LCD_DC   = 4;   // GP4
static const int PIN_LCD_RST  = 3;   // GP3（若直连3.3V则 -1）
static const int PIN_LCD_BL   = 2;   // GP2（直连3.3V则 -1）

// 触摸(FT6236/FT5x06)——若不用可全设为 -1
static const int PIN_CTP_SCL  = -1;   // GP9
static const int PIN_CTP_SDA  = -1;   // GP8
static const int PIN_CTP_INT  = -1;   // GP7
static const int PIN_CTP_RST  = -1;   // GP6
/***********************************************/

class LGFX : public lgfx::LGFX_Device {
  // 如果你的屏是 ST7789，就把 ILI9341 改成 ST7789（见下）
  lgfx::Panel_ILI9341 _panel;
  lgfx::Bus_SPI       _bus;
  lgfx::Light_PWM     _light;
  lgfx::Touch_FT5x06  _touch;  // 若是 GT911，改成 Touch_GT911

public:
  LGFX() {
    { // SPI 总线（ESP32-S3 用 SPI2_HOST 或 SPI3_HOST 都行）
      auto b = _bus.config();
      b.spi_host   = SPI3_HOST;     // “VSPI”
      b.spi_mode   = 0;
      b.freq_write = 40000000;      // 不稳可降到 27MHz
      b.freq_read  = 20000000;
      b.spi_3wire  = false;
      b.use_lock   = true;
      b.pin_sclk   = PIN_LCD_SCK;
      b.pin_mosi   = PIN_LCD_MOSI;
      b.pin_miso   = PIN_LCD_MISO;  // 若未接 MISO，设为 -1
      b.pin_dc     = PIN_LCD_DC;
      _bus.config(b);
      _panel.setBus(&_bus);
    }
    { // 面板
      auto p = _panel.config();
      p.pin_cs          = PIN_LCD_CS;
      p.pin_rst         = PIN_LCD_RST;   // 未接则 -1
      p.pin_busy        = -1;
      p.memory_width    = 240;
      p.memory_height   = 320;
      p.panel_width     = 240;
      p.panel_height    = 320;
      p.offset_x        = 0;
      p.offset_y        = 0;
      p.offset_rotation = 0;
      p.invert          = true;    // 颜色反了就 true（ST7789 常为 true）
      p.readable        = (PIN_LCD_MISO >= 0);
      p.bus_shared      = true;     // 与 SD 共享 SPI
      _panel.config(p);
    }
    if (PIN_LCD_BL >= 0) {         // 背光，可 PWM
      auto l = _light.config();
      l.pin_bl      = PIN_LCD_BL;
      l.invert      = false;
      l.freq        = 12000;
      l.pwm_channel = 7;
      _light.config(l);
      _panel.setLight(&_light);
    }
    { // 触摸（不用就把 setTouch 删掉）
      auto t = _touch.config();
      t.i2c_port = 0;     // Wire
      t.i2c_addr = 0x38;  // FT6236 常见地址；GT911 多为 0x5D
      t.pin_sda  = PIN_CTP_SDA;
      t.pin_scl  = PIN_CTP_SCL;
      t.pin_int  = PIN_CTP_INT;
      t.pin_rst  = PIN_CTP_RST;    // 未接则 -1
      t.freq     = 400000;
      t.x_min    = 0; t.y_min = 0;
      t.x_max    = 240; t.y_max = 320;
      _touch.config(t);
      // _panel.setTouch(&_touch);
    }
    setPanel(&_panel);
  }
};
