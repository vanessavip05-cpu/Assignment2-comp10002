/* Program to check a proposed route through a grid and visualise it.
 *
 * Skeleton program written for the COMP10002 assignment, with the
 * route checking and grid printing code added below.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

// various separators used in input/output
#define SEP0 '$'
#define SEP1 "==STAGE %d=======================================\n"
#define SEP2 "------------------------------------------------\n"
#define SEP3 "================================================\n"

// ASCII codes used in grid visualizations
#define CELL_CODE_INITIAL 73 // 'I' - initial cell
#define CELL_CODE_GOAL 71 // 'G' - goal cell
#define CELL_CODE_BLOCK 35 // '#' - block cell
#define CELL_CODE_EMPTY 32 // ' ' - empty cell
#define CELL_CODE_VISITED 42 // '*' - visited cell

// route statuses
#define ROUTE_STATUS_UNKNOWN 0 // Unknown route status
#define ROUTE_INVALID_INITIAL 1 // Initial cell in the route is wrong!
#define ROUTE_INVALID_GOAL 2 // Goal cell in the route is wrong!
#define ROUTE_INVALID_MOVE 3 // There is an illegal move in this route!
#define ROUTE_INVALID_BLOCK 4 // There is a block on this route!
#define ROUTE_VALID 5 // The route is valid!

#define MAX_CELLS_PER_LINE 5 // max number of cells to print per line

// additional constants
#define DEFAULT_ROWS 10 // grid rows when the input gives no dimensions
#define DEFAULT_COLS 10 // grid columns when the input gives no dimensions
#define INIT_BLOCKS 4 // initial size of the dynamic array of blocks

/* type definitions ----------------------------------------------------------*/

// grid cell
typedef struct {
	short row; // row of the cell
	short col; // column of the cell
	unsigned int counter; // counter value associated with this cell
} cell_t;

// state in a route
typedef struct state state_t;
struct state {
	cell_t *cell; // pointer to a cell
	state_t *next; // pointer to the next state in the route
};

// route (implemented as a linked list)
typedef struct {
	state_t *head; // pointer to the node in the head of the linked list
	state_t *tail; // pointer to the node in the tail of the linked list
} route_t;

// grid with its initial cell, goal cell and blocks
typedef struct {
	int rows; // number of rows in the grid
	int cols; // number of columns in the grid
	cell_t initial; // initial cell
	cell_t goal; // goal cell
	cell_t *blocks; // dynamic array of blocked cells
	int nblocks; // number of blocked cells
} grid_t;

/* function prototypes -------------------------------------------------------*/
route_t* make_empty_route(void);
void free_state(state_t*);
void free_route(route_t* R);
route_t* insert_at_head(route_t*, cell_t*);
route_t* insert_at_tail(route_t*, cell_t*);

cell_t* make_cell(short row, short col);
int read_cell(cell_t *cell);
void read_grid(grid_t *grid);
route_t* read_route(void);
void free_grid(grid_t *grid);
int same_cell(cell_t *a, cell_t *b);
int in_grid(grid_t *grid, cell_t *cell);
int is_block(grid_t *grid, cell_t *cell);
int is_legal_move(cell_t *from, cell_t *to);
int route_status(grid_t *grid, route_t *route);
void print_grid_info(grid_t *grid);
void print_route(route_t *route);
void print_route_status(int status);
void print_grid(grid_t *grid, route_t *route);

/* where it all happens ------------------------------------------------------*/
int
main(int argc, char *argv[]) {
	grid_t grid;
	route_t *route;

	/* stage 0: read the input and check the route */
	read_grid(&grid);
	route = read_route();
	print_grid_info(&grid);
	print_route(route);
	print_route_status(route_status(&grid, route));

	/* stage 1: visualise the route on the grid */
	printf(SEP1, 1);
	print_grid(&grid, route);
	printf(SEP3);

	free_route(route);
	free_grid(&grid);
	return EXIT_SUCCESS; // we are done! algorithms are fun!!!
}

void
free_state(state_t* state) {
	if (state) {
		free(state->cell);
		free(state);
	}
}

// Adapted version of the make_empty_list function by Alistair Moffat:
// https://people.eng.unimelb.edu.au/ammoffat/ppsaa/c/listops.c
// Data type and variable names changed
route_t
*make_empty_route(void) {
	route_t *R;
	R = (route_t*)malloc(sizeof(*R));
	assert(R!=NULL);
	R->head = R->tail = NULL;
	return R;
}

// Adapted version of the free_list function by Alistair Moffat:
// https://people.eng.unimelb.edu.au/ammoffat/ppsaa/c/listops.c
// Data type and variable names changed
void
free_route(route_t* R) {
	state_t *curr, *prev;
	assert(R!=NULL);
	curr = R->head;
	while (curr) {
		prev = curr;
		curr = curr->next;
		free_state(prev);
	}
	free(R);
}

// Adapted version of the insert_at_head function by Alistair Moffat:
// https://people.eng.unimelb.edu.au/ammoffat/ppsaa/c/listops.c
// Data type and variable names changed
route_t
*insert_at_head(route_t* R, cell_t* addr) {
	assert(R!=NULL);
	state_t *new;
	new = (state_t*)malloc(sizeof(*new));
	assert(new!=NULL);
	new->cell = addr;
	new->next = R->head;
	R->head = new;
	if (R->tail==NULL) { /* this is the first insertion into the route */
		R->tail = new;
	}
	return R;
}

// Adapted version of the insert_at_foot function by Alistair Moffat:
// https://people.eng.unimelb.edu.au/ammoffat/ppsaa/c/listops.c
// Data type and variable names changed
route_t
*insert_at_tail(route_t* R, cell_t* addr) {
	assert(R!=NULL);
	state_t *new;
	new = (state_t*)malloc(sizeof(*new));
	assert(new!=NULL);
	new->cell = addr;
	new->next = NULL;
	if (R->tail==NULL) { /* this is the first insertion into the route */
		R->head = R->tail = new;
	} else {
		R->tail->next = new;
		R->tail = new;
	}
	return R;
}

/* input ---------------------------------------------------------------------*/

// allocates a new cell with the given coordinates
cell_t
*make_cell(short row, short col) {
	cell_t *cell;
	cell = (cell_t*)malloc(sizeof(*cell));
	assert(cell!=NULL);
	cell->row = row;
	cell->col = col;
	cell->counter = 0;
	return cell;
}

// reads the next "[r,c]" from stdin, skipping anything before it;
// returns 0 when SEP0 or the end of input is reached instead
int
read_cell(cell_t *cell) {
	int c;
	while ((c=getchar())!=EOF && c!='[') {
		if (c==SEP0) {
			return 0;
		}
	}
	if (c==EOF || scanf("%hd,%hd]", &cell->row, &cell->col)!=2) {
		return 0;
	}
	cell->counter = 0;
	return 1;
}

// reads the optional "RxC" dimensions, the initial and goal cells, and
// the blocks up to SEP0
void
read_grid(grid_t *grid) {
	int capacity = INIT_BLOCKS;
	cell_t cell;

	if (scanf(" %dx%d", &grid->rows, &grid->cols)!=2) {
		grid->rows = DEFAULT_ROWS;
		grid->cols = DEFAULT_COLS;
	}
	if (!read_cell(&grid->initial) || !read_cell(&grid->goal)) {
		fprintf(stderr, "malformed input\n");
		exit(EXIT_FAILURE);
	}

	grid->nblocks = 0;
	grid->blocks = (cell_t*)malloc(capacity*sizeof(*grid->blocks));
	assert(grid->blocks!=NULL);
	while (read_cell(&cell)) {
		if (grid->nblocks==capacity) {
			capacity *= 2;
			grid->blocks = (cell_t*)realloc(grid->blocks,
				capacity*sizeof(*grid->blocks));
			assert(grid->blocks!=NULL);
		}
		grid->blocks[grid->nblocks++] = cell;
	}
}

// reads the route that follows SEP0; it may be split across lines
route_t
*read_route(void) {
	route_t *route = make_empty_route();
	cell_t cell;
	while (read_cell(&cell)) {
		insert_at_tail(route, make_cell(cell.row, cell.col));
	}
	return route;
}

void
free_grid(grid_t *grid) {
	free(grid->blocks);
	grid->blocks = NULL;
	grid->nblocks = 0;
}

/* route checks --------------------------------------------------------------*/

int
same_cell(cell_t *a, cell_t *b) {
	return a->row==b->row && a->col==b->col;
}

int
in_grid(grid_t *grid, cell_t *cell) {
	return cell->row>=0 && cell->row<grid->rows
		&& cell->col>=0 && cell->col<grid->cols;
}

int
is_block(grid_t *grid, cell_t *cell) {
	int i;
	for (i=0; i<grid->nblocks; i++) {
		if (same_cell(&grid->blocks[i], cell)) {
			return 1;
		}
	}
	return 0;
}

// a legal move goes up, down, left or right by exactly one cell
int
is_legal_move(cell_t *from, cell_t *to) {
	return abs(from->row - to->row) + abs(from->col - to->col)==1;
}

int
route_status(grid_t *grid, route_t *route) {
	state_t *s;

	if (route->head==NULL || !same_cell(route->head->cell, &grid->initial)) {
		return ROUTE_INVALID_INITIAL;
	}
	if (!same_cell(route->tail->cell, &grid->goal)) {
		return ROUTE_INVALID_GOAL;
	}
	for (s=route->head; s!=NULL; s=s->next) {
		if (!in_grid(grid, s->cell)
			|| (s->next!=NULL && !is_legal_move(s->cell, s->next->cell))) {
			return ROUTE_INVALID_MOVE;
		}
	}
	for (s=route->head; s!=NULL; s=s->next) {
		if (is_block(grid, s->cell)) {
			return ROUTE_INVALID_BLOCK;
		}
	}
	return ROUTE_VALID;
}

/* output --------------------------------------------------------------------*/

void
print_grid_info(grid_t *grid) {
	printf("The grid has %d rows and %d columns.\n", grid->rows, grid->cols);
	printf("The grid has %d block(s).\n", grid->nblocks);
	printf("The initial cell in the grid is [%d,%d].\n",
		grid->initial.row, grid->initial.col);
	printf("The goal cell in the grid is [%d,%d].\n",
		grid->goal.row, grid->goal.col);
}

void
print_route(route_t *route) {
	state_t *s;
	int count = 0;

	printf("The proposed route in the grid is:\n");
	for (s=route->head; s!=NULL; s=s->next) {
		printf("[%d,%d]", s->cell->row, s->cell->col);
		count++;
		if (s->next==NULL) {
			printf(".\n");
		} else {
			printf("->");
			if (count%MAX_CELLS_PER_LINE==0) {
				printf("\n");
			}
		}
	}
}

void
print_route_status(int status) {
	switch (status) {
	case ROUTE_INVALID_INITIAL:
		printf("Initial cell in the route is wrong!\n");
		break;
	case ROUTE_INVALID_GOAL:
		printf("Goal cell in the route is wrong!\n");
		break;
	case ROUTE_INVALID_MOVE:
		printf("There is an illegal move in this route!\n");
		break;
	case ROUTE_INVALID_BLOCK:
		printf("There is a block on this route!\n");
		break;
	case ROUTE_VALID:
		printf("The route is valid!\n");
		break;
	default:
		break;
	}
}

void
print_grid(grid_t *grid, route_t *route) {
	char *codes;
	state_t *s;
	int r, c, i, size = grid->rows*grid->cols;

	codes = (char*)malloc(size);
	assert(codes!=NULL);
	for (i=0; i<size; i++) {
		codes[i] = CELL_CODE_EMPTY;
	}
	for (i=0; i<grid->nblocks; i++) {
		if (in_grid(grid, &grid->blocks[i])) {
			codes[grid->blocks[i].row*grid->cols + grid->blocks[i].col] =
				CELL_CODE_BLOCK;
		}
	}
	for (s=route->head; s!=NULL; s=s->next) {
		if (in_grid(grid, s->cell)) {
			codes[s->cell->row*grid->cols + s->cell->col] = CELL_CODE_VISITED;
		}
	}
	if (in_grid(grid, &grid->initial)) {
		codes[grid->initial.row*grid->cols + grid->initial.col] =
			CELL_CODE_INITIAL;
	}
	if (in_grid(grid, &grid->goal)) {
		codes[grid->goal.row*grid->cols + grid->goal.col] = CELL_CODE_GOAL;
	}

	printf(" ");
	for (c=0; c<grid->cols; c++) {
		printf("%d", c%10);
	}
	printf("\n");
	for (r=0; r<grid->rows; r++) {
		printf("%d", r%10);
		for (c=0; c<grid->cols; c++) {
			putchar(codes[r*grid->cols + c]);
		}
		printf("\n");
	}
	free(codes);
}
