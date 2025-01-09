#include "bmp.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

// Estructura para pasar datos a cada hilo
typedef struct
{
    BMP_Image *imageIn;
    BMP_Image *imageOut;
    int startRow;
    int endRow;
    int (*boxFilter)[3]; // Filtro de 3x3
} ThreadData;

// Aplicar filtro en una imagen de manera secuencial
void apply(BMP_Image *imageIn, BMP_Image *imageOut)
{
    // Filtro básico de 3x3
    int boxFilter[3][3] = {
        {1, 1, 1},
        {1, 1, 1},
        {1, 1, 1}};
    int width = imageIn->header.width_px;
    int height = imageIn->header.height_px;

    printf("    Aplicando el filtro a una imagen de tamaño %dx%d\n", width, height);

    for (int i = 1; i < height - 1; i++) // Recorrer de arriba hacia abajo
    {
        for (int j = 1; j < width - 1; j++)
        {
            int sumRed = 0, sumGreen = 0, sumBlue = 0;

            for (int m = -1; m <= 1; m++)
            {
                for (int n = -1; n <= 1; n++)
                {
                    int pY = i + m;
                    int pX = j + n;

                    // Verificar que el píxel esté dentro de los límites
                    Pixel *pixel = &imageIn->pixels[pY][pX];
                    sumRed += pixel->red * boxFilter[m + 1][n + 1];
                    sumGreen += pixel->green * boxFilter[m + 1][n + 1];
                    sumBlue += pixel->blue * boxFilter[m + 1][n + 1];
                }
            }

            // Asignar el promedio normalizado
            sumRed /= 9;
            sumGreen /= 9;
            sumBlue /= 9;

            imageOut->pixels[i][j].red = sumRed;
            imageOut->pixels[i][j].green = sumGreen;
            imageOut->pixels[i][j].blue = sumBlue;
        }
    }
}

// Función de trabajo para los hilos
void *filterThreadWorker(void *args)
{
    ThreadData *data = (ThreadData *)args;

    BMP_Image *imageIn = data->imageIn;
    BMP_Image *imageOut = data->imageOut;
    int(*boxFilter)[3] = data->boxFilter; // Obtener el filtro

    printf("Debug: Thread %ld processing rows from %d to %d\n", pthread_self(), data->startRow, data->endRow);

    for (int i = data->startRow; i < data->endRow; i++) // Procesar filas según rango
    {
        for (int j = 1; j < imageIn->header.width_px - 1; j++)
        {
            int sumRed = 0, sumGreen = 0, sumBlue = 0;

            for (int m = -1; m <= 1; m++)
            {
                for (int n = -1; n <= 1; n++)
                {
                    int pY = i + m;
                    int pX = j + n;

                    // Verificar que el píxel esté dentro de los límites
                    if (pY < 0 || pY >= imageIn->header.height_px || pX < 0 || pX >= imageIn->header.width_px)
                    {
                        continue; // Si el píxel está fuera de los límites, lo saltamos
                    }

                    Pixel *pixel = &imageIn->pixels[pY][pX];
                    sumRed += pixel->red * boxFilter[m + 1][n + 1];
                    sumGreen += pixel->green * boxFilter[m + 1][n + 1];
                    sumBlue += pixel->blue * boxFilter[m + 1][n + 1];
                }
            }

            // Asignar el promedio normalizado
            sumRed /= 9;
            sumGreen /= 9;
            sumBlue /= 9;

            imageOut->pixels[i][j].red = sumRed;
            imageOut->pixels[i][j].green = sumGreen;
            imageOut->pixels[i][j].blue = sumBlue;
        }
    }

    printf("Debug: Thread %ld finished processing rows from %d to %d\n", pthread_self(), data->startRow, data->endRow);
    pthread_exit(NULL);
}

// Función que aplica el filtro de forma paralela usando múltiples hilos
void applyParallel(BMP_Image *imageIn, BMP_Image *imageOut, int numThreads, int boxFilter[3][3])
{
    printf("Debug: Applying filter in parallel with %d threads...\n", numThreads);

    pthread_t threads[numThreads];
    ThreadData threadData[numThreads];

    int rowsPerThread = imageIn->header.height_px / numThreads;
    for (int i = 0; i < numThreads; i++)
    {
        threadData[i].imageIn = imageIn;
        threadData[i].imageOut = imageOut;
        threadData[i].startRow = i * rowsPerThread;
        threadData[i].endRow = (i == numThreads - 1) ? imageIn->header.height_px : (i + 1) * rowsPerThread;
        threadData[i].boxFilter = boxFilter;

        printf("Debug: Creating thread %d for rows %d to %d\n", i, threadData[i].startRow, threadData[i].endRow);

        pthread_create(&threads[i], NULL, filterThreadWorker, (void *)&threadData[i]);
    }

    // Esperar a que todos los hilos terminen
    for (int i = 0; i < numThreads; i++)
    {
        pthread_join(threads[i], NULL);
        printf("Debug: Thread %d joined.\n", i);
    }

    printf("Debug: Filter applied successfully.\n");
}
