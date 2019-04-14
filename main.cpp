#include <iostream>
#include "pngimage.h"

int main() {
    PNGImage img;
    img.read_png_file("png/color.png");

    return 0;
}