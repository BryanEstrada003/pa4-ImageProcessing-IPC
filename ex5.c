#include <stdlib.h>
#include <stdio.h>
#include "bmp.h"
#include "filter.h"

int boxFilter[3][3] = {
    {1, 1, 1},
    {1, 1, 1},
    {1, 1, 1}};

int main(int argc, char *argv[])
{
  if (argc != 3)
  {
    printf("Error: Argumentos inválidos. Uso: %s <input-bmp> <output-bmp>\n", argv[0]);
    return EXIT_FAILURE;
  }

  // Abrir el archivo de entrada
  FILE *srcFile = fopen(argv[1], "rb");
  if (!srcFile)
  {
    printf("Error: No se pudo abrir el archivo de origen '%s'.\n", argv[1]);
    return EXIT_FAILURE;
  }

  printf("Archivo de origen '%s' abierto correctamente.\n", argv[1]);

  // Crear una estructura BMP_Image y leer la imagen desde el archivo
  BMP_Image *image = createBMPImage(srcFile);
  if (!image)
  {
    printf("Error: No se pudo crear la estructura de la imagen BMP desde el archivo.\n");
    fclose(srcFile);
    return EXIT_FAILURE;
  }

  // Leer los datos de la imagen
  readImage(srcFile, image);
  printf("Datos de la imagen leídos correctamente desde el archivo de origen.\n");

  // Crear una nueva imagen para almacenar la imagen filtrada
  BMP_Image *filtered = malloc(sizeof(BMP_Image));
  if (!filtered)
  {
    printf("Error: Falló la asignación de memoria para la imagen filtrada.\n");
    freeImage(image);
    fclose(srcFile);
    return EXIT_FAILURE;
  }

  filtered->header = image->header;
  filtered->norm_height = image->norm_height;

  // Asignar memoria para los píxeles de la imagen filtrada
  filtered->pixels = malloc(filtered->header.height_px * sizeof(Pixel *));
  if (!filtered->pixels)
  {
    printf("Error: Falló la asignación de memoria para los píxeles de la imagen filtrada.\n");
    freeImage(image);
    free(filtered);
    fclose(srcFile);
    return EXIT_FAILURE;
  }

  for (int i = 0; i < filtered->header.height_px; i++)
  {
    filtered->pixels[i] = malloc(filtered->header.width_px * sizeof(Pixel));
    if (!filtered->pixels[i])
    {
      printf("Error: Falló la asignación de memoria para una fila de píxeles de la imagen filtrada.\n");
      freeImage(image);
      for (int j = 0; j < i; j++)
      {
        free(filtered->pixels[j]);
      }
      free(filtered->pixels);
      free(filtered);
      fclose(srcFile);
      return EXIT_FAILURE;
    }
  }

  // Aplicar el filtro a la imagen
  apply(image, filtered);
  printf("Filtro de imagen aplicado correctamente.\n");

  // Guardar la imagen filtrada en el archivo de salida
  writeImage(argv[2], filtered);
  printf("Imagen filtrada escrita correctamente en el archivo de salida '%s'.\n", argv[2]);

  // Liberar memoria
  freeImage(image);
  for (int i = 0; i < filtered->header.height_px; i++)
  {
    free(filtered->pixels[i]);
  }
  free(filtered->pixels);
  free(filtered);
  printf("Memoria de ambas imágenes liberada correctamente.\n");

  fclose(srcFile);

  return EXIT_SUCCESS;
}
