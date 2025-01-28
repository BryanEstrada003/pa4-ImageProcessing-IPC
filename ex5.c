#include <stdio.h>
#include <stdlib.h>
#include "bmp.h"
#include "blur.h"

void printPixelInfo(BMP_Image *image, int x, int y)
{
  if (image != NULL && image->pixels != NULL)
  {
    if (x >= 0 && x < image->header.width_px && y >= 0 && y < image->header.height_px)
    {
      Pixel *pixel = &image->pixels[y][x];
      printf("Pixel en (%d, %d): (R: %d, G: %d, B: %d)\n", x, y, pixel->red, pixel->green, pixel->blue);
    }
    else
    {
      fprintf(stderr, "Error: coordenadas del píxel fuera de los límites de la imagen\n");
    }
  }
  else
  {
    fprintf(stderr, "Error: la imagen o los píxeles no están correctamente inicializados\n");
  }
}

BMP_Image *createImageCopy(BMP_Image *image_in)
{
  BMP_Image *image_out = (BMP_Image *)malloc(sizeof(BMP_Image));
  if (image_out == NULL)
  {
    printError(MEMORY_ERROR);
    return NULL;
  }

  image_out->norm_height = image_in->norm_height;
  image_out->bytes_per_pixel = image_in->bytes_per_pixel;
  image_out->header = image_in->header;
  image_out->pixels = (Pixel **)malloc(image_out->header.height_px * sizeof(Pixel *));
  for (int i = 0; i < image_out->header.height_px; i++)
  {
    image_out->pixels[i] = (Pixel *)malloc(image_out->header.width_px * image_out->bytes_per_pixel);
  }

  return image_out;
}

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
    return EXIT_FAILURE;
  }

  printPixelInfo(image_in, 0, 0);
  printf("--------------------------------------------------------\n");
  
  printf("Create output image\n");
  BMP_Image *image_out = createImageCopy(image_in);
  if (image_out == NULL)
  {
    freeImage(image_in);
    fclose(source);
    return EXIT_FAILURE;
  }
  printf("--------------------------------------------------------\n");
  printf("Copy image_in data to image_out\n");

  printBMPHeader(&image_out->header);
  printBMPImage(image_out);

  printf("Apply filter\n");
  applyParallelFirstHalfBlur(image_in, image_out, 10);

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

  writeImage(argv[2], image_out);

  freeImage(image_in);
  freeImage(image_out);
  fclose(source);
  fclose(dest);

  return EXIT_SUCCESS;
}