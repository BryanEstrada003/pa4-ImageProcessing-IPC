#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>

void cargarImagen(const char *ruta, unsigned char *buffer, int *width, int *height, unsigned char *header, int *stride) {
    FILE *fp = fopen(ruta, "rb");
    if (!fp) {
        perror("Error al abrir la imagen");
        exit(EXIT_FAILURE);
    }

    printf("[DEBUG] Archivo abierto correctamente: %s\n", ruta);

    fread(header, sizeof(unsigned char), 54, fp);

    *width = *(int *)&header[18];
    *height = *(int *)&header[22];
    *stride = ((*width * 3 + 3) & ~3); // Alinear el ancho a múltiplos de 4 bytes

    fread(buffer, sizeof(unsigned char), (*stride) * (*height), fp);
    fclose(fp);

    printf("[DEBUG] Imagen cargada: %dx%d, Stride: %d\n", *width, *height, *stride);
}

void guardarImagen(const char *ruta, unsigned char *buffer, int height, unsigned char *header, int stride) {
    FILE *fp = fopen(ruta, "wb");
    if (!fp) {
        perror("Error al guardar la imagen");
        exit(EXIT_FAILURE);
    }

    printf("[DEBUG] Guardando imagen procesada: %s\n", ruta);

    // Actualizar campos clave del encabezado BMP
    *(int *)&header[2] = 54 + (stride * height); // bfSize: Tamaño total del archivo
    *(int *)&header[34] = stride * height;      // biSizeImage: Tamaño de los datos de la imagen

    // Escribir el encabezado actualizado
    fwrite(header, sizeof(unsigned char), 54, fp);

    // Escribir los datos procesados con alineación de stride
    fwrite(buffer, sizeof(unsigned char), stride * height, fp);
    fclose(fp);

    printf("[DEBUG] Imagen guardada correctamente\n");
}

int main() {
    key_t key = ftok("ruta/unica", 65);
    int shmid = shmget(key, 1024 * 1024, 0666 | IPC_CREAT);
    if (shmid == -1) {
        perror("Error al crear memoria compartida");
        exit(EXIT_FAILURE);
    }

    unsigned char *shared_mem = (unsigned char *)shmat(shmid, NULL, 0);
    if (shared_mem == (void *)-1) {
        perror("Error al adjuntar memoria compartida");
        exit(EXIT_FAILURE);
    }

    unsigned char header[54];

    // Crear el directorio ./outputs si no existe
    struct stat st = {0};
    if (stat("./outputs", &st) == -1) {
        if (mkdir("./outputs", 0777) == -1) {
            perror("Error al crear el directorio ./outputs");
            exit(EXIT_FAILURE);
        }
        printf("[DEBUG] Directorio ./outputs creado correctamente\n");
    } else {
        printf("[DEBUG] El directorio ./outputs ya existe\n");
    }

    while (1) {
        printf("Ingrese la ruta de la imagen (o 'salir' para terminar): ");
        char ruta[256];
        if (scanf("%s", ruta) == EOF) {
            perror("Error al leer la entrada del usuario");
            exit(EXIT_FAILURE);
        }

        if (strcmp(ruta, "salir") == 0) {
            printf("Finalizando programa...\n");
            break;
        }

        int width, height, stride;
        if (access(ruta, F_OK) != 0) {
            printf("Error: Archivo no encontrado. Intente nuevamente.\n");
            continue;
        }

        cargarImagen(ruta, shared_mem + 2 * sizeof(int), &width, &height, header, &stride);

        memcpy(shared_mem, &width, sizeof(int));
        memcpy(shared_mem + sizeof(int), &height, sizeof(int));

        printf("[DEBUG] Imagen cargada en memoria compartida\n");

        /* if (fork() == 0) {
            printf("[DEBUG] Proceso hijo para desenfocador creado\n");
            execl("./desenfocador", "./desenfocador", NULL);
            perror("Error al ejecutar desenfocador");
            exit(EXIT_FAILURE);
        }

        if (fork() == 0) {
            printf("[DEBUG] Proceso hijo para realzador creado\n");
            execl("./realzador", "./realzador", NULL);
            perror("Error al ejecutar realzador");
            exit(EXIT_FAILURE);
        }

        wait(NULL);
        printf("[DEBUG] Proceso hijo para desenfocador terminado\n");
        wait(NULL);
        printf("[DEBUG] Proceso hijo para realzador terminado\n"); */

        // Generar el nombre de archivo de salida con el sufijo "_output"
        char salida[256];
        snprintf(salida, sizeof(salida), "./outputs/%s_output.bmp", strtok(ruta, "."));
        guardarImagen(salida, shared_mem + 2 * sizeof(int), height, header, stride);
    }

    shmdt(shared_mem);
    shmctl(shmid, IPC_RMID, NULL);

    return 0;
}
