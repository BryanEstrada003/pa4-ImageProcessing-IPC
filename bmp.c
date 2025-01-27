#include <stdlib.h>
#include <stdio.h>

#include "bmp.h"
/* USE THIS FUNCTION TO PRINT ERROR MESSAGES
   DO NOT MODIFY THIS FUNCTION
*/
void printError(int error)
{
  switch (error)
  {
  case ARGUMENT_ERROR:
    printf("  Usage:ex5 <source> <destination>\n");
    break;
  case FILE_ERROR:
    printf("  Unable to open file!\n");
    break;
  case MEMORY_ERROR:
    printf("  Unable to allocate memory!\n");
    break;
  case VALID_ERROR:
    printf("  BMP file not valid!\n");
    break;
  default:
    break;
  }
}

/* The input argument is the source file pointer. The function will first construct a BMP_Image image by allocating memory to it.
 * Then the function read the header from source image to the image's header.
 * Compute data size, width, height, and bytes_per_pixel of the image and stores them as image's attributes.
 * Finally, allocate menory for image's data according to the image size.
 * Return image;
 */
BMP_Image *createBMPImage(FILE *fptr)
{

  // Allocate memory for BMP_Image*
  BMP_Image *image = (BMP_Image *)malloc(sizeof(BMP_Image));
  if (image == NULL)
  {
    printf(" ");
    printError(MEMORY_ERROR);
    return NULL;
  }

  // Read the first 54 bytes of the source into the header
  printf("  Reading header\n");
  if (fread(&(image->header), sizeof(BMP_Header), 1, fptr) != 1)
  {
    printf(" ");
    printError(FILE_ERROR);
    free(image);
    return NULL;
  }

  // Compute data size, width, height, and bytes per pixel
  int width = image->header.width_px;
  int height = image->header.height_px;
  int bytesPerPixel = image->header.bits_per_pixel / 8;

  image->norm_height = abs(height);
  image->bytes_per_pixel = bytesPerPixel;

  // Allocate memory for image data
  image->pixels = (Pixel **)malloc(height * sizeof(Pixel *));
  if (image->pixels == NULL)
  {
    printError(MEMORY_ERROR);
    free(image);
    return NULL;
  }

  for (int i = 0; i < height; i++)
  {
    image->pixels[i] = (Pixel *)malloc(width * sizeof(Pixel));
    if (image->pixels[i] == NULL)
    {
      printError(MEMORY_ERROR);
      for (int j = 0; j < i; j++)
      {
        free(image->pixels[j]);
      }
      free(image->pixels);
      free(image);
      return NULL;
    }
  }

  // Read the image data
  printf("  Reading image data\n");
  readImageData(fptr, image);
  if (image->pixels == NULL)
  {
    freeImage(image);
    return NULL;
  }

  return image;
}

/* The input arguments are the source file pointer, the image data pointer, and the size of image data.
 * The functions reads data from the source into the image data matriz of pixels.
 */
void readImageData(FILE *fptr, BMP_Image *image)
{
  int rowSize = (image->header.width_px * image->bytes_per_pixel + 3) & ~3; // Row size is padded to the nearest multiple of 4 bytes
  for (int i = 0; i < image->header.height_px; i++)
  {
    if (fread(image->pixels[i], rowSize, 1, fptr) != 1)
    {
      printError(FILE_ERROR);
      for (int j = 0; j <= i; j++)
      {
        free(image->pixels[j]);
      }
      free(image->pixels);
      image->pixels = NULL;
      return;
    }
  }
}

/* The input arguments are the pointer of the binary file, and the image data pointer.
 * The functions open the source file and call to CreateBMPImage to load de data image.
 */
void readImage(FILE *srcFile, BMP_Image **dataImage)
{
  *dataImage = createBMPImage(srcFile);
  if (*dataImage == NULL)
  {
    printf(" ");
    printError(FILE_ERROR);
  }
}

/* The input arguments are the destination file name, and BMP_Image pointer.
 * The function write the header and image data into the destination file.
 */
void writeImage(char *destFileName, BMP_Image *dataImage)
{
  FILE *destFile = fopen(destFileName, "wb");
  if (destFile == NULL)
  {
    printf(" ");
    printError(FILE_ERROR);
    return;
  }

  // Write the header to the destination file
  if (fwrite(&(dataImage->header), sizeof(BMP_Header), 1, destFile) != 1)
  {
    printf(" ");
    printError(FILE_ERROR);
    fclose(destFile);
    return;
  }

  // Write the image data to the destination file
  int rowSize = (dataImage->header.width_px * dataImage->bytes_per_pixel + 3) & ~3;
  dataImage->header.imagesize = rowSize * abs(dataImage->header.height_px);

  for (int i = 0; i < dataImage->header.height_px; i++)
  {
    if (fwrite(dataImage->pixels[i], rowSize, 1, destFile) != 1)
    {
      printf(" ");
      printError(FILE_ERROR);
      fclose(destFile);
      return;
    }
  }

  fclose(destFile);
}

/* The input argument is the BMP_Image pointer. The function frees memory of the BMP_Image.
 */
void freeImage(BMP_Image *image)
{
  if (image != NULL)
  {
    if (image->pixels != NULL)
    {
      for (int i = 0; i < image->header.height_px; i++)
      {
        free(image->pixels[i]);
      }
      free(image->pixels);
    }
    free(image);
  }
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
    return FALSE;
  }
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
  int rowSize = (image->header.width_px * image->bytes_per_pixel + 3) & ~3;
  int dataSize = rowSize * abs(image->header.height_px);
  printf("  data size is %d\n", dataSize);
  printf("  norm_height size is %d\n", image->norm_height);
  printf("  bytes per pixel is %d\n", image->bytes_per_pixel);
}

/* The function prints the pixel matrix of the BMP_Image.
 */
void printPixelMatrix(BMP_Image *image)
{
  int width = image->header.width_px;
  int height = image->header.height_px;

  for (int y = 0; y < height; y++)
  {
    for (int x = 0; x < width; x++)
    {
      Pixel *pixel = &image->pixels[y][x];
      printf("(%d, %d, %d) ", pixel->red, pixel->green, pixel->blue);
    }
    printf("\n");
  }
}
