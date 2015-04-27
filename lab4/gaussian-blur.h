#ifndef GAUSSIAN_BLUR_H

#define GAUSSIAN_BLUR_H
#include "PNM.h"
#include "stencil.h"

void gaussianBlur(PNM &image, PNM &retImage, int threadCount, int startRow, int endRow, Stencil stencil) {
    int r = stencil.r;
#pragma omp parallel for num_threads(threadCount)
    for (int i=startRow; i<=endRow; i++) {
        for (int j=0; j<image.width; j++) {
            double sumR = 0, sumG = 0, sumB = 0;
            for (int px=-r; px<=r; px++) {
                for (int py=-r; py<=r; py++) {
                    int x = i+px, y = j+py;
                    if (x < 0 || x >= image.height || y < 0 || y >= image.width) {
                        if (x < 0) {
                            x = -x;
                        }
                        if (y < 0) {
                            y = -y;
                        }
                        if (x >= image.height) {
                            x = image.height-(x-image.height+1);
                        }
                        if (y >= image.width) {
                            y = image.width-(y-image.width+1);
                        }
                    }
                    sumR += image.r(x, y) * stencil.data(px+r, py+r);
                    sumG += image.g(x, y) * stencil.data(px+r, py+r);
                    sumB += image.b(x, y) * stencil.data(px+r, py+r);
                }
            }
            retImage.r(i, j) = (int)sumR;
            retImage.g(i, j) = (int)sumG;
            retImage.b(i, j) = (int)sumB;
        }
    }
}

#endif
