#include "bmp.h"
#include <pthread.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h> // Include math.h for sqrt function

// Prewitt operator masks
int prewittX[3][3] = {
    {-1, 0, 1},
    {-1, 0, 1},
    {-1, 0, 1}};

int prewittY[3][3] = {
    {-1, -1, -1},
    {0, 0, 0},
    {1, 1, 1}};

typedef struct {
    BMP_Image *imageIn;
    BMP_Image *imageOut;
    int startRow;
    int endRow;
    int prewittX[3][3];
    int prewittY[3][3];
} ThreadArgs;

// Clamps the value to the range [0, 255]
int clamp(int value) {
    if (value < 0) return 0;
    if (value > 255) return 255;
    return value;
}

// Normalize the gradient magnitude to the range [0, 255]
int normalize(int value, int maxGradient) {
    if (maxGradient == 0) return 0; // Avoid division by zero
    return clamp((value * 255) / maxGradient);
}

// Ensure the BMP image structure is valid
int validateBMPImage(BMP_Image *image) {
    return image != NULL && image->pixels != NULL && image->header.width_px > 0 && image->header.height_px > 0;
}

// Aplica el filtro de detección de bordes
void applyEdgeDetection(BMP_Image *imageIn, BMP_Image *imageOut) {
    if (!validateBMPImage(imageIn) || !validateBMPImage(imageOut)) {
        fprintf(stderr, "Invalid BMP image structure.\n");
        return;
    }

    int width = imageIn->header.width_px;
    int height = imageIn->header.height_px;

    // Determine the maximum gradient for normalization
    int maxGradient = 0;
    for (int y = 1; y < height - 1; y++) {
        for (int x = 1; x < width - 1; x++) {
            int sumX[3] = {0, 0, 0};
            int sumY[3] = {0, 0, 0};
            for (int ky = -1; ky <= 1; ky++) {
                for (int kx = -1; kx <= 1; kx++) {
                    Pixel *pixel = &imageIn->pixels[y + ky][x + kx];
                    int weightX = prewittX[ky + 1][kx + 1];
                    int weightY = prewittY[ky + 1][kx + 1];
                    sumX[0] += pixel->red * weightX;
                    sumX[1] += pixel->green * weightX;
                    sumX[2] += pixel->blue * weightX;
                    sumY[0] += pixel->red * weightY;
                    sumY[1] += pixel->green * weightY;
                    sumY[2] += pixel->blue * weightY;
                }
            }
            int gradient = (int)sqrt(sumX[0] * sumX[0] + sumY[0] * sumY[0]);
            if (gradient > maxGradient) {
                maxGradient = gradient;
            }
        }
    }

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            Pixel *outPixel = &imageOut->pixels[y][x];
            if (y == 0 || y == height - 1 || x == 0 || x == width - 1) {
                // Borde negro
                outPixel->red = outPixel->green = outPixel->blue = 0;
            } else {
                // Aplicar filtro de Prewitt
                int sumX[3] = {0, 0, 0};
                int sumY[3] = {0, 0, 0};
                for (int ky = -1; ky <= 1; ky++) {
                    for (int kx = -1; kx <= 1; kx++) {
                        Pixel *pixel = &imageIn->pixels[y + ky][x + kx];
                        int weightX = prewittX[ky + 1][kx + 1];
                        int weightY = prewittY[ky + 1][kx + 1];
                        sumX[0] += pixel->red * weightX;
                        sumX[1] += pixel->green * weightX;
                        sumX[2] += pixel->blue * weightX;
                        sumY[0] += pixel->red * weightY;
                        sumY[1] += pixel->green * weightY;
                        sumY[2] += pixel->blue * weightY;
                    }
                }
                outPixel->red = normalize((int)sqrt(sumX[0] * sumX[0] + sumY[0] * sumY[0]), maxGradient);
                outPixel->green = normalize((int)sqrt(sumX[1] * sumX[1] + sumY[1] * sumY[1]), maxGradient);
                outPixel->blue = normalize((int)sqrt(sumX[2] * sumX[2] + sumY[2] * sumY[2]), maxGradient);
            }
        }
    }
}

// Función del hilo
void *edgeDetectionThreadWorker(void *args) {
    ThreadArgs *threadArgs = (ThreadArgs *)args;
    BMP_Image *imageIn = threadArgs->imageIn;
    BMP_Image *imageOut = threadArgs->imageOut;
    int startRow = threadArgs->startRow;
    int endRow = threadArgs->endRow;
    int width = imageIn->header.width_px;

    if (!validateBMPImage(imageIn) || !validateBMPImage(imageOut)) {
        fprintf(stderr, "Invalid BMP image structure in thread.\n");
        return NULL;
    }

    for (int y = startRow; y < endRow; y++) {
        for (int x = 0; x < width; x++) {
            Pixel *outPixel = &imageOut->pixels[y][x];
            if (y == 0 || x == 0 || y == imageIn->header.height_px - 1 || x == width - 1) {
                outPixel->red = outPixel->green = outPixel->blue = 0;
            } else {
                int sumX[3] = {0, 0, 0};
                int sumY[3] = {0, 0, 0};
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
    }
    return NULL;
}

// Aplicar detección de bordes en paralelo
void applyParallelEdgeDetection(BMP_Image *imageIn, BMP_Image *imageOut, int numThreads) {
    pthread_t threads[numThreads];
    ThreadArgs threadArgs[numThreads];

    if (!validateBMPImage(imageIn) || !validateBMPImage(imageOut)) {
        fprintf(stderr, "Invalid BMP image structure for parallel processing.\n");
        return;
    }

    int height = imageIn->header.height_px;
    int rowsPerThread = height / numThreads;

    for (int i = 0; i < numThreads; i++) {
        threadArgs[i].imageIn = imageIn;
        threadArgs[i].imageOut = imageOut;
        threadArgs[i].startRow = i * rowsPerThread;
        threadArgs[i].endRow = (i == numThreads - 1) ? height : (i + 1) * rowsPerThread;
        memcpy(threadArgs[i].prewittX, prewittX, sizeof(int) * 9);
        memcpy(threadArgs[i].prewittY, prewittY, sizeof(int) * 9);

        pthread_create(&threads[i], NULL, edgeDetectionThreadWorker, &threadArgs[i]);
    }

    for (int i = 0; i < numThreads; i++) {
        pthread_join(threads[i], NULL);
    }
}
