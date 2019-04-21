#pragma once

#include <stdint.h>

class RGBColor {
   public:
    uint8_t r, g, b;
    RGBColor() : r(0), g(0), b(0) {}
    RGBColor(uint8_t _r, uint8_t _g, uint8_t _b) : r(_r), g(_g), b(_b) {}
};