#include <stdio.h>
#include <stdlib.h>
#include <stdint.h> /* define inteiros de tamanho específico */
#include <string.h>
#include <dirent.h>

#define K_WOOD 107
#define WHITE 0xFFFFFF
#define BLACK 0x000000
#define BLUE 0x0000FF

#pragma pack(push, 1)
typedef struct {
	uint16_t type;
	uint32_t size;
	uint16_t reserved1;
	uint16_t reserved2;
	uint32_t offset;
	uint32_t header_size;
	int32_t width;
	int32_t height;    
	uint16_t planes; 
	uint16_t bits;
	uint32_t compression;
	uint32_t imagesize;
	int32_t xresolution;
	int32_t yresolution;
	uint32_t ncolours;
	uint32_t importantcolours;
} BitmapHeader;
#pragma pack(pop) /* restaura comportamento do compilador */

typedef struct {
	BitmapHeader bmp_header;
	unsigned int **matrix;
} Bitmap;

typedef struct {
	int row;
	int col;
} Node;

typedef struct {
	int size;
	Node *nodes;
} Nodes;

uint8_t getBlue(unsigned int color)
{
	return 0xff & color;
}

uint8_t getGreen(unsigned int color)
{
	return (color >> 8) & 0xff;
}

uint8_t getRed(unsigned int color)
{
	return (color >> 16) & 0xff;
}

unsigned int **loadBitmap(char *file_name, BitmapHeader *bH)
{
	FILE *imagem;
	
	imagem = fopen(file_name, "rb");
	
	if (imagem == NULL)
	{
		printf("Erro ao abrir a imagem %s\n", file_name);
		perror("");
		exit(0);
	}

	if (!fread(bH, sizeof(BitmapHeader), 1, imagem))
	{
		perror("Erro ao ler bitmap header");	
		exit(0);
	}
	
	unsigned int **matrix = (unsigned int **) malloc(sizeof(unsigned int *) * bH->height);
	
	int i, j;
	
	for (i = 0; i < bH->height; i++) {
		matrix[i] = (unsigned int *) malloc(sizeof(unsigned int) * bH->width);
	}
	/* leitura */
	for (i = 0; i < bH->height; i++) {
		for (j = 0; j < bH->width; j++) {
			fread(&matrix[i][j], bH->bits / 8, 1, imagem); // 1byte = 8bits
			double media = (getRed(matrix[i][j]) + getGreen(matrix[i][j]) + getBlue(matrix[i][j])) / 3;
			int val = BLACK;
			if (media > K_WOOD)
			{
				val = WHITE;
			}
			matrix[i][j] = val;
		}
	}

	fclose(imagem);

	return matrix;
}

void saveBitmap(char *fileName, BitmapHeader bH, unsigned int **matrix)
{
	FILE *nova = fopen(fileName, "wb");

	fwrite(&bH, sizeof(BitmapHeader), 1, nova);

	/* escrita */
	for (int i = 0; i < bH.height; i++) {
		for (int j = 0; j < bH.width; j++) {
			fwrite(&matrix[i][j], bH.bits / 8, 1, nova); // 1byte = 8bits
		}
	}

	fclose(nova);
}

void freeMatrix(BitmapHeader bH, unsigned int **matrix)
{
	for (int i = 0; i < bH.height; i++) 
	{
		free(matrix[i]);
	}
	free(matrix);
}

void printBitmap(BitmapHeader bH, unsigned int **matrix)
{
	for (int i = 0; i < bH.height; i++) {
		for (int j = 0; j < bH.width; j++) {
			printf("%d ", matrix[i][j]);
		}
		printf("\n");
	}	
}

int invertPosMatrix(int height, int val)
{
	return height - val - 1;
}

int getLinhaInicial(BitmapHeader bH, unsigned int **matrix)
{
	//nao considera imagens vazias
	int i = bH.height - 1;
	while (matrix[i][10] == BLACK)
	{
		--i;
	}
	return invertPosMatrix(bH.height, i);
}

int getLinhaFinal(BitmapHeader bH, unsigned int **matrix)
{
	//nao considera imagens vazias
	int i = 0;
	while (matrix[i][10] == BLACK)
	{
		++i;
	}
	return invertPosMatrix(bH.height, i);
}

int checkErosion(int i, int j, unsigned int **matrix)
{
	for (int k = -1; k < 2; ++k)
	{
		for (int l = -1; l < 2; ++l)
		{
			if (matrix[i-k][j-l] == WHITE)
			{
				return 0;
			}
		}
	}
	return 1;
}

void erosion(int init_line, int end_line, Bitmap bitmap)
{
	unsigned int **flag_matrix;
	int size = end_line - init_line;
	flag_matrix = (unsigned int **)malloc(sizeof(unsigned int *) * size);
	//matriz bitmap é invertida
	int temp_init = invertPosMatrix(bitmap.bmp_header.height, end_line);
	int temp_end = invertPosMatrix(bitmap.bmp_header.height, init_line);
	for (int i = temp_init; i < temp_end; ++i)
	{
		flag_matrix[i-temp_init] = (unsigned int *)malloc(sizeof(unsigned int) * bitmap.bmp_header.width);
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
			bitmap.matrix[i][j] = flag_matrix[i - temp_init][j];
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
		   bitmap.matrix[i][res] == BLACK)
	{
		flag_matrix[i-temp_init][res] = 1;
		bitmap.matrix[i][res] = BLUE;
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

/*void floodFill(int i, int j, Bitmap bitmap, unsigned char **flag_matrix, NodePoints *np)
{
	if (bitmap.matrix[i][j] == BLACK && bitmap.matrix[i][j] != BLUE)
	{
		bitmap.matrix[i][j] = BLUE;
		floodFill(i+1, j, bitmap, flag_matrix);	
		floodFill(i-1, j, bitmap, flag_matrix);	
		floodFill(i, j+1, bitmap, flag_matrix);	
		floodFill(i, j-1, bitmap, flag_matrix);	
	}
}*/

Nodes getWoodNodes(int init_line, int end_line, Bitmap bitmap)
{
	unsigned char **flag_matrix;
	int size = end_line - init_line;
	
	//matriz bitmap é invertida
	int temp_init = invertPosMatrix(bitmap.bmp_header.height, end_line);
	int temp_end = invertPosMatrix(bitmap.bmp_header.height, init_line);

	Nodes nodes = {0, NULL};

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
				if (bitmap.matrix[i][j] == BLACK)
				{
					++nodes.size;
					nodes.nodes = (Node *)realloc(nodes.nodes, nodes.size * sizeof(Node));

					int k = i;
					int l = j;
					int local_max_j, local_min_j;
					int global_max_j = j;
					int global_min_j = j;
					do 
					{
						local_min_j = flagSegment(k, l, temp_init, bitmap, flag_matrix, -1);
						local_max_j = flagSegment(k, l, temp_init, bitmap, flag_matrix, 1);
						++k;
						l = local_min_j;
						while (bitmap.matrix[k][l] != BLACK && l <= local_max_j)
						{
							++l;
						}
						global_max_j = max(global_max_j, local_max_j);
						global_min_j = min(global_min_j, local_min_j);
					} while (k < temp_end && l <= local_max_j);
					nodes.nodes[nodes.size - 1].col = (global_max_j + global_min_j) / 2;
					nodes.nodes[nodes.size - 1].row = invertPosMatrix(bitmap.bmp_header.height, (k+i)/2);
					char name[50];
					snprintf(name,50,"testes/nova_imagem%d_%d.bmp", i, j);
					saveBitmap(name, bitmap.bmp_header, bitmap.matrix);
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

	return nodes;
}

int main() 
{
	char *dir_name = "validos/";
	DIR *dir = opendir(dir_name);
	if (dir == NULL)
	{
		perror("Diretório 'validos' não encontrado");
		exit(0);
	}
	
	struct dirent *ent;
	while ((ent = readdir(dir)) != NULL)
	{
		char *ext = strrchr(ent->d_name, '.');
		if (strcmp(ent->d_name, "st1183.bmp") == 0)
		// if (ext != NULL && strcmp(ext, ".bmp") == 0)
		{
			Bitmap bitmap;
			char file_name[20];
			snprintf(file_name, sizeof(file_name), "%s%s", dir_name, ent->d_name);
			bitmap.matrix = loadBitmap(file_name, &bitmap.bmp_header);

			saveBitmap("nova_imagem.bmp", bitmap.bmp_header, bitmap.matrix);

			int init_line = getLinhaInicial(bitmap.bmp_header, bitmap.matrix);
			int end_line = getLinhaFinal(bitmap.bmp_header, bitmap.matrix);
			for (int i = 0; i < 2; ++i)
			{
				erosion(init_line, end_line, bitmap);
			}

			saveBitmap("nova_imagem2.bmp", bitmap.bmp_header, bitmap.matrix);

			Nodes nodes = getWoodNodes(init_line, end_line, bitmap);

			saveBitmap("nova_imagem3.bmp", bitmap.bmp_header, bitmap.matrix);

			printf("%s %d %d %d", ent->d_name, init_line, end_line, nodes.size);
			for (int i = 0; i < nodes.size; ++i)
			{
				printf(" %d %d", nodes.nodes[i].row, nodes.nodes[i].col);
			}
			printf("\n");
			free(nodes.nodes);
			freeMatrix(bitmap.bmp_header, bitmap.matrix);
		}
	}

	closedir(dir);
	
	return 0;
}
