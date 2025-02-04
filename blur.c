#include "bmp.h"
#include <pthread.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>
#include <semaphore.h>
#include <fcntl.h>

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

// Función del hilo
void *filterThreadWorker(void *args)
{
    ThreadArgs *threadArgs = (ThreadArgs *)args;
    BMP_Image *imageIn = threadArgs->imageIn;
    BMP_Image *imageOut = threadArgs->imageOut;
    int startRow = threadArgs->startRow;
    int endRow = threadArgs->endRow;
    int width = imageIn->header.width_px;

    printf("Thread starting: startRow=%d, endRow=%d\n", startRow, endRow);

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
                        Pixel *inPixel = &imageIn->pixels[y + ky][x + kx];
                        sum[0] += inPixel->red * threadArgs->boxFilter[ky + 1][kx + 1];
                        sum[1] += inPixel->green * threadArgs->boxFilter[ky + 1][kx + 1];
                        sum[2] += inPixel->blue * threadArgs->boxFilter[ky + 1][kx + 1];
                    }
                }
                outPixel->red = (unsigned char)sum[0];
                outPixel->green = (unsigned char)sum[1];
                outPixel->blue = (unsigned char)sum[2];
            }
        }
    }

    printf("Thread  finished: startRow=%d, endRow=%d\n", startRow, endRow);
    return NULL;
}

void applyParallelFirstHalfBlur(BMP_Image *imageIn, BMP_Image *imageOut, int numThreads)
{
    pthread_t threads[numThreads];
    ThreadArgs threadArgs[numThreads];

    int height = imageIn->header.height_px;
    int halfHeight = height / 2;
    int rowsPerThread = (halfHeight + numThreads - 1) / numThreads;

    for (int i = 0; i < numThreads; i++)
    {
        threadArgs[i].imageIn = imageIn;
        threadArgs[i].imageOut = imageOut;
        threadArgs[i].startRow = i * rowsPerThread;
        threadArgs[i].endRow = (i == numThreads - 1) ? halfHeight : (i + 1) * rowsPerThread;
        memcpy(threadArgs[i].boxFilter, boxFilter, sizeof(float) * 9);
        printf("Creating thread %d: startRow=%d, endRow=%d\n", i, threadArgs[i].startRow, threadArgs[i].endRow);
        int rc = pthread_create(&threads[i], NULL, filterThreadWorker, &threadArgs[i]);
        if (rc)
        {
            fprintf(stderr, "Error creating thread %d: %d\n", i, rc);
            exit(EXIT_FAILURE);
        }
    }

    printf("Waiting for threads to finish\n");

    for (int i = 0; i < numThreads; i++) {
    int rc = pthread_join(threads[i], NULL);
    if (rc) {
        fprintf(stderr, "Error joining thread %d: %d\n", i, rc);
        exit(EXIT_FAILURE);
    }


    sem_t *sem = sem_open("/blur_semaphore", 0);
    if (sem == SEM_FAILED)
    {
        perror("Error opening semaphore");
        exit(EXIT_FAILURE);
    }
    sem_post(sem);
    sem_close(sem);

    printf("Thread %d finished\n", i);
}

printf("All threads finished\n");
}

int main()
{
    printf("ENTRE AL MAIN DE BLUR\n");
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

    printf("Applying blur filter with %d threads...\n", numThreads);
    applyParallelFirstHalfBlur(imageIn, imageOut, numThreads);
    printf("Filter applied.\n");

    // Detach shared memory
   /*  shmdt(shared_mem); */

    return 0;
}