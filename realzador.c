#include <stdio.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <string.h>

void laplacianEdgeDetection(unsigned char *input, unsigned char *output, int width, int height) {
    int kernel[3][3] = {
        { 0,  1,  0 },
        { 1, -4,  1 },
        { 0,  1,  0 }
    };

    // Aplicar el kernel Laplaciano a la segunda mitad de la imagen
    for (int y = 1; y < height - 1; y++) {
        for (int x = 1; x < width - 1; x++) {
            int sum = 0;

            // Solo procesar la segunda mitad de la imagen
            if (y >= height / 2) {
                for (int j = -1; j <= 1; j++) {
                    for (int i = -1; i <= 1; i++) {
                        sum += input[(y + j) * width + (x + i)] * kernel[j + 1][i + 1];
                    }
                }

                // Convertir la suma en un valor válido de píxel (0-255)
                sum = abs(sum);
                if (sum > 255) sum = 255;

                output[y * width + x] = (unsigned char)sum;
            }
        }
    }
}

int main() {
    key_t key = ftok("ruta/unica", 65);
    int shmid = shmget(key, 1024 * 1024, 0666);
    if (shmid == -1) {
        perror("Error al obtener memoria compartida");
        exit(EXIT_FAILURE);
    }

    unsigned char *shared_mem = (unsigned char *)shmat(shmid, NULL, 0);
    if (shared_mem == (void *)-1) {
        perror("Error al adjuntar memoria compartida");
        exit(EXIT_FAILURE);
    }

    int width, height;
    memcpy(&width, shared_mem, sizeof(int));
    memcpy(&height, shared_mem + sizeof(int), sizeof(int));

    unsigned char *input = shared_mem + 2 * sizeof(int);
    unsigned char *output = shared_mem + 2 * sizeof(int);

    printf("[DEBUG] Realzador (Laplaciano) iniciado. Procesando segunda mitad de la imagen...\n");
    laplacianEdgeDetection(input, output, width, height);
    printf("[DEBUG] Realzador completado.\n");

    shmdt(shared_mem);
    return 0;
}
