#include <stdio.h>
#include <stdlib.h>
#include <stdint.h> /* define inteiros de tamanho específico */
#include <string.h>

#define K_THRESHOLDING 107

#pragma pack(push, 1) /* diz pro compilador não alterar alinhamento 
						ou tamanho da struct */
struct Pixel {
	uint8_t r;
	uint8_t g;
	uint8_t b;
};
/*struct Pixel{uint8_t r,g,b;};*/

struct BitmapHeader {
	uint16_t type;
	uint32_t size;
	uint16_t reserved1,
			 reserved2;
	uint32_t offset,
			 header_size;
	int32_t width,
			height;    
	uint16_t planes, 
			 bits;
	uint32_t compression,
			 imagesize;
	int32_t xresolution,
			yresolution;
	uint32_t ncolours,
			 importantcolours;
	uint32_t redbitmask,greenbitmask,bluebitmask,alphabitmask;
	uint32_t ColorSpaceType;
	uint32_t ColorSpaceEndPoints[9];
	uint32_t Gamma_Red,Gamma_Green,Gamma_Blue,intent,ICCProfileData,ICCProfileSize,Reserved;
};

typedef struct Bitmap {
	struct BitmapHeader bmp_header;
	struct Pixel **matrix;
} Bitmap;

#pragma pack(pop) /* restaura comportamento do compilador */

struct Pixel **loadBitmap(char *file_name, struct BitmapHeader *bH)
{
	FILE *imagem;
	
	imagem = fopen(file_name, "rb");
	
	if (imagem == NULL)
		perror("Erro ao abrir a imagem");
	
	fread(bH, sizeof(struct BitmapHeader), 1, imagem);
	
	struct Pixel **matrix;

	matrix = (struct Pixel **) malloc(sizeof(struct Pixel *) * bH->height);
	
	int i, j;
	
	for (i = 0; i < bH->height; i++) {
		matrix[i] = (struct Pixel *) malloc(sizeof(struct Pixel) * bH->width);
	}
	
	/* leitura */
	for (i = 0; i < bH->height; i++) {
		for (j = 0; j < bH->width; j++) {
			fread(&matrix[i][j], sizeof(struct Pixel), 1, imagem);
			int media = (matrix[i][j].r + matrix[i][j].g + matrix[i][j].b) / 3;
			int val = 0;
			if (media > K_THRESHOLDING)
			{
				val = 255;
			}
			matrix[i][j].r = val; 
			matrix[i][j].g = val;
			matrix[i][j].b = val;
		}
	}

	fclose(imagem);

	return matrix;
}

void saveBitmap(char *fileName, struct BitmapHeader bH, struct Pixel **matrix)
{
	FILE *nova = fopen(fileName, "wb");

	fwrite(&bH, sizeof(struct BitmapHeader), 1, nova);

	/* escrita */
	for (int i = 0; i < bH.height; i++) {
		for (int j = 0; j < bH.width; j++) {
			fwrite(&matrix[i][j], sizeof(struct Pixel), 1, nova);
		}
	}

	fclose(nova);
}

void freeMatrix(struct BitmapHeader bH, struct Pixel **matrix)
{
	for (int i = 0; i < bH.height; i++) 
	{
		free(matrix[i]);
	}
	free(matrix);
}

void printBitmap(struct BitmapHeader bH, struct Pixel **matrix)
{
	for (int i = 0; i < bH.height; i++) {
		for (int j = 0; j < bH.width; j++) {
			printf("%d ", matrix[i][j].r);
		}
		printf("\n");
	}	
}

int getLinhaInicial(struct BitmapHeader bH, struct Pixel **matrix)
{
	//nao considera imagens vazias
	int i = bH.height - 1;
	while (matrix[i][10].r == 0)
	{
		--i;
	}
	return bH.height - i - 1;
}

int getLinhaFinal(struct BitmapHeader bH, struct Pixel **matrix)
{
	//nao considera imagens vazias
	int i = 0;
	while (matrix[i][10].r == 0)
	{
		++i;
	}
	return bH.height - i - 1;
}

void getWoodNodes(int init_line, int end_line, Bitmap bitmap)
{
	char **flag_matrix;
	int size = end_line - init_line;
	flag_matrix = (char **)malloc(sizeof(char *) * size);
	for (int i = 0; i < size; ++i)
	{
		flag_matrix[i] = (char *)malloc(sizeof(char) * bitmap.bmp_header.width);
	}

	for (int i = init_line; i < end_line; ++i)
	{
		for (int j = 0; i < bitmap.bmp_header.width; ++j)
		{
			
			if (bitmap.matrix[i][j] == 0 && bitmap.matrix[i])
			{

			}
		}
	}

	//free memory
	for (int i = 0; i < size; i++) 
	{
		free(flag_matrix[i]);
	}
	free(flag_matrix);
}

int main() {
	
	struct Bitmap bitmap;
	char *file_name = "st1012.bmp";
	char dir[18] = "validos/";
	strcat(dir, file_name);
	bitmap.matrix = loadBitmap(dir, &bitmap.bmp_header);
	//printBitmap(bH, matrix);
	// printf("Image size = %d x %d\n", bH.width, bH.height);
	saveBitmap("nova_imagem.bmp", bitmap.bmp_header, bitmap.matrix);

	int init_line = getLinhaInicial(bitmap.bmp_header, bitmap.matrix);
	int end_line = getLinhaFinal(bitmap.bmp_header, bitmap.matrix);
	getWoodNodes(init_line, end_line, bitmap);

	printf("%s\t%d %d\n", file_name, init_line, end_line);
	freeMatrix(bitmap.bmp_header, bitmap.matrix);
	return 0;
}
