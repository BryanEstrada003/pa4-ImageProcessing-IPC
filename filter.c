#include "bmp.h"
#include <pthread.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

int boxFilter[3][3] = {
    {0.0625, 0.125, 0.0625},
    {0.125, 0.25, 0.125},
    {0.0625, 0.125, 0.0625}};

typedef struct
{
    BMP_Image *imageIn;
    BMP_Image *imageOut;
    int startRow;
    int endRow;
    int boxFilter[3][3];
} ThreadArgs;

/* Aplica el filtro de caja a la imagen de entrada y escribe el resultado en la imagen de salida.
   Maneja bordes colocando negro por defecto.
*/
void apply(BMP_Image *imageIn, BMP_Image *imageOut)
{
    printf("Funcion apply");
    int width = imageIn->header.width_px;
    int height = imageIn->header.height_px;

    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {

            Pixel *outPixel = &imageOut->pixels[y][x];

            // Si está en el borde, píxel negro
            if (y == 0 || y == height - 1 || x == 0 || x == width - 1)
            {
                outPixel->red = 0;
                outPixel->green = 0;
                outPixel->blue = 0;
            }
            else
            {
                // Filtro 3x3 en la zona central
                int sum[3] = {0, 0, 0};
                for (int ky = -1; ky <= 1; ky++)
                {
                    for (int kx = -1; kx <= 1; kx++)
                    {
                        Pixel *pixel = &imageIn->pixels[y + ky][x + kx];
                        sum[0] += pixel->red;
                        sum[1] += pixel->green;
                        sum[2] += pixel->blue;
                    }
                }
                outPixel->red = sum[0] / 9;
                outPixel->green = sum[1] / 9;
                outPixel->blue = sum[2] / 9;
            }
        }
    }
}
/* Esta función es la rutina que ejecutarán los hilos.
   Maneja bordes colocando negro por defecto.
*/
void *filterThreadWorker(void *args)
{
    ThreadArgs *threadArgs = (ThreadArgs *)args;
    BMP_Image *imageIn = threadArgs->imageIn;
    BMP_Image *imageOut = threadArgs->imageOut;
    int startRow = threadArgs->startRow;
    int endRow = threadArgs->endRow;
    int width = imageIn->header.width_px;
    int height = imageIn->header.height_px;

    for (int y = startRow; y < endRow; y++)
    {
        for (int x = 0; x < width; x++)
        {
            int sum[3] = {0, 0, 0};
            int validCount = 0;

            for (int ky = -1; ky <= 1; ky++)
            {
                for (int kx = -1; kx <= 1; kx++)
                {
                    int yy = y + ky;
                    int xx = x + kx;

                    if (yy >= 0 && yy < height && xx >= 0 && xx < width)
                    {
                        // Píxel dentro de los límites
                        Pixel *pixel = &imageIn->pixels[yy][xx];
                        sum[0] += pixel->red * threadArgs->boxFilter[ky + 1][kx + 1];
                        sum[1] += pixel->green * threadArgs->boxFilter[ky + 1][kx + 1];
                        sum[2] += pixel->blue * threadArgs->boxFilter[ky + 1][kx + 1];
                        validCount++;
                    }
                }
            }

            Pixel *outPixel = &imageOut->pixels[y][x];
            if (validCount > 0)
            {
                outPixel->red = sum[0] / validCount;
                outPixel->green = sum[1] / validCount;
                outPixel->blue = sum[2] / validCount;
            }
            else
            {
                // Negro por defecto
                outPixel->red = 0;
                outPixel->green = 0;
                outPixel->blue = 0;
            }
        }
    }

    return NULL;
}

/*  Divide la imagen en secciones y lanza varios hilos para procesar cada sección en paralelo.
    Cada hilo ejecuta filterThreadWorker para aplicar el filtro a su sección de la imagen.
*/
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
        memcpy(threadArgs[i].boxFilter, boxFilter, sizeof(int) * 9);

        pthread_create(&threads[i], NULL, filterThreadWorker, &threadArgs[i]);
    }

    for (int i = 0; i < numThreads; i++)
    {
        pthread_join(threads[i], NULL);
    }
}