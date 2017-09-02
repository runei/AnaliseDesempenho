#include <stdio.h>
#include <stdlib.h>
#include <stdint.h> /* define inteiros de tamanho específico */
#include <string.h>

#define K_WOOD 107
#define WHITE 255
#define BLACK 0
#define SIZE_STACK 50

#pragma pack(push, 1) /* diz pro compilador não alterar alinhamento 
						ou tamanho da struct */
struct Pixel {
	uint8_t b;
	uint8_t g;
	uint8_t r;
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
	
	if (!fread(bH, sizeof(struct BitmapHeader), 1, imagem))
	{
		perror("Erro ao ler bitmap header");	
	}
	
	struct Pixel **matrix;

	matrix = (struct Pixel **) malloc(sizeof(struct Pixel *) * bH->height);
	
	int i, j;
	
	for (i = 0; i < bH->height; i++) {
		matrix[i] = (struct Pixel *) malloc(sizeof(struct Pixel) * bH->width);
	}
	/* leitura */
	for (i = 0; i < bH->height; i++) {
		for (j = 0; j < bH->width; j++) {
			if (!fread(&matrix[i][j], sizeof(struct Pixel), 1, imagem))
			{
//				perror("Erro ao ler imagem");
				continue;	
			}
			int media = (matrix[i][j].r + matrix[i][j].g + matrix[i][j].b) / 3;
			int val = BLACK;
			if (media > K_WOOD)
			{
				val = WHITE;
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

int invertPosMatrix(int height, int val)
{
	return height - val - 1;
}

int getLinhaInicial(struct BitmapHeader bH, struct Pixel **matrix)
{
	//nao considera imagens vazias
	int i = bH.height - 1;
	while (matrix[i][10].r == BLACK)
	{
		--i;
	}
	return invertPosMatrix(bH.height, i);
}

int getLinhaFinal(struct BitmapHeader bH, struct Pixel **matrix)
{
	//nao considera imagens vazias
	int i = 0;
	while (matrix[i][10].r == BLACK)
	{
		++i;
	}
	return invertPosMatrix(bH.height, i);
}

int checkErosion(int i, int j, struct Pixel **matrix)
{
	for (int k = -1; k < 2; ++k)
	{
		for (int l = -1; l < 2; ++l)
		{
			if (matrix[i-k][j-l].r == WHITE)
			{
				return 0;
			}
		}
	}
	return 1;
}

void erosion(int init_line, int end_line, Bitmap bitmap)
{
	unsigned char **flag_matrix;
	int size = end_line - init_line;
	flag_matrix = (unsigned char **)malloc(sizeof(unsigned char *) * size);
	//matriz bitmap é invertida
	int temp_init = invertPosMatrix(bitmap.bmp_header.height, end_line);
	int temp_end = invertPosMatrix(bitmap.bmp_header.height, init_line);
	for (int i = temp_init; i < temp_end; ++i)
	{
		flag_matrix[i-temp_init] = (unsigned char *)malloc(sizeof(unsigned char) * bitmap.bmp_header.width);
		for (int j = 0; j < bitmap.bmp_header.width; ++j)
		{
			if (i == temp_init || i == temp_end - 1 || j == 0 || j == bitmap.bmp_header.width - 1 || !checkErosion(i, j, bitmap.matrix))
			{
				flag_matrix[i - temp_init][j] = WHITE;
			}
			else
			{
				flag_matrix[i - temp_init][j] = BLACK;
			}
		}
	}

	for (int i = temp_init; i < temp_end; ++i)
	{
		for (int j = 0; j < bitmap.bmp_header.width; ++j)
		{
			bitmap.matrix[i][j].r = flag_matrix[i - temp_init][j];
			bitmap.matrix[i][j].g = flag_matrix[i - temp_init][j];
			bitmap.matrix[i][j].b = flag_matrix[i - temp_init][j];
		}
	}

	//free memory
	for (int i = 0; i < size; i++) 
	{
		free(flag_matrix[i]);
	}
	free(flag_matrix);
}

int flagSegment(int i, int j, int temp_init, Bitmap bitmap, unsigned char **flag_matrix, int val)
{
	//val controla o valor do j dentro do while
	int res = j;
	while (res < bitmap.bmp_header.width && 
		   res >= 0 &&
		   bitmap.matrix[i][res].r == BLACK)
	{
		flag_matrix[i-temp_init][res] = 1;
		bitmap.matrix[i][res].b = 255;
		res += val;
	}
	//retorna o maior ou menor j encontrado
	return res;
}

int max(int a, int b)
{
	if (a > b)
	{
		return a;
	}
	return b;
}

int min(int a, int b)
{
	if (a < b)
	{
		return a;
	}
	return b;
}

int getWoodNodes(int init_line, int end_line, Bitmap bitmap)
{
	unsigned char **flag_matrix;
	int size = end_line - init_line;
	
	//matriz bitmap é invertida
	int temp_init = invertPosMatrix(bitmap.bmp_header.height, end_line);
	int temp_end = invertPosMatrix(bitmap.bmp_header.height, init_line);

	int n_nodes = 0;

	flag_matrix = (unsigned char **)malloc(sizeof(unsigned char *) * size);
	for (int i = 0; i < size; ++i)
	{
		flag_matrix[i-temp_init] = (unsigned char *)malloc(sizeof(unsigned char) * bitmap.bmp_header.width);
		memset(flag_matrix[i], 0, bitmap.bmp_header.width);
	}
	
	for (int i = temp_init; i < temp_end; ++i)
	{
		for (int j = 0; j < bitmap.bmp_header.width; ++j)
		{
			if (!flag_matrix[i-temp_init][j])
			{
				flag_matrix[i-temp_init][j] = 1;
				if (bitmap.matrix[i][j].r == BLACK)
				{
					++n_nodes;
					int k = i;
					int l = j;
					int local_max_j, local_min_j;
					int global_max_j = j;
					int global_min_j = j;
					do 
					{
						local_min_j = flagSegment(k, l, temp_init, bitmap, flag_matrix, -1);
						local_max_j = flagSegment(k, l, temp_init, bitmap, flag_matrix, 1);
						printf("%d\n", local_min_j);
						++k;
						l = local_min_j;
						while (bitmap.matrix[k][l].r != BLACK && l <= local_max_j)
						{
							++l;
						}
						global_max_j = max(global_max_j, local_max_j);
						global_min_j = min(global_min_j, local_min_j);
					} while (k < temp_end && l <= local_max_j);
					printf("%d %d\n", global_max_j,global_min_j);
					// printf("%d %d\n", invertPosMatrix(bitmap.bmp_header.height, (k+i)/2), (global_max_j+global_min_j)/2);
				}
			}
		}
	}

	//free memory
	for (int i = 0; i < size; i++) 
	{
		free(flag_matrix[i]);
	}
	free(flag_matrix);

	return n_nodes;
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
	erosion(init_line, end_line, bitmap);
	erosion(init_line, end_line, bitmap);

	saveBitmap("nova_imagem2.bmp", bitmap.bmp_header, bitmap.matrix);

	int n_nodes = getWoodNodes(init_line, end_line, bitmap);

	saveBitmap("nova_imagem3.bmp", bitmap.bmp_header, bitmap.matrix);

	printf("%s\t%d %d %d\n", file_name, init_line, end_line, n_nodes);
	freeMatrix(bitmap.bmp_header, bitmap.matrix);
	return 0;
}
