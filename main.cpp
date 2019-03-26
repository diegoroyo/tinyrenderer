#include <iostream>
#include "pngimage.h"

int main() {
    PNGImage img;
    img.read_png_file("png/green.png");

    return 0;
}