#include <stdio.h>
#include <stdlib.h>
#include "bmp.h"

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
  BMP_Image* image = NULL;
  readImage(source, &image);
  if (image == NULL) {
    fclose(source);
    return EXIT_FAILURE;
  }
  printf("--------------------------------------------------------\n");

  printf("Read image data %s\n", argv[1]);
  printBMPHeader(&image->header);
  printBMPImage(image);

  if (!checkBMPValid(&image->header)) {
    printError(VALID_ERROR);
    freeImage(image);
    fclose(source);
    exit(EXIT_FAILURE);
  }
  printf("--------------------------------------------------------\n");

  printf("Write image data %s\n", argv[2]);
  FILE* dest = fopen(argv[2], "wb");
  if (dest == NULL) {
    perror("Error opening destination file");
    freeImage(image);
    fclose(source);
    return EXIT_FAILURE;
  }

  writeImage(argv[2], image); // Pasar el nombre del archivo en lugar del puntero al archivo

  freeImage(image);
  fclose(source);
  fclose(dest);

  return EXIT_SUCCESS;
}