#ifndef EDGE_H
#define EDGE_H

#include "bmp.h"

// Función del hilo
void *edgeDetectionThreadWorker(void *args);

// Aplicar detección de bordes en la segunda mitad de la imagen en paralelo
void applyParallelSecondHalfEdge(BMP_Image *imageIn, BMP_Image *imageOut, int numThreads);

#endif // EDGE_H
