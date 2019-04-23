#include "rgbcolor.h"

const RGBColor RGBColor::Black(0, 0, 0);
const RGBColor RGBColor::White(255, 255, 255);
const RGBColor RGBColor::Red(255, 0, 0);
const RGBColor RGBColor::Green(0, 255, 0);
const RGBColor RGBColor::Blue(0, 0, 255);
const RGBColor RGBColor::Yellow(255, 255, 0);
const RGBColor RGBColor::Magenta(255, 0, 255);
const RGBColor RGBColor::Cyan(0, 255, 255);

void RGBColor::set_values(const RGBColor& other) {
    this->r = other.r;
    this->g = other.g;
    this->b = other.b;
}