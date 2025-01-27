#include <stdio.h>
#include <stdlib.h>
#include "bmp.h"
#include "filter.h"

int main(int argc, char *argv[])
{
  if (argc != 3)
  {
    fprintf(stderr, "Usage: %s <source BMP file> <destination BMP file>\n", argv[0]);
    return EXIT_FAILURE;
  }

  FILE *source = fopen(argv[1], "rb");
  if (source == NULL)
  {
    perror("Error opening source file");
    return EXIT_FAILURE;
  }
  printf("Create BMP img\n");
  BMP_Image *image_in = (BMP_Image *)malloc(sizeof(BMP_Image));
  if (image_in == NULL)
  {
    printError(MEMORY_ERROR);
    fclose(source);
    return EXIT_FAILURE;
  }

  printf("--------------------------------------------------------\n");
  readImage(source, &image_in);
  if (image_in == NULL)
  {
    fclose(source);
    return EXIT_FAILURE;
  }

  printf("Read image_in data %s\n", argv[1]);
  printBMPHeader(&image_in->header);
  printBMPImage(image_in);

  if (!checkBMPValid(&image_in->header))
  {
    printError(VALID_ERROR);
    freeImage(image_in);
    fclose(source);
    exit(EXIT_FAILURE);
  }

  // Suponiendo que quieres ver el píxel en la posición (x, y)
  int x = 0; // Coordenada x del píxel
  int y = 0; // Coordenada y del píxel

  if (image_in != NULL && image_in->pixels != NULL)
  {
    if (x >= 0 && x < image_in->header.width_px && y >= 0 && y < image_in->header.height_px)
    {
      Pixel *pixel = &image_in->pixels[y][x];
      printf("Pixel en (%d, %d): (R: %d, G: %d, B: %d)\n", x, y, pixel->red, pixel->green, pixel->blue);
    }
    else
    {
      fprintf(stderr, "Error: coordenadas del píxel fuera de los límites de la image_inn\n");
    }
  }
  else
  {
    fprintf(stderr, "Error: la image_in o los píxeles no están correctamente inicializados\n");
  }
  printf("--------------------------------------------------------\n");

  // Create an output image with the same header as the input image
  printf("Create output image\n");
  BMP_Image *image_out = (BMP_Image *)malloc(sizeof(BMP_Image));
  if (image_out == NULL)
  {
    printError(MEMORY_ERROR);
    freeImage(image_out);
    fclose(source);
    return EXIT_FAILURE;
  }
  printf("--------------------------------------------------------\n");
  printf("Copy image_in data to image_out\n");
  // ...existing code...

  image_out->norm_height = image_in->norm_height;
  image_out->bytes_per_pixel = image_in->bytes_per_pixel;
  image_out->header = image_in->header;
  image_out->pixels = (Pixel **)malloc(image_out->header.height_px * sizeof(Pixel *));
  for (int i = 0; i < image_out->header.height_px; i++)
  {
    image_out->pixels[i] = (Pixel *)malloc(image_out->header.width_px * image_out->bytes_per_pixel);
  };
  image_out->header.size = image_in->header.size;
  image_out->header.imagesize = image_in->header.imagesize;
  image_out->header.width_px = image_in->header.width_px;
  image_out->header.height_px = image_in->header.height_px;
  image_out->header.bits_per_pixel = image_in->header.bits_per_pixel;
  image_out->header.offset = image_in->header.offset;
  image_out->header.header_size = image_in->header.header_size;
  image_out->header.planes = image_in->header.planes;
  image_out->header.compression = image_in->header.compression;
  image_out->header.xresolution = image_in->header.xresolution;
  image_out->header.yresolution = image_in->header.yresolution;
  image_out->header.ncolours = image_in->header.ncolours;
  image_out->header.importantcolours = image_in->header.importantcolours;
  image_out->header.type = image_in->header.type;
  image_out->header.reserved1 = image_in->header.reserved1;
  image_out->header.reserved2 = image_in->header.reserved2;

  // ...existing code...

  printBMPHeader(&image_out->header);
  printBMPImage(image_out);

  // Apply the filter
  printf("Apply filter\n");
  // int numThreads = 10; // You can adjust the number of threads as needed
  // apply(image_in, image_out);
  applyParallelSecondHalf(image_in, image_out, 10);

  printf("Write image_in data %s\n", argv[2]);
  FILE *dest = fopen(argv[2], "wb");
  if (dest == NULL)
  {
    perror("Error opening destination file");
    freeImage(image_in);
    freeImage(image_out);
    fclose(source);
    return EXIT_FAILURE;
  }

  writeImage(argv[2], image_out); // Pasar el nombre del archivo en lugar del puntero al archivo

  freeImage(image_in);
  fclose(source);
  fclose(dest);

  return EXIT_SUCCESS;
}