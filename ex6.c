#include <stdio.h>
#include <stdlib.h>
#include "bmp.h"
#include "edge.h" // Include edge.h for applyParallelEdgeDetection

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

  printf("--------------------------------------------------------\n");

  // Create an output image with the same header as the input image
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

  // Apply the filter
  printf("Apply filter\n");
  applyParallelSecondHalfEdge(image_in, image_out, 20);

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