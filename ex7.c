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
#include "blur.h"
#include "edge.h"

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
            if (numThreads <= 0)
            {
          fprintf(stderr, "Number of threads must be a positive integer.\n");
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

        printf("Apply filter\n");

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

        applyParallelFirstHalfBlur(shared_image_in, image_out, numThreads);
        printf("Blur Filter applied.\n");
        applyParallelSecondHalfEdge(shared_image_in, image_out, numThreads);
        printf("Edge Filter applied.\n");


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