#include "bmp.h"
#include <pthread.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h> // Include math.h for sqrt function

// Prewitt operator masks
const int prewittX[3][3] = {
    {-1, 0, 1},
    {-1, 0, 1},
    {-1, 0, 1}};

const int prewittY[3][3] = {
    {-1, -1, -1},
    {0, 0, 0},
    {1, 1, 1}};

typedef struct {
    BMP_Image *imageIn;
    BMP_Image *imageOut;
    int startRow;
    int endRow;
    const int (*prewittX)[3];
    const int (*prewittY)[3];
} ThreadArgs;

// Clamps the value to the range [0, 255]
int clamp(int value) {
    return value < 0 ? 0 : (value > 255 ? 255 : value);
}

// Ensure the BMP image structure is valid
int validateBMPImage(BMP_Image *image) {
    return image != NULL && image->pixels != NULL &&
           image->header.width_px > 0 && image->header.height_px > 0;
}

// Worker thread function
void *edgeDetectionThreadWorker(void *args) {
    ThreadArgs *threadArgs = (ThreadArgs *)args;
    BMP_Image *imageIn = threadArgs->imageIn;
    BMP_Image *imageOut = threadArgs->imageOut;
    int width = imageIn->header.width_px;
    int startRow = threadArgs->startRow;
    int endRow = threadArgs->endRow;

    for (int y = startRow; y < endRow; y++) {
        for (int x = 1; x < width - 1; x++) {
            Pixel *outPixel = &imageOut->pixels[y][x];
            int sumX[3] = {0}, sumY[3] = {0};

            for (int ky = -1; ky <= 1; ky++) {
                for (int kx = -1; kx <= 1; kx++) {
                    Pixel *pixel = &imageIn->pixels[y + ky][x + kx];
                    int weightX = threadArgs->prewittX[ky + 1][kx + 1];
                    int weightY = threadArgs->prewittY[ky + 1][kx + 1];

                    sumX[0] += pixel->red * weightX;
                    sumX[1] += pixel->green * weightX;
                    sumX[2] += pixel->blue * weightX;

                    sumY[0] += pixel->red * weightY;
                    sumY[1] += pixel->green * weightY;
                    sumY[2] += pixel->blue * weightY;
                }
            }

            outPixel->red = clamp((int)sqrt(sumX[0] * sumX[0] + sumY[0] * sumY[0]));
            outPixel->green = clamp((int)sqrt(sumX[1] * sumX[1] + sumY[1] * sumY[1]));
            outPixel->blue = clamp((int)sqrt(sumX[2] * sumX[2] + sumY[2] * sumY[2]));
        }
    }
    return NULL;
}

// Parallel edge detection
void applyParallelEdgeDetection(BMP_Image *imageIn, BMP_Image *imageOut, int numThreads) {
    if (!validateBMPImage(imageIn) || !validateBMPImage(imageOut)) {
        fprintf(stderr, "Invalid BMP image structure.\n");
        return;
    }

    pthread_t threads[numThreads];
    ThreadArgs threadArgs[numThreads];
    int height = imageIn->header.height_px;
    int rowsPerThread = height / numThreads;
    int extraRows = height % numThreads;

    for (int i = 0; i < numThreads; i++) {
        threadArgs[i].imageIn = imageIn;
        threadArgs[i].imageOut = imageOut;
        threadArgs[i].prewittX = prewittX;
        threadArgs[i].prewittY = prewittY;
        threadArgs[i].startRow = i * rowsPerThread;
        threadArgs[i].endRow = (i + 1) * rowsPerThread;

        if (i == numThreads - 1) {
            threadArgs[i].endRow += extraRows;
        }

        if (pthread_create(&threads[i], NULL, edgeDetectionThreadWorker, &threadArgs[i]) != 0) {
            fprintf(stderr, "Error creating thread %d\n", i);
            exit(EXIT_FAILURE);
        }
    }

    for (int i = 0; i < numThreads; i++) {
        if (pthread_join(threads[i], NULL) != 0) {
            fprintf(stderr, "Error joining thread %d\n", i);
            exit(EXIT_FAILURE);
        }
    }
}

void applyParallelSecondHalfEdgeDetection(BMP_Image *imageIn, BMP_Image *imageOut, int numThreads) {
    if (!validateBMPImage(imageIn) || !validateBMPImage(imageOut)) {
        fprintf(stderr, "Invalid BMP image structure for parallel processing.\n");
        return;
    }

    pthread_t threads[numThreads];
    ThreadArgs threadArgs[numThreads];

    int height = imageIn->header.height_px;
    int width = imageIn->header.width_px;
    int halfHeight = height / 2;
    int rowsPerThread = (height - halfHeight) / numThreads;
    int extraRows = (height - halfHeight) % numThreads;

    if (numThreads > (height - halfHeight)) {
        numThreads = height - halfHeight;  // Ajustar si hay más hilos que filas
    }

    for (int i = 0; i < numThreads; i++) {
        threadArgs[i].imageIn = imageIn;
        threadArgs[i].imageOut = imageOut;
        threadArgs[i].prewittX = prewittX;
        threadArgs[i].prewittY = prewittY;
        threadArgs[i].startRow = halfHeight + i * rowsPerThread;
        threadArgs[i].endRow = threadArgs[i].startRow + rowsPerThread;

        if (i == numThreads - 1) {
            threadArgs[i].endRow += extraRows;  // Último hilo maneja filas restantes
        }

        if (threadArgs[i].endRow > height) {
            threadArgs[i].endRow = height;  // Limitar al máximo de la imagen
        }

        printf("Hilo %d: startRow = %d, endRow = %d\n", i, threadArgs[i].startRow, threadArgs[i].endRow);

        if (pthread_create(&threads[i], NULL, edgeDetectionThreadWorker, &threadArgs[i]) != 0) {
            fprintf(stderr, "Error creating thread %d\n", i);
            exit(EXIT_FAILURE);
        }
    }

    for (int i = 0; i < numThreads; i++) {
        if (pthread_join(threads[i], NULL) != 0) {
            fprintf(stderr, "Error joining thread %d\n", i);
            exit(EXIT_FAILURE);
        }
    }

    // Copiar la mitad superior de la imagen sin cambios
    for (int y = 0; y < halfHeight; y++) {
        for (int x = 0; x < width; x++) {
            imageOut->pixels[y][x] = imageIn->pixels[y][x];
        }
    }
}
