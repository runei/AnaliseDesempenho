#include <stdio.h>
#include <stdlib.h>
#include <stdint.h> /* define inteiros de tamanho específico */
#include <string.h>
#include <dirent.h>
#include <float.h>

#define K_WOOD 107
#define LEFT -1
#define RIGHT 1
#define UP 1
#define DOWN -1
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
} Point;

typedef struct {
	int size;
	Point *nodes;
} Nodes;

typedef struct {
	int stack_pointer;
	Point *stack;
} Stack;

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

unsigned int setColor(int red, int green, int blue)
{
	return (255 << 24) | (red << 16) | (green << 8) | blue;
}

void push(Stack *stack, Point val)
{
	if (stack == NULL)
	{
		perror("Stack = NULL");
		return;
	}
	++stack->stack_pointer;
	Point *p = (Point *)realloc(stack->stack, (stack->stack_pointer+1) * sizeof(Point));
	if (p != NULL)
	{
		stack->stack = p;
	}
	stack->stack[stack->stack_pointer] = val;
}

Point pop(Stack *stack)
{
	Point p = stack->stack[stack->stack_pointer];
	--stack->stack_pointer;
	return p;
}

unsigned int **sobelFiltering(Bitmap *bmp)
     /* Spatial filtering of image data */
     /* Sobel filter (horizontal differentiation */
     /* Input: image1[y][x] ---- Outout: image2[y][x] */
{
	unsigned int **image2 = (unsigned int **) malloc(sizeof(unsigned int *) * bmp->bmp_header.height);
	for (int i = 0; i < bmp->bmp_header.height; i++) {
		image2[i] = (unsigned int *) malloc(sizeof(unsigned int) * bmp->bmp_header.width);
	}
  /* Definition of Sobel filter in horizontal direction */
	int weight[3][3] = {{ -1,  0,  1 },
						{ -2,  0,  2 },
						{ -1,  0,  1 }};
	double pixel_value;
	double min, max;
	int x, y, i, j;  /* Loop variable */

  /* Maximum values calculation after filtering*/
	printf("Now, filtering of input image is performed\n\n");
	min = DBL_MAX;
	max = -DBL_MAX;
	for (y = 1; y < bmp->bmp_header.height - 1; y++) {
		for (x = 1; x < bmp->bmp_header.width - 1; x++) {
			pixel_value = 0.0;
			for (j = -1; j <= 1; j++) {
				for (i = -1; i <= 1; i++) {
					pixel_value += weight[j + 1][i + 1] * bmp->matrix[y + j][x + i];
				}
			}
			if (pixel_value < min) min = pixel_value;
			if (pixel_value > max) max = pixel_value;
		}
	}
	if ((int)(max - min) == 0) {
		printf("Nothing exists!!!\n\n");
		exit(1);
	}

  /* Initialization of image2[y][x] */
	int x_size2 = bmp->bmp_header.width;
	int y_size2 = bmp->bmp_header.height;
	for (y = 0; y < y_size2; y++) {
		for (x = 0; x < x_size2; x++) {
			image2[y][x] = 0;
		}
	}
  /* Generation of image2 after linear transformtion */
	for (y = 1; y < bmp->bmp_header.height - 1; y++) {
		for (x = 1; x < bmp->bmp_header.width - 1; x++) {
			pixel_value = 0.0;
			for (j = -1; j <= 1; j++) {
				for (i = -1; i <= 1; i++) {
					pixel_value += weight[j + 1][i + 1] * bmp->matrix[y + j][x + i];
				}
			}
			pixel_value = WHITE * (pixel_value - min) / (max - min);
			image2[y][x] = (unsigned int)pixel_value;
		}
	}
	return image2;
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
			// int media = (getRed(matrix[i][j]) + getGreen(matrix[i][j]) + getBlue(matrix[i][j])) / 3;
			// matrix[i][j] = setColor(media, media, media);
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

int fill(int temp_init, Point p, Bitmap *bmp, char **flag_matrix, char side)
{
	int result = p.col;
	while ( p.col < bmp->bmp_header.width &&
			p.col >= 0 &&
			bmp->matrix[p.row][result] == BLACK)
	{
		flag_matrix[p.row-temp_init][result] = 1;
		result += side;
	}
	return result;
}

void findStartPoints(int xleft, int xright, int col, unsigned int *row, Stack *stack, char *row_flag_matrix)
{
	char new_point = 1;
	int i = xleft;
	while (i <= xright)
	{
		if (new_point && !row_flag_matrix[i] && row[i] == BLACK)
		{
			Point p = {col, i};
			push(stack, p);
			new_point = 0;
		}
		else if (!new_point && row[i] == WHITE)
		{
			new_point = 1;
		}
		++i;
	}
}

Nodes getWoodNodes(int init_line, int end_line, Bitmap bitmap)
{
	char **flag_matrix;
	int size = end_line - init_line;
	
	//matriz bitmap é invertida
	int temp_init = invertPosMatrix(bitmap.bmp_header.height, end_line);
	int temp_end = invertPosMatrix(bitmap.bmp_header.height, init_line);

	Nodes nodes = {0, NULL};

	flag_matrix = (char **)malloc(sizeof(char *) * size);
	for (int i = 0; i < size; ++i)
	{
		flag_matrix[i] = (char *)malloc(sizeof(char) * bitmap.bmp_header.width);
		// memset(flag_matrix[i], 0, bitmap.bmp_header.width);
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
					nodes.nodes = (Point *)realloc(nodes.nodes, nodes.size * sizeof(Point));
					
					int max_col, min_col, max_row, min_row;
					max_col = min_col = j;
					max_row = min_row = i;
					Stack stack = {0, NULL};

					Point p = {i, j};
					push(&stack, p);
					while (stack.stack_pointer)
					{
						p = pop(&stack);

						max_col = max(max_col, p.col);
						min_col = min(min_col, p.col);
						max_row = max(max_row, p.row);
						min_row = min(min_row, p.row);

						int xleft = fill(temp_init, p, &bitmap, flag_matrix, LEFT);
						int xright = fill(temp_init, p, &bitmap, flag_matrix, RIGHT);
						findStartPoints(xleft, xright, p.row-1, bitmap.matrix[p.row-1], &stack, flag_matrix[p.row-temp_init-1]);
						findStartPoints(xleft, xright, p.row+1, bitmap.matrix[p.row+1], &stack, flag_matrix[p.row-temp_init+1]);
						// printf("%d\n", stack.stack_pointer);
					}

					nodes.nodes[nodes.size - 1].col = (max_col + min_col) / 2;
					nodes.nodes[nodes.size - 1].row = invertPosMatrix(bitmap.bmp_header.height, (max_row + min_row) / 2);
					free(stack.stack);
/*
					char name[50];
					snprintf(name,50,"testes/nova_imagem%d_%d.bmp", i, j);
					saveBitmap(name, bitmap.bmp_header, bitmap.matrix);*/
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
		// if (strcmp(ent->d_name, "st1285.bmp") == 0)
		if (ext != NULL && strcmp(ext, ".bmp") == 0)
		{
			Bitmap bitmap;
			char file_name[20];
			snprintf(file_name, sizeof(file_name), "%s%s", dir_name, ent->d_name);
			bitmap.matrix = loadBitmap(file_name, &bitmap.bmp_header);

			// saveBitmap("nova_imagem.bmp", bitmap.bmp_header, bitmap.matrix);

			// bitmap.matrix = sobelFiltering(&bitmap);

			// saveBitmap("nova_imagem4.bmp", bitmap.bmp_header, bitmap.matrix);			

			int init_line = getLinhaInicial(bitmap.bmp_header, bitmap.matrix);
			int end_line = getLinhaFinal(bitmap.bmp_header, bitmap.matrix);
			for (int i = 0; i < 3; ++i)
			{
				erosion(init_line, end_line, bitmap);
			}

			// saveBitmap("nova_imagem2.bmp", bitmap.bmp_header, bitmap.matrix);

			Nodes nodes = getWoodNodes(init_line, end_line, bitmap);

			// saveBitmap("nova_imagem3.bmp", bitmap.bmp_header, bitmap.matrix);

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
