#include <stdlib.h>
#include <stdio.h>
#include "bmp.h"

/* FUNCION PARA IMPRIMIR MENSAJES DE ERROR
   NO MODIFIQUES ESTA FUNCIÓN
*/
void printError(int error)
{
  switch (error)
  {
  case ARGUMENT_ERROR:
    printf("  Uso: ex5 <origen> <destino>\n");
    break;
  case FILE_ERROR:
    printf("  ¡No se pudo abrir el archivo!\n");
    break;
  case MEMORY_ERROR:
    printf("  ¡No se pudo asignar memoria!\n");
    break;
  case VALID_ERROR:
    printf("  ¡Archivo BMP no válido!\n");
    break;
  default:
    break;
  }
}

/* Esta función construye una imagen BMP al asignar memoria para ella.
 * Luego lee la cabecera del archivo BMP y calcula el tamaño de los datos,
 * las dimensiones de la imagen y los bytes por píxel, y los almacena
 * como atributos de la imagen. Finalmente, asigna memoria para los
 * datos de la imagen según el tamaño calculado.
 */
BMP_Image *createBMPImage(FILE *fptr)
{
  printf("  Creando imagen BMP...\n");

  BMP_Image *image = malloc(sizeof(BMP_Image));
  if (!image)
  {
    printError(MEMORY_ERROR);
    return NULL;
  }

  // Leemos la cabecera BMP
  printf("  Leyendo la cabecera del BMP...\n");
  if (fread(&image->header, sizeof(BMP_Header), 1, fptr) != 1)
  {
    printf("  Error al leer la cabecera del BMP.\n");
    printError(FILE_ERROR);
    free(image);
    return NULL;
  }

  // Mostramos la cabecera para depuración
  printf("  Cabecera BMP leída correctamente.\n");
  printBMPHeader(&image->header);

  // Validamos el archivo BMP
  if (!checkBMPValid(&image->header))
  {
    printf("  ¡Error: archivo BMP no válido!\n");
    printError(VALID_ERROR);
    free(image);
    return NULL;
  }

  // Calculamos bytes por píxel y altura normalizada
  image->bytes_per_pixel = image->header.bits_per_pixel / 8;
  image->norm_height = abs(image->header.height_px);

  printf("  Dimensiones de la imagen: %d x %d\n", image->header.width_px, image->header.height_px);

  // Asignamos memoria para los píxeles de la imagen
  printf("  Asignando memoria para los píxeles... (altura: %d, ancho: %d)\n", image->norm_height, image->header.width_px);

  image->pixels = malloc(image->norm_height * sizeof(Pixel *));
  if (!image->pixels)
  {
    printError(MEMORY_ERROR);
    free(image);
    return NULL;
  }

  // Asignamos memoria para cada fila de píxeles
  for (int i = 0; i < image->norm_height; i++)
  {
    image->pixels[i] = malloc(image->header.width_px * sizeof(Pixel));
    if (!image->pixels[i])
    {
      printf("  Error al asignar memoria para la fila %d\n", i);
      printError(MEMORY_ERROR);
      free(image->pixels);
      free(image);
      return NULL;
    }
  }

  printf("  Imagen BMP creada correctamente.\n");

  return image;
}

/* Función para leer los datos de la imagen BMP desde el archivo */
void readImage(FILE *srcFile, BMP_Image *dataImage)
{
  printf("  Leyendo los datos de la imagen BMP...\n");

  // Movemos el puntero del archivo a la posición donde empiezan los datos
  fseek(srcFile, dataImage->header.offset, SEEK_SET);
  printf("  Puntero de archivo movido a desplazamiento de datos: %d\n", dataImage->header.offset);

  // Leemos los datos de cada fila de píxeles
  for (int i = 0; i < dataImage->norm_height; i++)
  {
    if (fread(dataImage->pixels[i], sizeof(Pixel), dataImage->header.width_px, srcFile) != dataImage->header.width_px)
    {
      printf("  Error al leer los datos de píxeles para la fila %d\n", i);
      printError(FILE_ERROR);
      exit(EXIT_FAILURE);
    }
  }

  printf("  Datos de la imagen BMP leídos correctamente.\n");
}

/* Función para escribir la imagen BMP a un archivo de destino */
void writeImage(char *destFileName, BMP_Image *dataImage)
{
  printf("  Escribiendo la imagen al archivo de destino %s...\n", destFileName);

  FILE *destFile = fopen(destFileName, "wb");
  if (destFile == NULL)
  {
    printError(FILE_ERROR);
    exit(EXIT_FAILURE);
  }

  // Escribimos la cabecera BMP
  fwrite(&dataImage->header, sizeof(BMP_Header), 1, destFile);

  // Escribimos los datos de la imagen (filas de píxeles)
  int dataSize = dataImage->header.width_px * dataImage->header.bits_per_pixel / 8;
  for (int i = 0; i < dataImage->header.height_px; i++)
  {
    fwrite(dataImage->pixels[i], dataSize, 1, destFile);
  }

  fclose(destFile);
  printf("  Imagen escrita al archivo de destino con éxito.\n");
}

/* Función para liberar la memoria de la imagen BMP */
void freeImage(BMP_Image *image)
{
  printf("  Liberando memoria de la imagen...\n");

  // Liberamos memoria de cada fila de píxeles
  for (int i = 0; i < image->header.height_px; i++)
  {
    free(image->pixels[i]);
  }

  // Liberamos memoria de los punteros de píxeles y de la estructura BMP_Image
  free(image->pixels);
  free(image);

  printf("  Memoria de la imagen liberada.\n");
}

/* The functions checks if the source image has a valid format.
 * It returns TRUE if the image is valid, and returns FASLE if the image is not valid.
 * DO NOT MODIFY THIS FUNCTION
 */
int checkBMPValid(BMP_Header *header)
{
  // Make sure this is a BMP file
  if (header->type != 0x4d42)
  {
    printf("  Error: Not a BMP file!\n");
    return FALSE;
  }
  /*
  // Make sure we are getting 24 bits per pixel
  if (header->bits_per_pixel != 24 || header->header_size != 40 || header->compression != 0) 
  {
    printf("  Error: Not a 24-bit BMP image!\n");
    return FALSE;
  }
  */
  
  // Make sure there is only one image plane
  if (header->planes != 1)
  {
    printf("  Error: BMP image is not single-plane!\n");
    return FALSE;
  }
  // Make sure there is no compression
  if (header->compression != 0)
  {
    printf("  Error: BMP image is compressed!\n");
    return FALSE;
  }
  return TRUE;
}

/* The function prints all information of the BMP_Header.
   DO NOT MODIFY THIS FUNCTION
*/
void printBMPHeader(BMP_Header *header)
{
  printf("  file type (should be 0x4d42): %x\n", header->type);
  printf("  file size: %d\n", header->size);
  printf("  offset to image data: %d\n", header->offset);
  printf("  header size: %d\n", header->header_size);
  printf("  width_px: %d\n", header->width_px);
  printf("  height_px: %d\n", header->height_px);
  printf("  planes: %d\n", header->planes);
  printf("  bits: %d\n", header->bits_per_pixel);
}

/* The function prints information of the BMP_Image.
   DO NOT MODIFY THIS FUNCTION
*/
void printBMPImage(BMP_Image *image)
{
  printf("  data size is %ld\n", sizeof(image->pixels));
  printf("  norm_height size is %d\n", image->norm_height);
  printf("  bytes per pixel is %d\n", image->bytes_per_pixel);
}
