/* Program to check a proposed route through a grid, visualise it, and
 * repair it when blocks are placed on it.
 *
 * Skeleton program written for the COMP10002 assignment, with stages 0,
 * 1 and 2 added below.
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
#define READ_CELL 1 // read_cell() read a cell
#define READ_SEP 0 // read_cell() reached SEP0
#define READ_EOF (-1) // read_cell() reached the end of the input
#define REPAIR_NONE 0 // the route had no block to repair
#define REPAIR_DONE 1 // one broken segment of the route was repaired
#define REPAIR_FAILED 2 // a broken segment could not be repaired
#define NUM_MOVES 4 // moves are up, down, left, right (in that order)

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
	char *blocked; // rows*cols flags, nonzero where a cell is blocked
	int nblocks; // number of blocks read for the current configuration
} grid_t;

// row and column offsets of the moves: up, down, left, right
const int MOVE_ROW[NUM_MOVES] = {-1, 1, 0, 0};
const int MOVE_COL[NUM_MOVES] = {0, 0, -1, 1};

/* function prototypes -------------------------------------------------------*/
route_t* make_empty_route(void);
void free_state(state_t*);
void free_route(route_t* R);
route_t* insert_at_head(route_t*, cell_t*);
route_t* insert_at_tail(route_t*, cell_t*);

cell_t* make_cell(short row, short col);
int read_cell(cell_t *cell);
int more_input(void);
void read_header(grid_t *grid);
int read_blocks(grid_t *grid);
int read_route(route_t *route);
void free_grid(grid_t *grid);
int same_cell(cell_t *a, cell_t *b);
int in_grid(grid_t *grid, cell_t *cell);
int is_block(grid_t *grid, cell_t *cell);
int is_legal_move(cell_t *from, cell_t *to);
int route_status(grid_t *grid, route_t *route);
int repair_route(grid_t *grid, route_t *route);
int stage_one(grid_t *grid, route_t *route);
int stage_two_case(grid_t *grid, route_t *route);
void print_grid_info(grid_t *grid);
void print_route(route_t *route);
void print_route_status(int status);
void print_grid(grid_t *grid, route_t *route);

/* where it all happens ------------------------------------------------------*/
int
main(int argc, char *argv[]) {
	grid_t grid;
	route_t *route;
	int status, more, ok, first;

	/* stage 0: read the grid and the route, and check the route */
	read_header(&grid);
	read_blocks(&grid);
	route = make_empty_route();
	more = (read_route(route)==READ_SEP) && more_input();

	printf(SEP1, 0);
	print_grid_info(&grid);
	printf("The proposed route in the grid is:\n");
	print_route(route);
	status = route_status(&grid, route);
	print_route_status(status);

	/* stage 1: draw the route, and repair its first broken segment */
	printf(SEP1, 1);
	ok = stage_one(&grid, route);

	/* stage 2: repair the route fully for each new set of blocks */
	if (ok && more) {
		printf(SEP1, 2);
		first = 1;
		while (ok && more) {
			more = (read_blocks(&grid)==READ_SEP) && more_input();
			if (!first) {
				printf(SEP3);
			}
			first = 0;
			ok = stage_two_case(&grid, route);
		}
	}
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
// returns READ_CELL, or READ_SEP / READ_EOF if those come first
int
read_cell(cell_t *cell) {
	int c;
	while ((c=getchar())!=EOF && c!='[') {
		if (c==SEP0) {
			return READ_SEP;
		}
	}
	if (c==EOF || scanf("%hd,%hd]", &cell->row, &cell->col)!=2) {
		return READ_EOF;
	}
	cell->counter = 0;
	return READ_CELL;
}

// returns 1 if anything other than whitespace is left in the input
int
more_input(void) {
	int c;
	while ((c=getchar())==' ' || c=='\t' || c=='\r' || c=='\n') {
		/* skip whitespace */
	}
	if (c==EOF) {
		return 0;
	}
	ungetc(c, stdin);
	return 1;
}

// reads the optional "RxC" dimensions and the initial and goal cells
void
read_header(grid_t *grid) {
	if (scanf(" %dx%d", &grid->rows, &grid->cols)!=2) {
		grid->rows = DEFAULT_ROWS;
		grid->cols = DEFAULT_COLS;
	}
	if (read_cell(&grid->initial)!=READ_CELL
		|| read_cell(&grid->goal)!=READ_CELL) {
		fprintf(stderr, "malformed input\n");
		exit(EXIT_FAILURE);
	}
	grid->blocked = (char*)malloc(grid->rows*grid->cols);
	assert(grid->blocked!=NULL);
	grid->nblocks = 0;
}

// reads a set of blocks up to SEP0 or the end of the input, replacing
// any previous blocks; returns READ_SEP or READ_EOF
int
read_blocks(grid_t *grid) {
	cell_t cell;
	int i, result;
	for (i=0; i<grid->rows*grid->cols; i++) {
		grid->blocked[i] = 0;
	}
	grid->nblocks = 0;
	while ((result=read_cell(&cell))==READ_CELL) {
		if (in_grid(grid, &cell)) {
			grid->blocked[cell.row*grid->cols + cell.col] = 1;
		}
		grid->nblocks++;
	}
	return result;
}

// reads the route up to SEP0 or the end of the input (it may be split
// across lines); returns READ_SEP or READ_EOF
int
read_route(route_t *route) {
	cell_t cell;
	int result;
	while ((result=read_cell(&cell))==READ_CELL) {
		insert_at_tail(route, make_cell(cell.row, cell.col));
	}
	return result;
}

void
free_grid(grid_t *grid) {
	free(grid->blocked);
	grid->blocked = NULL;
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
	return in_grid(grid, cell)
		&& grid->blocked[cell->row*grid->cols + cell->col];
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

/* route repair --------------------------------------------------------------*/

// repairs the first broken segment of the route: a breadth-first search
// starts at the last cell before the first block, and stops at the first
// unblocked route cell after that block that it reaches; that cell is then
// joined to the start by tracing back through decreasing counter values
int
repair_route(grid_t *grid, route_t *route) {
	state_t *start = NULL, *s, *end = NULL, *next;
	cell_t *queue, cur, nbr;
	int *dist, *target, size = grid->rows*grid->cols;
	int head = 0, tail = 0, found = 0, i, m, idx;
	route_t *path;

	/* find the first block on the route, and the cell before it */
	for (s=route->head; s!=NULL && !is_block(grid, s->cell); s=s->next) {
		start = s;
	}
	if (s==NULL) {
		return REPAIR_NONE;
	}
	if (start==NULL) { /* the initial cell itself is blocked */
		return REPAIR_FAILED;
	}

	/* mark the unblocked route cells after the first block as targets */
	dist = (int*)malloc(size*sizeof(*dist));
	target = (int*)malloc(size*sizeof(*target));
	queue = (cell_t*)malloc(size*sizeof(*queue));
	assert(dist!=NULL && target!=NULL && queue!=NULL);
	for (i=0; i<size; i++) {
		dist[i] = -1;
		target[i] = 0;
	}
	for (; s!=NULL; s=s->next) {
		if (in_grid(grid, s->cell) && !is_block(grid, s->cell)) {
			target[s->cell->row*grid->cols + s->cell->col] = 1;
		}
	}

	/* breadth-first search from the start, counting steps in counter */
	cur = *start->cell;
	cur.counter = 0;
	dist[cur.row*grid->cols + cur.col] = 0;
	queue[tail++] = cur;
	while (head<tail && !found) {
		cur = queue[head++];
		for (m=0; m<NUM_MOVES && !found; m++) {
			nbr.row = cur.row + MOVE_ROW[m];
			nbr.col = cur.col + MOVE_COL[m];
			nbr.counter = cur.counter + 1;
			idx = nbr.row*grid->cols + nbr.col;
			if (!in_grid(grid, &nbr) || is_block(grid, &nbr)
				|| dist[idx]>=0) {
				continue;
			}
			dist[idx] = nbr.counter;
			queue[tail++] = nbr;
			if (target[idx]) {
				found = 1;
			}
		}
	}
	if (!found) {
		free(dist);
		free(target);
		free(queue);
		return REPAIR_FAILED;
	}

	/* trace back from the reached cell to the start */
	path = make_empty_route();
	cur = nbr;
	while (cur.counter>0) {
		insert_at_head(path, make_cell(cur.row, cur.col));
		for (m=0; m<NUM_MOVES; m++) {
			nbr.row = cur.row + MOVE_ROW[m];
			nbr.col = cur.col + MOVE_COL[m];
			if (in_grid(grid, &nbr)
				&& dist[nbr.row*grid->cols + nbr.col]==(int)cur.counter-1) {
				break;
			}
		}
		nbr.counter = cur.counter - 1;
		cur = nbr;
	}

	/* find the reached cell in the route after the first block (which is
	   the state right after start) */
	for (s=start->next->next; s!=NULL && end==NULL; s=s->next) {
		if (!is_block(grid, s->cell) && same_cell(s->cell, path->tail->cell)) {
			end = s;
		}
	}
	assert(end!=NULL);

	/* replace the states between start and end (inclusive of end) with
	   the new path, whose last cell is the same as end's */
	s = start->next;
	while (s!=end) {
		next = s->next;
		free_state(s);
		s = next;
	}
	start->next = path->head;
	path->tail->next = end->next;
	if (route->tail==end) {
		route->tail = path->tail;
	}
	free_state(end);
	free(path);

	free(dist);
	free(target);
	free(queue);
	return REPAIR_DONE;
}

// stage 1: draws the route and repairs its first broken segment;
// returns 0 if the route could not be repaired
int
stage_one(grid_t *grid, route_t *route) {
	int result;

	print_grid(grid, route);
	if (route_status(grid, route)!=ROUTE_INVALID_BLOCK) {
		return route_status(grid, route)==ROUTE_VALID;
	}
	printf(SEP2);
	result = repair_route(grid, route);
	print_grid(grid, route);
	printf(SEP2);
	if (result==REPAIR_FAILED) {
		printf("The route cannot be repaired!\n");
		return 0;
	}
	print_route(route);
	print_route_status(route_status(grid, route));
	return 1;
}

// stage 2: draws the route with the current blocks and repairs every
// broken segment of it; returns 0 if the route could not be repaired
int
stage_two_case(grid_t *grid, route_t *route) {
	int result;

	print_grid(grid, route);
	if (route_status(grid, route)!=ROUTE_INVALID_BLOCK) {
		return 1;
	}
	printf(SEP2);
	while ((result=repair_route(grid, route))==REPAIR_DONE) {
		/* keep repairing until no block is left on the route */
	}
	print_grid(grid, route);
	printf(SEP2);
	if (result==REPAIR_FAILED) {
		printf("The route cannot be repaired!\n");
		return 0;
	}
	print_route(route);
	print_route_status(route_status(grid, route));
	return 1;
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

// draws the grid; blocks are drawn on top of the route
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
	for (s=route->head; s!=NULL; s=s->next) {
		if (in_grid(grid, s->cell)) {
			codes[s->cell->row*grid->cols + s->cell->col] = CELL_CODE_VISITED;
		}
	}
	for (i=0; i<size; i++) {
		if (grid->blocked[i]) {
			codes[i] = CELL_CODE_BLOCK;
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
