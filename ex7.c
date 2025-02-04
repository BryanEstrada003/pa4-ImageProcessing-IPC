#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <fcntl.h>
#include "bmp.h"
#include <math.h>
#include <pthread.h>

BMP_Image *createImageCopy(BMP_Image *image_in, void *shared_mem)
{
    BMP_Image *image_out = (BMP_Image *)shared_mem;
    image_out->norm_height = image_in->norm_height;
    image_out->bytes_per_pixel = image_in->bytes_per_pixel;
    image_out->header = image_in->header;
    image_out->pixels = (Pixel **)((char *)shared_mem + sizeof(BMP_Image));
    
    for (int i = 0; i < image_out->header.height_px; i++)
    {
        image_out->pixels[i] = (Pixel *)((char *)shared_mem + sizeof(BMP_Image) + image_out->header.height_px * sizeof(Pixel *) + i * image_out->header.width_px * image_out->bytes_per_pixel);
        memcpy(image_out->pixels[i], image_in->pixels[i], image_out->header.width_px * image_out->bytes_per_pixel);
    }

    return image_out;
}

//FILTRO BLUR

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
} BlurThreadArgs;

// Función del hilo
void *filterThreadWorker(void *args)
{
    BlurThreadArgs *threadArgs = (BlurThreadArgs *)args;
    BMP_Image *imageIn = threadArgs->imageIn;
    BMP_Image *imageOut = threadArgs->imageOut;
    int startRow = threadArgs->startRow;
    int endRow = threadArgs->endRow;
    int width = imageIn->header.width_px;

    printf("Blur Thread starting: startRow=%d, endRow=%d\n", startRow, endRow);

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

    printf("Blur Thread finished: startRow=%d, endRow=%d\n", startRow, endRow);
    return NULL;
}

void applyParallelFirstHalfBlur(BMP_Image *imageIn, BMP_Image *imageOut, int numThreads)
{
    pthread_t threads[numThreads];
    BlurThreadArgs threadArgs[numThreads];
    int height = imageIn->header.height_px;
    //int width = imageIn->header.width_px;
    int halfHeight = height / 2;                                               // Mitad de la imagen
    int rowsPerThread = ((height - halfHeight) + numThreads - 1) / numThreads; // Redondeo hacia arriba
    // Configurar y crear los hilos para procesar desde la mitad hasta la parte inferior
    for (int i = 0; i < numThreads; i++)
    {
        threadArgs[i].imageIn = imageIn;
        threadArgs[i].imageOut = imageOut;
        // Configurar los rangos de filas para cada hilo
        threadArgs[i].startRow = halfHeight + i * rowsPerThread; // Comienza en la mitad
        threadArgs[i].endRow = (i == numThreads - 1) ? height : halfHeight + (i + 1) * rowsPerThread;
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

    sem_t *sem = sem_open("/blur_semaphore", 0);
    if (sem == SEM_FAILED)
    {
        perror("Error opening semaphore");
        exit(EXIT_FAILURE);
    }
    sem_post(sem);
    sem_close(sem);


    printf("Blur Threads finished\n");

}

//FILTRO EDGE DETECTION

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
} EdgeThreadArgs;

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
    
    EdgeThreadArgs *threadArgs = (EdgeThreadArgs *)args;
    BMP_Image *imageIn = threadArgs->imageIn;
    BMP_Image *imageOut = threadArgs->imageOut;
    int width = imageIn->header.width_px;
    int startRow = threadArgs->startRow;
    int endRow = threadArgs->endRow;

    printf("Edge Detection Thread starting: startRow=%d, endRow=%d\n", startRow, endRow);


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

    printf("Edge Detection Thread finished: startRow=%d, endRow=%d\n", startRow, endRow);
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
    EdgeThreadArgs threadArgs[numThreads];
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


    printf("Edge Detection Threads finished\n");
}

int main()
{
    char inputFilePath[256];
    char outputFilePath[256];
    char inputNumThreads[256];
    int numThreads;

    while (1)
    {
        while (1)
        {
            printf("Enter input BMP file path (or 'ex' to exit): ");
            scanf("%s", inputFilePath);
            if (strcmp(inputFilePath, "ex") == 0)
            {
          break;
            }

            // Validate the file path
            if (access(inputFilePath, F_OK) == -1)
            {
          perror("Error: File does not exist");
          continue;
            }
            if (access(inputFilePath, R_OK) == -1)
            {
          perror("Error: No read permission for the file");
          continue;
            }
            break;
        }
        if (strcmp(inputFilePath, "ex") == 0)
        {
            break;
        }

        while (1)
        {
            printf("Enter output BMP file path (or 'ex' to exit): ");
            scanf("%s", outputFilePath);
            if (strcmp(outputFilePath, "ex") == 0)
            {
          break;
            }
            if (strlen(outputFilePath) < 4 || strcmp(outputFilePath + strlen(outputFilePath) - 4, ".bmp") != 0)
            {
          fprintf(stderr, "Error: Output file path must end with '.bmp'\n");
          continue;
            }
            break;
        }
        if (strcmp(outputFilePath, "ex") == 0)
        {
            break;
        }

        while (1)
        {
            printf("Enter number of threads (or 'ex' to exit): ");
            if (scanf("%s", inputNumThreads) == 1 && strcmp(inputNumThreads, "ex") == 0)
            {
            break;
            }
            if (sscanf(inputNumThreads, "%d", &numThreads) != 1)
            {
            fprintf(stderr, "Invalid input. Please enter an integer.\n");
            while (getchar() != '\n'); // Clear the input buffer
            continue;
            }
            if (numThreads <= 0 || numThreads > 19)
            {
            fprintf(stderr, "Number of threads must be a positive integer between 1 and 19.\n");
            continue;
            }
            break;
        }
        if (strcmp(inputNumThreads, "ex") == 0)
        {
            break;
        }

        FILE *source = fopen(inputFilePath, "rb");
        if (source == NULL)
        {
            perror("Error opening source file");
            continue;
        }

        BMP_Image *image_in;
        readImage(source, &image_in);
        if (image_in == NULL)
        {
            fclose(source);
            continue;
        }

        printf("Read image_in data %s\n", inputFilePath);
        printBMPHeader(&image_in->header);
        printBMPImage(image_in);

        if (!checkBMPValid(&image_in->header))
        {
            printError(VALID_ERROR);
            freeImage(image_in);
            fclose(source);
            continue;
        }

        printf("Create output image\n");

        key_t key = ftok("ruta/unica", 65);
        int shmid = shmget(key, 1024 * 1024, 0666 | IPC_CREAT);
        if (shmid == -1)
        {
            perror("Error al obtener memoria compartida");
            freeImage(image_in);
            fclose(source);
            continue;
        }

        void *shared_mem = shmat(shmid, NULL, 0);
        if (shared_mem == (void *)-1)
        {
            perror("Error al adjuntar memoria compartida");
            freeImage(image_in);
            fclose(source);
            continue;
        }

        // Copy image_in to shared memory
        BMP_Image *shared_image_in = (BMP_Image *)shared_mem;
        shared_image_in->norm_height = image_in->norm_height;
        shared_image_in->bytes_per_pixel = image_in->bytes_per_pixel;
        shared_image_in->header = image_in->header;
        shared_image_in->pixels = (Pixel **)((char *)shared_mem + sizeof(BMP_Image));
        
        for (int i = 0; i < shared_image_in->header.height_px; i++)
        {
            shared_image_in->pixels[i] = (Pixel *)((char *)shared_mem + sizeof(BMP_Image) + shared_image_in->header.height_px * sizeof(Pixel *) + i * shared_image_in->header.width_px * shared_image_in->bytes_per_pixel);
            memcpy(shared_image_in->pixels[i], image_in->pixels[i], shared_image_in->header.width_px * shared_image_in->bytes_per_pixel);
        }

        BMP_Image *image_out = createImageCopy(shared_image_in, (char *)shared_mem + 512 * 1024);
        if (image_out == NULL)
        {
            freeImage(image_in);
            fclose(source);
            continue;
        }

        // Store the number of threads in shared memory
        int *shared_numThreads = (int *)((char *)shared_mem + 1024 * 1024 - sizeof(int));
        *shared_numThreads = numThreads;

        printf("--------------------------------------------------------\n");
        printf("Copy image_in data to image_out\n");

        printBMPHeader(&image_out->header);
        printBMPImage(image_out);

        printf("Apply filters\n");

        // Crear y inicializar los semáforos
        sem_t *sem_blur = sem_open("/blur_semaphore", O_CREAT, 0644, 0);
        if (sem_blur == SEM_FAILED)
        {
            perror("Error creating blur semaphore");
            freeImage(image_in);
            fclose(source);
            continue;
        }

        sem_t *sem_edge = sem_open("/edge_semaphore", O_CREAT, 0644, 0);
        if (sem_edge == SEM_FAILED)
        {
            perror("Error creating edge semaphore");
            freeImage(image_in);
            fclose(source);
            continue;
        }

        pid_t pid_blur = fork();
        if (pid_blur == 0)
        {
            // Child process for blur filter
            printf("Child process: Executing blur with %d threads...\n", numThreads);
            applyParallelFirstHalfBlur(shared_image_in, image_out, numThreads);
            printf("Blur Filter applied.\n");
            exit(EXIT_SUCCESS);
        }
        else if (pid_blur < 0)
        {
            perror("Error creating blur filter process");
            freeImage(image_in);
            fclose(source);
            continue;
        }

        pid_t pid_edge = fork();
        if (pid_edge == 0)
        {
            // Child process for edge detection filter
            printf("Child process: Executing edge detection with %d threads...\n", numThreads);
            applyParallelSecondHalfEdge(shared_image_in, image_out, numThreads);
            printf("Edge Detection Filter applied.\n");
            exit(EXIT_SUCCESS);
        }
        else if (pid_edge < 0)
        {
            perror("Error creating edge detection filter process");
            freeImage(image_in);
            fclose(source);
            continue;
        }

        // Wait for the semaphore to be signaled by the child process
        sem_wait(sem_blur);
        sem_wait(sem_edge);

        // Wait for both child processes to finish
        waitpid(pid_blur, NULL, 0);
        printf("Parent process: Blur Child process finished\n");
        waitpid(pid_edge, NULL, 0);
        printf("Parent process: Edge Child process finished\n");


        // Reattach shared memory
        shared_image_in = (BMP_Image *)shared_mem;
        image_out = (BMP_Image *)((char *)shared_mem + 512 * 1024);

        printf("Write image in data %s\n", outputFilePath);
        FILE *dest = fopen(outputFilePath, "wb");
        if (dest == NULL)
        {
            perror("Error opening destination file");
            freeImage(image_in);
            fclose(source);
            continue;
        }

        writeImage(outputFilePath, image_out);

        freeImage(image_in);
        fclose(source);
        fclose(dest);

        // Cerrar y eliminar el semáforo
        sem_close(sem_edge);
        sem_unlink("/edge_semaphore");
        sem_close(sem_blur);
        sem_unlink("/blur_semaphore");

        // Detach shared memory
        shmdt(shared_mem);
    }

    return EXIT_SUCCESS;
}