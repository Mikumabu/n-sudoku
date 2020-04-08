/*
*	Implementado por Manuel Zuleta Bernal
*	Basado en la implementación original de Kevin Kuan 
*	https://github.com/kevinkuan/parallelSudoku
*/

#include "pch.h"
#include <stdlib.h>
#include <iostream>
#include <stdio.h>
#include <string.h>
#include <cstddef>
#include <cstdio>
#include <time.h>
#include <omp.h>
#include "CycleTimer.h"
#include <string.h>
#include <string>

using namespace std;

/* Global variable declarations */

/*
	The Matrix
*/
typedef struct matrix {
	short** data;
	short** fixed;
} MATRIX;

struct list_el {
	MATRIX mat;
	short i, j;
	struct list_el* next;
};

typedef struct list_el item;
int l;
int SIZEMATRIX;
FILE* inputMatrix;
MATRIX solution;
item* head;
item* tail;

/* Global variable declarations end here*/


/*
  Function: read_matrix_with_spaces(char *filename)
  Description: read the file and create the matrix puzzle
*/

MATRIX read_matrix_with_spaces(char* filename) {
	int i, j;
	MATRIX matrix;
	int element_int;

	inputMatrix = fopen(filename, "rt");

	fscanf(inputMatrix, "%d", &element_int);
	l = element_int;
	SIZEMATRIX = l * l;

	// allocate memory for matrix
	matrix.data = (short**)malloc(SIZEMATRIX * sizeof(short*));
	for (i = 0; i < SIZEMATRIX; i++)
		matrix.data[i] = (short*)malloc(SIZEMATRIX * sizeof(short));

	matrix.fixed = (short**)malloc(SIZEMATRIX * sizeof(short*));
	for (i = 0; i < SIZEMATRIX; i++)
		matrix.fixed[i] = (short*)malloc(SIZEMATRIX * sizeof(short));

	// init
	for (i = 0; i < SIZEMATRIX; i++) {
		for (j = 0; j < SIZEMATRIX; j++) {
			matrix.fixed[i][j] = 0;
		}
	}

	for (i = 0; i < SIZEMATRIX; i++) {
		for (j = 0; j < SIZEMATRIX; j++) {
			fscanf(inputMatrix, "%d", &element_int);
			matrix.data[i][j] = element_int;
			if (matrix.data[i][j] != 0)
				matrix.fixed[i][j] = 1;
		}
	}

	fclose(inputMatrix);

	return matrix;
}


/*
  Function: printMatrix(MATRIX *matrix)
  Description: prints a MATRIX to the standard output
*/
void printMatrix(MATRIX* matrix) {
	int i, j;
	printf("\n_________________________\n");
	for (i = 0; i < SIZEMATRIX; i++) {
		printf("| ");
		for (j = 0; j < SIZEMATRIX; j = j + 3) {
			printf("%1d %1d %1d | ", matrix->data[i][j], matrix->data[i][j + 1], matrix->data[i][j + 2]);
			//printf("%2d ", matrix->data[i][j]);
		}
		if ((i + 1) % 3 == 0) {
			printf("\n-------------------------\n");
		}
		else {
			printf("\n");
		}
	}
}

/*
  Solving the Sudoku
  Function: isValid(MATRIX matrix, short i_line, short j_co)
  Description: checks if the value at (i_line, j_col) in the matrix are valid according to Sudoku rules. Returns 1 if it is valid and 0 if it is not
*/
short isValid(MATRIX matrix, short i_line, short j_col) {

	short line, column;
	short value = matrix.data[i_line][j_col];

	// check same column
	for (line = 0; line < SIZEMATRIX; line++) {
		if (matrix.data[line][j_col] == 0)
			continue;

		if ((i_line != line) &&
			(matrix.data[line][j_col] == value))
			return 0;
	}

	// check same line
	for (column = 0; column < SIZEMATRIX; column++) {
		if (matrix.data[i_line][column] == 0)
			continue;

		if (j_col != column && matrix.data[i_line][column] == value)
			return 0;
	}

	// check group
	short igroup = (i_line / l) * l;
	short jgroup = (j_col / l) * l;
	for (line = igroup; line < igroup + l; line++) {
		for (column = jgroup; column < jgroup + l; column++) {
			if (matrix.data[line][column] == 0)
				continue;

			if ((i_line != line) &&
				(j_col != column) &&
				(matrix.data[line][column] == value)) {
				return 0;
			}
		}
	}

	return 1;
}

/*
  Backtracking process during solve (if it is a bad value)
  Function: decreasePosition(MATRIX* matrix, short* iPointer, short* jPointer)
  Description: moves the pointer in backward direction to a non-fixed value
*/
void decreasePosition(MATRIX* matrix, short* iPointer, short* jPointer) {
	do {
		if (*jPointer == 0 && *iPointer > 0) {
			*jPointer = SIZEMATRIX - 1;
			(*iPointer)--;
		}
		else
			(*jPointer)--;
	} while (*jPointer >= 0 && (*matrix).fixed[*iPointer][*jPointer] == 1);
}


/*
  Moving forward during solve (if it is OK)
  Function: increasePosition(MATRIX* matrix, short* iPointer, short* jPointer)
  Description: moves the pointer in forward direction to a non-fixed value
*/
void increasePosition(MATRIX* matrix, short* iPointer, short* jPointer) {

	do {
		if (*jPointer < SIZEMATRIX - 1)
			(*jPointer)++;
		else {
			*jPointer = 0;
			(*iPointer)++;
		}
	} while (*iPointer < SIZEMATRIX && (*matrix).fixed[*iPointer][*jPointer]);
}

/*
  Releasing memory
  Function: freeListElement(item *node)
  Description: deallocates memory for the item node
*/
void freeListElement(item* node) {
	int i;
	for (i = 0; i < SIZEMATRIX; i++) {
		free(node->mat.data[i]);
		free(node->mat.fixed[i]);
	}
	free(node->mat.data);
	free(node->mat.fixed);
	free(node);
}


/*
  Generating a current value to the matrix
  Function: createItem(MATRIX matrix, short i, short j)
  Description: creates an item for the matrix and returns it
 */
item* createItem(MATRIX matrix, short i, short j) {
	item* curr = (item*)malloc(sizeof(item));
	int m;
	short x, y;

	/* allocate memory for new copy */
	curr->mat.data = (short**)malloc(SIZEMATRIX * sizeof(short*));
	for (m = 0; m < SIZEMATRIX; m++)
		curr->mat.data[m] = (short*)malloc(SIZEMATRIX * sizeof(short));

	curr->mat.fixed = (short**)malloc(SIZEMATRIX * sizeof(short*));
	for (m = 0; m < SIZEMATRIX; m++)
		curr->mat.fixed[m] = (short*)malloc(SIZEMATRIX * sizeof(short));


	//copy matrix
	for (x = 0; x < SIZEMATRIX; x++) {
		for (y = 0; y < SIZEMATRIX; y++) {
			curr->mat.data[x][y] = matrix.data[x][y];
			curr->mat.fixed[x][y] = matrix.fixed[x][y];
		}
	}

	curr->i = i;
	curr->j = j;
	curr->next = NULL;

	return curr;
}


// LinkedList (Queue) functions for elements to be inserted/removed in matrix

/*
  Function: attachItem(MATRIX matrix, short i, short j)
  Description: adds an item to the tail of the linked list
 */
void attachItem(item* newItem) {

	if (head == NULL) {
		head = newItem;
		tail = newItem;
	}
	else {
		tail->next = newItem;
		tail = newItem;
	}
}

/*
  Function: removeItem()
  Description: removes an item from the head of the linked list and returns it
 */
item* removeItem() {
	item* result = NULL;
	if (head != NULL) {
		result = head;
		head = result->next;
		if (head == NULL) {
			tail = NULL;
		}
	}
	return result;
}

// End of LinkedList functions


/* Initialize permissible matrix pool */
void initializePool2(MATRIX* matrix) {

	short i = 0;
	short j = 0;

	if ((*matrix).fixed[i][j] == 1)
		increasePosition(matrix, &i, &j);

	short num;
	for (num = 0; num < SIZEMATRIX; num++) {
		((*matrix).data[i][j])++;
		if (isValid(*matrix, i, j) == 1) {
			item* newPath = createItem(*matrix, i, j);
			attachItem(newPath);
		}
	}

}

// Resolving process start

/* Improved Initialize permissible matrix pool */
void initializePool(MATRIX* matrix) {

	short i = 0;
	short j = 0;

	if ((*matrix).fixed[i][j] == 1)
		increasePosition(matrix, &i, &j);

	short num = 0;

	item* current = NULL;

	while (1) {
		((*matrix).data[i][j])++;

		//adding the matrix to the pool only if the value is permissible
		if (matrix->data[i][j] <= SIZEMATRIX && isValid(*matrix, i, j) == 1) {
			item* newPath = createItem(*matrix, i, j);
			attachItem(newPath);
			//printf("matrix %d added to pool i=%d, j=%d\n",num, i, j);
			//printMatrix(matrix);
			//printf("\n");
			num++;
		}
		else if (matrix->data[i][j] > SIZEMATRIX) {
			if (current != NULL) {
				freeListElement(current);
			}

			if (num >= SIZEMATRIX) {
				break;
			}

			item* current = removeItem();

			if (current == NULL) {
				break;
			}

			matrix = &(current->mat);
			i = current->i;
			j = current->j;

			if (i == SIZEMATRIX - 1 && j == SIZEMATRIX - 1) {
				//Is a solution
				attachItem(current);
				break;
			}

			num--;

			increasePosition(matrix, &i, &j);
		}
	}

	if (current != NULL) {
		freeListElement(current);
	}

	//printf("\nCreated %d initial boards.\n", num);

}


/* 
	OpenMP Parallel process to solve the Sudoku puzzle
*/
short bf_pool(MATRIX matrix) {

	head = NULL;
	tail = NULL;

	initializePool(&matrix);


	/* Begin of parallel block
		- result is a global variable
		- current, i and j are private variables
		- access to methods removeItem
		  and attachItem should be exclusive
	*/

	/* Remove matrix from repository, and
	   move to the next non-fixed position,
	   adding permissible matrices to the
	   repository for the next iteration
	 */

	short found = 0;
	short i, j;
	item* current;
	int level;

#pragma omp parallel shared(found) private(i,j, current, level)
	{

#pragma omp critical (pool)
		current = removeItem();

		while (current != NULL && found == 0) {

			MATRIX currMat = current->mat;

			i = current->i;
			j = current->j;

			increasePosition(&currMat, &i, &j);

			level = 1;

			while (level > 0 && i < SIZEMATRIX && found == 0) {
				if (currMat.data[i][j] < SIZEMATRIX) {
					// increase cell value, and check if
					// new value is permissible
					currMat.data[i][j]++;

					if (isValid(currMat, i, j) == 1) {
						increasePosition(&currMat, &i, &j);
						level++;
					}
				}
				else {
					// goes back to the previous non-fixed cell

					currMat.data[i][j] = 0;
					decreasePosition(&currMat, &i, &j);
					level--;
				} // end else

			} // end while

			if (i == SIZEMATRIX) {
				found = 1;
				solution = currMat;

				continue;
			}

			free(current);

#pragma omp critical (pool)
			current = removeItem();

		}

	}  /* End of parallel block */

	return found;
}
// End of solving process

/*
   Function: main
   Description: main function
*/
int main(int argc, char* argv[]) {

	double startTime;
	double endTime;

	// Entry values from console
	string optionAux = "";
	int option = 0;
	
	/*
		The following line is to choose the puzzle to solve, and can be:
		std::string str = "case/easy-case.txt";
		std::string str = "case/medium-case.txt";
		std::string str = "case/worst-case.txt";
		For now, is set the worst-case/hard difficulty
	*/
	std::string str = "case/worst-case.txt";
	char* cstr = new char[str.length() + 1];
	strcpy(cstr, str.c_str());

	// Insert number of cores to use
	do {
		cout << "Favor, ingrese la cantidad de nucleos a usar (Max.: " << omp_get_max_threads() << "):" << endl;
		cin >> optionAux;
		option = atoi(optionAux.c_str());
		if (option>omp_get_max_threads()) {
			cout << "Valor mayor a la cantidad de nucleos disponibles" << endl;
		}
	} while (option>omp_get_max_threads());

	// Start of the time to solve
	startTime = CycleTimer::currentSeconds();
	// new code
	int thread_count = option;

	printf("----------------------------------------------------------\n");
	printf("Max system threads = %d\n", omp_get_max_threads());
	printf("Running with %d threads\n", thread_count);
	printf("----------------------------------------------------------\n");


	//If we want to run on all threads
	if (thread_count <= -1)
	{
		//Static assignment to get consistent usage across trials
		int max_threads = omp_get_max_threads();
		int n_usage = (max_threads < 6) ? max_threads : 6;

		int* assignment;

		//static assignments
		int assignment12[6] = { 1, 2, 4, 6, 8, 12 };
		int assignment8[6] = { 1, 2, 3, 4, 6, 8 };
		int assignment40[6] = { 1, 2, 4, 8, 16, 20 };
		int assignment6[6] = { 1, 2, 3, 4, 5, 6 };

		//dynamic assignment
		if (max_threads == 12)
			assignment = assignment12;
		else if (max_threads == 8)
			assignment = assignment8;
		else if (max_threads == 40)
			assignment = assignment40;
		else if (max_threads == 6)
			assignment = assignment6;
		else {
			if (max_threads < 6)
			{
				int* temp = (int*)malloc(max_threads * sizeof(int));
				for (int i = 0; i < n_usage; i++)
					temp[i] = i + 1;
				assignment = temp;
			}
			else {
				int* temp = (int*)malloc(6 * sizeof(int));
				temp[0] = 1;
				temp[5] = max_threads;
				for (int i = 1; i < 5; i++)
					temp[i] = i * max_threads / 6;
				assignment = temp;
			}
		}

		//Loop through assignment values;
		for (int i = 0; i < n_usage; i++)
		{
			printf("----------------------------------------------------------\n");
			printf("Running with %d threads", assignment[i]);
			//Set thread count
			omp_set_num_threads(assignment[i]);

			//Run implementations

			MATRIX m = read_matrix_with_spaces(cstr);
			printf("Original board:");
			printMatrix(&m);

			startTime = CycleTimer::currentSeconds();

			//sending the input matrix to the bf_pool method which would return 1 if a solution is found
			short hasSolution = bf_pool(m);
			if (hasSolution == 0) {
				printf("No result!\n");
				return 1;
			}

		}

	}
	//Run the code with only one thread count and only report speedup
	else
	{
		//Set thread count
		omp_set_num_threads(thread_count);
		//Run implementations

		MATRIX m = read_matrix_with_spaces(cstr);
		printf("Original board:");
		printMatrix(&m);

		startTime = CycleTimer::currentSeconds();

		//sending the input matrix to the bf_pool method which would return 1 if a solution is found
		short hasSolution = bf_pool(m);
		if (hasSolution == 0) {
			printf("No result!\n");
			return 1;
		}
	}

	delete[] cstr;

	endTime = CycleTimer::currentSeconds();

	printf("Solution (Parallel):");
	//printing the solution to the standard output
	printMatrix(&solution);

	printf("Time: %.3f ms\n", endTime - startTime);

	//deallocating memory
	item* node = head;
	while (node != NULL) {
		item* next = node->next;
		freeListElement(node);
		node = next;
	}

	return 0;

}
