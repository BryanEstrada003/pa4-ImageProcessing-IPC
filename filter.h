#include "bmp.h"

void apply(BMP_Image *imageIn, BMP_Image *imageOut);

void applyParallel(BMP_Image *imageIn, BMP_Image *imageOut, int numThreads, int boxFilter[3][3]);

void *filterThreadWorker(void *args);