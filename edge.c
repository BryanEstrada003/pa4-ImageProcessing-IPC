#include "bmp.h"
#include <pthread.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h> // Include math.h for sqrt function
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>
#include <semaphore.h>
#include <fcntl.h>

// Prewitt operator masks
const int prewittX[3][3] = {
    {-1, 0, 1},
    {-1, 0, 1},
    {-1, 0, 1}};

const int prewittY[3][3] = {
    {-1, -1, -1},
    {0, 0, 0},
    {1, 1, 1}};

typedef struct
{
    BMP_Image *imageIn;
    BMP_Image *imageOut;
    int startRow;
    int endRow;
    const int (*prewittX)[3];
    const int (*prewittY)[3];
} ThreadArgs;

// Clamps the value to the range [0, 255]
int clamp(int value)
{
    return value < 0 ? 0 : (value > 255 ? 255 : value);
}

// Ensure the BMP image structure is valid
int validateBMPImage(BMP_Image *image)
{
    return image != NULL && image->pixels != NULL &&
           image->header.width_px > 0 && image->header.height_px > 0;
}

// Worker thread function
void *edgeDetectionThreadWorker(void *args)
{
    printf("Thread starting\n");
    
    ThreadArgs *threadArgs = (ThreadArgs *)args;
    BMP_Image *imageIn = threadArgs->imageIn;
    BMP_Image *imageOut = threadArgs->imageOut;
    int width = imageIn->header.width_px;
    int startRow = threadArgs->startRow;
    int endRow = threadArgs->endRow;

    for (int y = startRow; y < endRow; y++)
    {
        for (int x = 1; x < width - 1; x++)
        {
            Pixel *outPixel = &imageOut->pixels[y][x];
            int sumX[3] = {0}, sumY[3] = {0};
            if (y == 0 || x == 0 || y == imageIn->header.height_px - 1 || x == width - 1)
            {
                outPixel->red = outPixel->green = outPixel->blue = 0;
            }
            else
            {
                for (int ky = -1; ky <= 1; ky++)
                {
                    for (int kx = -1; kx <= 1; kx++)
                    {
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

    printf("Thread finished\n");
    return NULL;
}

void applyParallelSecondHalfEdge(BMP_Image *imageIn, BMP_Image *imageOut, int numThreads)
{
    if (!validateBMPImage(imageIn) || !validateBMPImage(imageOut))
    {
        fprintf(stderr, "Invalid BMP image structure for parallel processing.\n");
        return;
    }

    int height = imageIn->header.height_px;
    int halfHeight = height / 2; // Mitad de la imagen

    // Validar número de hilos
    int rowsToProcess = halfHeight; // Filas en la mitad superior
    if (numThreads > rowsToProcess)
    {
        numThreads = rowsToProcess; // Ajustar si hay más hilos que filas
    }

    pthread_t threads[numThreads];
    ThreadArgs threadArgs[numThreads];
    int rowsPerThread = rowsToProcess / numThreads;
    int extraRows = rowsToProcess % numThreads;

    // Crear hilos para la mitad superior
    for (int i = 0; i < numThreads; i++)
    {
        threadArgs[i].imageIn = imageIn;
        threadArgs[i].imageOut = imageOut;
        threadArgs[i].prewittX = prewittX;
        threadArgs[i].prewittY = prewittY;

        threadArgs[i].startRow = i * rowsPerThread;
        threadArgs[i].endRow = threadArgs[i].startRow + rowsPerThread;

        if (i == numThreads - 1)
        {
            threadArgs[i].endRow += extraRows; // Asignar filas adicionales al último hilo
        }

        // Validación de límites
        if (threadArgs[i].endRow > halfHeight)
        {
            threadArgs[i].endRow = halfHeight;
        }

        if (pthread_create(&threads[i], NULL, edgeDetectionThreadWorker, &threadArgs[i]) != 0)
        {
            fprintf(stderr, "Error creating thread %d\n", i);
            exit(EXIT_FAILURE);
        }
    }


    // Esperar a que los hilos terminen
    printf("Waiting for threads to finish\n");

    for (int i = 0; i < numThreads; i++)
    {
        if (pthread_join(threads[i], NULL) != 0)
        {
            fprintf(stderr, "Error joining thread %d\n", i);
            exit(EXIT_FAILURE);
        }
    }


    sem_t *sem = sem_open("/edge_semaphore", 0);
    if (sem == SEM_FAILED)
    {
        perror("Error opening semaphore");
        exit(EXIT_FAILURE);
    }
    sem_post(sem);
    sem_close(sem);


    printf("Threads finished\n");
}


int main()
{
    printf("ENTRE AL MAIN DE EDGE\n");
    key_t key = ftok("ruta/unica", 65);
    int shmid = shmget(key, 1024 * 1024, 0666);
    if (shmid == -1)
    {
        perror("Error al obtener memoria compartida");
        return EXIT_FAILURE;
    }

    void *shared_mem = shmat(shmid, NULL, 0);
    if (shared_mem == (void *)-1)
    {
        perror("Error al adjuntar memoria compartida");
        return EXIT_FAILURE;
    }

    BMP_Image *imageIn = (BMP_Image *)shared_mem;
    BMP_Image *imageOut = (BMP_Image *)((char *)shared_mem + 512 * 1024);

    // Obtener el número de hilos de la memoria compartida
    int *shared_numThreads = (int *)((char *)shared_mem + 1024 * 1024 - sizeof(int));
    int numThreads = *shared_numThreads;

    printf("Applying edge detection filter with %d threads...\n", numThreads);
    applyParallelSecondHalfEdge(imageIn, imageOut, numThreads);
    printf("Edge detection filter applied.\n");

    // Detach shared memory
    shmdt(shared_mem);

    return 0;
}