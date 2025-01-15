#include "bmp.h"
#include <pthread.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// Filtro de caja 3x3
float boxFilter[3][3] = {
    {0.0625, 0.125, 0.0625},
    {0.125, 0.25, 0.125},
    {0.0625, 0.125, 0.0625}};

typedef struct
{
    BMP_Image *imageIn;
    BMP_Image *imageOut;
    int startRow;
    int endRow;
    float boxFilter[3][3];
} ThreadArgs;

// Aplica el filtro
void apply(BMP_Image *imageIn, BMP_Image *imageOut)
{
    int width = imageIn->header.width_px;
    int height = imageIn->header.height_px;

    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            Pixel *outPixel = &imageOut->pixels[y][x];
            if (y == 0 || y == height - 1 || x == 0 || x == width - 1)
            {
                // Borde negro
                outPixel->red = outPixel->green = outPixel->blue = 0;
            }
            else
            {
                // Aplicar filtro
                float sum[3] = {0, 0, 0};
                for (int ky = -1; ky <= 1; ky++)
                {
                    for (int kx = -1; kx <= 1; kx++)
                    {
                        Pixel *pixel = &imageIn->pixels[y + ky][x + kx];
                        float weight = boxFilter[ky + 1][kx + 1];
                        sum[0] += pixel->red * weight;
                        sum[1] += pixel->green * weight;
                        sum[2] += pixel->blue * weight;
                    }
                }
                outPixel->red = (int)sum[0];
                outPixel->green = (int)sum[1];
                outPixel->blue = (int)sum[2];
            }
        }
    }
}

// Aplica el filtro en la primera mitad de la imagen
void applyFirstHalf(BMP_Image *imageIn, BMP_Image *imageOut)
{
    int width = imageIn->header.width_px;
    int halfHeight = imageIn->header.height_px / 2;

    for (int y = 0; y < halfHeight; y++)
    {
        for (int x = 0; x < width; x++)
        {
            Pixel *outPixel = &imageOut->pixels[y][x];
            if (y == 0 || x == 0 || x == width - 1)
            {
                // Borde negro
                outPixel->red = outPixel->green = outPixel->blue = 0;
            }
            else
            {
                // Aplicar filtro
                float sum[3] = {0, 0, 0};
                for (int ky = -1; ky <= 1; ky++)
                {
                    for (int kx = -1; kx <= 1; kx++)
                    {
                        Pixel *pixel = &imageIn->pixels[y + ky][x + kx];
                        float weight = boxFilter[ky + 1][kx + 1];
                        sum[0] += pixel->red * weight;
                        sum[1] += pixel->green * weight;
                        sum[2] += pixel->blue * weight;
                    }
                }
                outPixel->red = (int)sum[0];
                outPixel->green = (int)sum[1];
                outPixel->blue = (int)sum[2];
            }
        }
    }
    // Copiar el resto de la imagen sin cambios
    for (int y = halfHeight; y < imageIn->header.height_px; y++)
    {
        memcpy(imageOut->pixels[y], imageIn->pixels[y], width * sizeof(Pixel));
    }
}

// Función del hilo
void *filterThreadWorker(void *args)
{
    ThreadArgs *threadArgs = (ThreadArgs *)args;
    BMP_Image *imageIn = threadArgs->imageIn;
    BMP_Image *imageOut = threadArgs->imageOut;
    int startRow = threadArgs->startRow;
    int endRow = threadArgs->endRow;
    int width = imageIn->header.width_px;

    for (int y = startRow; y < endRow; y++)
    {
        for (int x = 0; x < width; x++)
        {
            Pixel *outPixel = &imageOut->pixels[y][x];
            if (y == 0 || x == 0 || y == imageIn->header.height_px - 1 || x == width - 1)
            {
                outPixel->red = outPixel->green = outPixel->blue = 0;
            }
            else
            {
                float sum[3] = {0, 0, 0};
                for (int ky = -1; ky <= 1; ky++)
                {
                    for (int kx = -1; kx <= 1; kx++)
                    {
                        Pixel *pixel = &imageIn->pixels[y + ky][x + kx];
                        float weight = threadArgs->boxFilter[ky + 1][kx + 1];
                        sum[0] += pixel->red * weight;
                        sum[1] += pixel->green * weight;
                        sum[2] += pixel->blue * weight;
                    }
                }
                outPixel->red = (int)sum[0];
                outPixel->green = (int)sum[1];
                outPixel->blue = (int)sum[2];
            }
        }
    }
    return NULL;
}

// Aplicar filtro en paralelo
void applyParallel(BMP_Image *imageIn, BMP_Image *imageOut, int numThreads)
{
    pthread_t threads[numThreads];
    ThreadArgs threadArgs[numThreads];

    int height = imageIn->header.height_px;
    int rowsPerThread = height / numThreads;

    for (int i = 0; i < numThreads; i++)
    {
        threadArgs[i].imageIn = imageIn;
        threadArgs[i].imageOut = imageOut;
        threadArgs[i].startRow = i * rowsPerThread;
        threadArgs[i].endRow = (i == numThreads - 1) ? height : (i + 1) * rowsPerThread;
        memcpy(threadArgs[i].boxFilter, boxFilter, sizeof(float) * 9);

        pthread_create(&threads[i], NULL, filterThreadWorker, &threadArgs[i]);
    }

    for (int i = 0; i < numThreads; i++)
    {
        pthread_join(threads[i], NULL);
    }
}

void applyParallelFirstHalf(BMP_Image *imageIn, BMP_Image *imageOut, int numThreads)
{
    pthread_t threads[numThreads];
    ThreadArgs threadArgs[numThreads];

    int height = imageIn->header.height_px;
    int width = imageIn->header.width_px;
    int halfHeight = height / 2; // Mitad de la imagen
    int rowsPerThread = halfHeight / numThreads;

    // Configurar y crear los hilos para procesar desde la parte superior hasta la mitad
    for (int i = 0; i < numThreads; i++)
    {
        threadArgs[i].imageIn = imageIn;
        threadArgs[i].imageOut = imageOut;

        // Configurar los rangos de filas para cada hilo
        threadArgs[i].startRow = i * rowsPerThread; // Comienza en la parte superior
        threadArgs[i].endRow = (i == numThreads - 1) ? halfHeight : (i + 1) * rowsPerThread;

        // Copiar el filtro al argumento del hilo
        memcpy(threadArgs[i].boxFilter, boxFilter, sizeof(float) * 9);

        // Crear el hilo
        pthread_create(&threads[i], NULL, filterThreadWorker, &threadArgs[i]);
    }

    // Esperar a que todos los hilos terminen
    for (int i = 0; i < numThreads; i++)
    {
        pthread_join(threads[i], NULL);
    }

    // Copiar la mitad inferior de la imagen original a la imagen de salida sin cambios
    for (int y = halfHeight; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            imageOut->pixels[y][x] = imageIn->pixels[y][x];
        }
    }
}
