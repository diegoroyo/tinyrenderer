#pragma once

#include <stdint.h>
#include <iostream>

class RGBColor {
   public:
    uint8_t r, g, b;
    RGBColor() : r(0), g(0), b(0) {}
    RGBColor(uint8_t _r, uint8_t _g, uint8_t _b) : r(_r), g(_g), b(_b) {}

    // Colores predefinidos
    static const RGBColor Black;
    static const RGBColor White;
    static const RGBColor Red;
    static const RGBColor Green;
    static const RGBColor Blue;
    static const RGBColor Yellow;
    static const RGBColor Magenta;
    static const RGBColor Cyan;
};