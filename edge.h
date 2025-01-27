#ifndef EDGE_H
#define EDGE_H

#include "bmp.h"


// Aplica el filtro de detección de bordes
void applyEdgeDetection(BMP_Image *imageIn, BMP_Image *imageOut);

// Función del hilo
void *edgeDetectionThreadWorker(void *args);

// Aplicar detección de bordes en paralelo
void applyParallelEdgeDetection(BMP_Image *imageIn, BMP_Image *imageOut, int numThreads);

// Aplicar detección de bordes en la segunda mitad de la imagen en paralelo
void applyParallelSecondHalfEdgeDetection(BMP_Image *imageIn, BMP_Image *imageOut, int numThreads);

#endif // EDGE_H
