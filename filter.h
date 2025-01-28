#ifndef FILTER_H
#define FILTER_H

#include "bmp.h"

void *filterThreadWorker(void * args);

void applyParallelSecondHalf(BMP_Image *imageIn, BMP_Image *imageOut, int numThreads);

#endif // FILTER_H