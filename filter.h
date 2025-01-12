#ifndef FILTER_H
#define FILTER_H

#include "bmp.h"

void apply(BMP_Image * imageIn, BMP_Image * imageOut);

void applyParallel(BMP_Image * imageIn, BMP_Image * imageOut, int numThreads);

void *filterThreadWorker(void * args);

void printPixelMatrix(BMP_Image *image);

#endif // FILTER_H