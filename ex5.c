#include <stdio.h>
#include <stdlib.h>
#include "bmp.h"
#include "filter.h"

int main(int argc, char* argv[]) {
  if (argc != 3) {
    fprintf(stderr, "Usage: %s <source BMP file> <destination BMP file>\n", argv[0]);
    return EXIT_FAILURE;
  }

  FILE* source = fopen(argv[1], "rb");
  if (source == NULL) {
    perror("Error opening source file");
    return EXIT_FAILURE;
  }
  printf("Create BMP img\n");
  BMP_Image* image_in = NULL;
  
  printf("--------------------------------------------------------\n");
  readImage(source, &image_in);
  if (image_in == NULL) {
    fclose(source);
    return EXIT_FAILURE;
  }

  printf("Read image_in data %s\n", argv[1]);
  printBMPHeader(&image_in->header);
  printBMPImage(image_in);

  if (!checkBMPValid(&image_in->header)) {
    printError(VALID_ERROR);
    freeImage(image_in);
    fclose(source);
    exit(EXIT_FAILURE);
  }
  printf("--------------------------------------------------------\n");

  // Create an output image with the same header as the input image
  printf("Create output image\n");
  BMP_Image* imageOut = (BMP_Image*)malloc(sizeof(BMP_Image));
  if (imageOut == NULL) {
    printError(MEMORY_ERROR);
    freeImage(image_in);
    fclose(source);
    return EXIT_FAILURE;
  }
  printf("--------------------------------------------------------\n");
  printf("Copy image_in data to imageOut\n");
  imageOut->header = image_in->header;
  imageOut->norm_height = image_in->norm_height;
  imageOut->bytes_per_pixel = image_in->bytes_per_pixel;
  imageOut->pixels = (Pixel**)malloc(image_in->header.height_px * sizeof(Pixel*));
  for (int i = 0; i < image_in->header.height_px; i++) {
    imageOut->pixels[i] = (Pixel*)malloc(image_in->header.width_px * sizeof(Pixel));
  }

  printBMPHeader(&imageOut->header);
  printBMPImage(imageOut);
  printf("-----------------------------------------");
  printPixelMatrix(image_in);
  printf("-----------------------------------------");
  printPixelMatrix(imageOut);


  // Apply the filter
  printf("Apply filter\n");
  //int numThreads = 10; // You can adjust the number of threads as needed
  apply(image_in, imageOut);

  printf("Write image_in data %s\n", argv[2]);
  FILE* dest = fopen(argv[2], "wb");
  if (dest == NULL) {
    perror("Error opening destination file");
    freeImage(image_in);
    fclose(source);
    return EXIT_FAILURE;
  }

  writeImage(argv[2], image_in); // Pasar el nombre del archivo en lugar del puntero al archivo

  freeImage(image_in);
  fclose(source);
  fclose(dest);

  return EXIT_SUCCESS;
}