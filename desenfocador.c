#include <stdio.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <string.h>

void desenfoqueRGB(unsigned char *input, unsigned char *output, int width, int height, int stride) {
    int kernel[3][3] = {
        {1, 1, 1},
        {1, 1, 1},
        {1, 1, 1}
    };
    int kernelSum = 9;

    // Buffer temporal para evitar sobrescritura
    unsigned char *tempOutput = malloc(stride * height);
    if (!tempOutput) {
        perror("Error al asignar memoria para el buffer temporal");
        exit(EXIT_FAILURE);
    }
    memcpy(tempOutput, input, stride * height);

    // Procesar la primera mitad de la imagen
    for (int y = 1; y < height / 2 - 1; y++) {
        for (int x = 1; x < width - 1; x++) {
            for (int channel = 0; channel < 3; channel++) { // Procesar canales R, G, B
                int sum = 0;
                for (int j = -1; j <= 1; j++) {
                    for (int i = -1; i <= 1; i++) {
                        int pixelIndex = ((y + j) * stride) + ((x + i) * 3) + channel;
                        sum += input[pixelIndex] * kernel[j + 1][i + 1];
                    }
                }
                int outputIndex = (y * stride) + (x * 3) + channel;
                tempOutput[outputIndex] = (unsigned char)(sum / kernelSum);
            }
        }
    }

    // Copiar el buffer temporal a la salida
    memcpy(output, tempOutput, stride * height);
    free(tempOutput);
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

    int width, height, stride;
    memcpy(&width, shared_mem, sizeof(int));
    memcpy(&height, shared_mem + sizeof(int), sizeof(int));
    stride = ((width * 3 + 3) & ~3); // Calcular el stride (alineación a múltiplos de 4 bytes)

    unsigned char *input = shared_mem + 2 * sizeof(int);
    unsigned char *output = shared_mem + 2 * sizeof(int);

    printf("[DEBUG] Desenfocador iniciado. Procesando primera mitad de la imagen...\n");
    desenfoqueRGB(input, output, width, height, stride);
    printf("[DEBUG] Desenfocador completado.\n");

    shmdt(shared_mem);
    return 0;
}
