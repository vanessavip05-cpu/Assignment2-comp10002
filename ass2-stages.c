/* Program to check, draw and repair a route of an agent in a grid with blocks.
 *
 * Stage 0: read the grid and the route, and report the status of the route.
 * Stage 1: draw the grid and route; if the route visits a block, repair its
 *          first broken segment and draw the repaired route.
 * Stage 2: for each further block configuration, repair all broken segments
 *          of the most recent route.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

/* DEFINITIONS ---------------------------------------------------------------*/
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

// additional definitions
#define DEFAULT_ROWS 10 // grid rows if the input has no "RxC" line
#define DEFAULT_COLS 10 // grid columns if the input has no "RxC" line
#define READ_CELL 1 // read_cell() read a cell
#define READ_SEP 0 // read_cell() reached a '$' line
#define READ_EOF (-1) // read_cell() reached the end of the input
#define REPAIR_NONE 0 // the route has no broken segment
#define REPAIR_DONE 1 // one broken segment was repaired
#define REPAIR_FAILED 2 // the first broken segment cannot be repaired
#define NUM_MOVES 4 // moves are up, down, left, right (in this order)
#define NOT_QUEUED (-1) // counter of a cell that is not in the queue

/* TYPE DEFINITIONS ----------------------------------------------------------*/
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
// grid (including the initial cell, the goal cell and the blocks)
typedef struct {
    int rows; // number of rows in the grid
    int cols; // number of columns in the grid
    cell_t initial; // the initial cell (I)
    cell_t goal; // the goal cell (G)
    char *blocked; // rows*cols flags, non-zero where a cell is blocked
    int nblocks; // number of block lines in the current configuration
} grid_t;

// row and column offsets of the moves: up, down, left, right
const int MOVE_ROW[NUM_MOVES] = {-1, 1, 0, 0};
const int MOVE_COL[NUM_MOVES] = {0, 0, -1, 1};

/* FUNCTION PROTOTYPES -------------------------------------------------------*/
route_t* make_empty_route(void);
void free_state(state_t*);
void free_route(route_t* R);
route_t* insert_at_head(route_t*, cell_t*);
route_t* insert_at_tail(route_t*, cell_t*);

// stages
int stage_zero(grid_t *grid, route_t *route);
void stage_one(grid_t *grid, route_t *route, int status);
void stage_two(grid_t *grid, route_t *route);
void repair_and_report(grid_t *grid, route_t *route, int repair_all);
// input
cell_t* make_cell(short row, short col);
int read_cell(cell_t *cell);
void read_header(grid_t *grid);
int read_blocks(grid_t *grid);
int read_route(route_t *route);
void free_grid(grid_t *grid);
// route checks
int same_cell(cell_t *a, cell_t *b);
int in_grid(grid_t *grid, int row, int col);
int is_block(grid_t *grid, int row, int col);
int is_legal_move(cell_t *from, cell_t *to);
int route_status(grid_t *grid, route_t *route);
// route repair
int repair_route(grid_t *grid, route_t *route);
int fill_counters(grid_t *grid, cell_t *s, char *target, int *counter,
    cell_t *t);
route_t* trace_back(grid_t *grid, int *counter, cell_t *t);
// output
void print_grid_info(grid_t *grid);
void print_route(route_t *route);
void print_route_status(int status);
void print_grid(grid_t *grid, route_t *route);

/* WHERE IT ALL HAPPENS ------------------------------------------------------*/
int
main(int argc, char *argv[]) {
    grid_t grid;
    route_t *route;
    int status, more;

    // read the grid, the first block configuration and the route
    read_header(&grid);
    read_blocks(&grid);
    route = make_empty_route();
    more = (read_route(route) == READ_SEP); // '$' => more configurations

    status = stage_zero(&grid, route);
    stage_one(&grid, route, status);
    // only a route that is otherwise valid can be repaired around blocks
    if (more && (status == ROUTE_VALID || status == ROUTE_INVALID_BLOCK)) {
        stage_two(&grid, route);
    }
    printf(SEP3);

    free_route(route);
    free_grid(&grid);
    return EXIT_SUCCESS; // we are done! algorithms are fun!!!
}

/* STAGES --------------------------------------------------------------------*/

// Stage 0: prints the grid information and the route, and the route status
int
stage_zero(grid_t *grid, route_t *route) {
    int status;
    printf(SEP1, 0);
    print_grid_info(grid);
    printf("The proposed route in the grid is:\n");
    print_route(route);
    status = route_status(grid, route);
    print_route_status(status);
    return status;
}

// Stage 1: draws the grid and route; if the route visits a block (status 4),
// repairs only its first broken segment
void
stage_one(grid_t *grid, route_t *route, int status) {
    printf(SEP1, 1);
    print_grid(grid, route);
    if (status != ROUTE_INVALID_BLOCK) {
        return;
    }
    repair_and_report(grid, route, 0);
}

// Stage 2: loads each further block configuration and repairs all broken
// segments of the most recent route
void
stage_two(grid_t *grid, route_t *route) {
    int more = 1, first = 1;
    printf(SEP1, 2);
    while (more) {
        more = (read_blocks(grid) == READ_SEP);
        if (!first) {
            printf(SEP3);
        }
        first = 0;
        print_grid(grid, route);
        if (route_status(grid, route) == ROUTE_INVALID_BLOCK) {
            repair_and_report(grid, route, 1);
        }
    }
}

// repairs the first broken segment (or all of them, if repair_all is set),
// then prints the repaired grid and route, or the failure message
void
repair_and_report(grid_t *grid, route_t *route, int repair_all) {
    int result;
    printf(SEP2);
    result = repair_route(grid, route);
    while (repair_all && result == REPAIR_DONE) {
        result = repair_route(grid, route);
    }
    print_grid(grid, route);
    printf(SEP2);
    if (result == REPAIR_FAILED) {
        printf("The route cannot be repaired!\n");
        return;
    }
    print_route(route);
    print_route_status(route_status(grid, route));
}

/* LINKED LIST ---------------------------------------------------------------*/

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

/* INPUT ---------------------------------------------------------------------*/

// allocates a new cell with the given coordinates
cell_t
*make_cell(short row, short col) {
    cell_t *cell;
    cell = (cell_t*)malloc(sizeof(*cell));
    assert(cell != NULL);
    cell->row = row;
    cell->col = col;
    cell->counter = 0;
    return cell;
}

// reads the next "[r,c]" from the input, skipping whatever comes before it
// (such as "->" and newlines); returns READ_CELL, or READ_SEP / READ_EOF if
// a '$' or the end of the input comes first
int
read_cell(cell_t *cell) {
    int c;
    while ((c = getchar()) != EOF && c != '[') {
        if (c == SEP0) {
            return READ_SEP;
        }
    }
    if (c == EOF || scanf("%hd,%hd]", &cell->row, &cell->col) != 2) {
        return READ_EOF;
    }
    cell->counter = 0;
    return READ_CELL;
}

// reads the "RxC" grid dimensions, the initial cell and the goal cell
void
read_header(grid_t *grid) {
    if (scanf(" %dx%d", &grid->rows, &grid->cols) != 2) {
        grid->rows = DEFAULT_ROWS;
        grid->cols = DEFAULT_COLS;
    }
    if (read_cell(&grid->initial) != READ_CELL
        || read_cell(&grid->goal) != READ_CELL) {
        printf("malformed input\n");
        exit(EXIT_FAILURE);
    }
    grid->blocked = (char*)malloc(grid->rows*grid->cols);
    assert(grid->blocked != NULL);
    grid->nblocks = 0;
}

// reads one block configuration, up to a '$' line or the end of the input;
// it replaces the previous configuration
int
read_blocks(grid_t *grid) {
    cell_t cell;
    int i, result;
    for (i = 0; i < grid->rows*grid->cols; i++) {
        grid->blocked[i] = 0;
    }
    grid->nblocks = 0;
    while ((result = read_cell(&cell)) == READ_CELL) {
        if (in_grid(grid, cell.row, cell.col)) {
            grid->blocked[cell.row*grid->cols + cell.col] = 1;
        }
        grid->nblocks++;
    }
    return result;
}

// reads the route into a linked list, up to a '$' line or the end of input
int
read_route(route_t *route) {
    cell_t cell;
    int result;
    while ((result = read_cell(&cell)) == READ_CELL) {
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

/* ROUTE CHECKS --------------------------------------------------------------*/

int
same_cell(cell_t *a, cell_t *b) {
    return a->row == b->row && a->col == b->col;
}

int
in_grid(grid_t *grid, int row, int col) {
    return row >= 0 && row < grid->rows && col >= 0 && col < grid->cols;
}

int
is_block(grid_t *grid, int row, int col) {
    return in_grid(grid, row, col) && grid->blocked[row*grid->cols + col];
}

// a legal move goes exactly one cell up, down, left or right
int
is_legal_move(cell_t *from, cell_t *to) {
    return abs(from->row - to->row) + abs(from->col - to->col) == 1;
}

// checks statuses 1 to 4 in order, and reports the first one that holds
int
route_status(grid_t *grid, route_t *route) {
    state_t *s;
    if (route->head == NULL || !same_cell(route->head->cell, &grid->initial)) {
        return ROUTE_INVALID_INITIAL;
    }
    if (!same_cell(route->tail->cell, &grid->goal)) {
        return ROUTE_INVALID_GOAL;
    }
    for (s = route->head; s != NULL; s = s->next) {
        if (!in_grid(grid, s->cell->row, s->cell->col)
            || (s->next != NULL && !is_legal_move(s->cell, s->next->cell))) {
            return ROUTE_INVALID_MOVE;
        }
    }
    for (s = route->head; s != NULL; s = s->next) {
        if (is_block(grid, s->cell->row, s->cell->col)) {
            return ROUTE_INVALID_BLOCK;
        }
    }
    return ROUTE_VALID;
}

/* ROUTE REPAIR --------------------------------------------------------------*/

// repairs the first broken segment of the route (the one closest to the
// initial cell); the segment starts at cell s, the last cell before the
// first block on the route
int
repair_route(grid_t *grid, route_t *route) {
    state_t *start = NULL, *s, *end = NULL, *next;
    int *counter, i, size = grid->rows*grid->cols;
    char *target;
    cell_t t;
    route_t *path;

    // find the first blocked state, and the state s just before it
    for (s = route->head; s != NULL
        && !is_block(grid, s->cell->row, s->cell->col); s = s->next) {
        start = s;
    }
    if (s == NULL) {
        return REPAIR_NONE;
    }
    if (start == NULL) { // the initial cell itself is blocked
        return REPAIR_FAILED;
    }

    // the cells of the route following the broken segment are the targets
    target = (char*)malloc(size);
    counter = (int*)malloc(size*sizeof(*counter));
    assert(target != NULL && counter != NULL);
    for (i = 0; i < size; i++) {
        target[i] = 0;
        counter[i] = NOT_QUEUED;
    }
    for (; s != NULL; s = s->next) {
        if (!is_block(grid, s->cell->row, s->cell->col)) {
            target[s->cell->row*grid->cols + s->cell->col] = 1;
        }
    }

    if (!fill_counters(grid, start->cell, target, counter, &t)) {
        free(target);
        free(counter);
        return REPAIR_FAILED;
    }

    // build the new fragment from s to t, then find t in the route after
    // the first block
    path = trace_back(grid, counter, &t);
    for (s = start->next->next; s != NULL && end == NULL; s = s->next) {
        if (same_cell(s->cell, &t)) {
            end = s;
        }
    }
    assert(end != NULL);

    // replace the states strictly between s and t with the new fragment
    s = start->next;
    while (s != end) {
        next = s->next;
        free_state(s);
        s = next;
    }
    if (path->head == NULL) { // s and t are adjacent
        start->next = end;
    } else {
        start->next = path->head;
        path->tail->next = end;
    }
    free(path);
    free(target);
    free(counter);
    return REPAIR_DONE;
}

// traverses a queue of (cell, counter) pairs, starting with (s, 0); for each
// pair, the adjacent cells (up, down, left, right) that are in the grid, not
// blocked and not yet queued are added with counter + 1; stops as soon as a
// target cell is added, which is stored in t; returns 0 if none is reached
int
fill_counters(grid_t *grid, cell_t *s, char *target, int *counter,
    cell_t *t) {
    cell_t *queue, cur, adj;
    int head = 0, tail = 0, m, found = 0;

    queue = (cell_t*)malloc(grid->rows*grid->cols*sizeof(*queue));
    assert(queue != NULL);
    cur = *s;
    cur.counter = 0;
    counter[cur.row*grid->cols + cur.col] = 0;
    queue[tail++] = cur;

    while (head < tail && !found) {
        cur = queue[head++];
        for (m = 0; m < NUM_MOVES && !found; m++) {
            adj.row = cur.row + MOVE_ROW[m];
            adj.col = cur.col + MOVE_COL[m];
            adj.counter = cur.counter + 1;
            if (!in_grid(grid, adj.row, adj.col)
                || is_block(grid, adj.row, adj.col)
                || counter[adj.row*grid->cols + adj.col] != NOT_QUEUED) {
                continue;
            }
            counter[adj.row*grid->cols + adj.col] = adj.counter;
            queue[tail++] = adj;
            if (target[adj.row*grid->cols + adj.col]) {
                *t = adj;
                found = 1;
            }
        }
    }
    free(queue);
    return found;
}

// walks backwards from t towards s (the cell with counter 0), each time
// moving to the adjacent queued cell with the smallest counter (ties go to
// up, down, left, right, in that order); returns the cells strictly between
// s and t, in route order
route_t
*trace_back(grid_t *grid, int *counter, cell_t *t) {
    route_t *path = make_empty_route();
    int row = t->row, col = t->col, m, r, c, best, best_row, best_col;

    while (1) {
        best = -1;
        best_row = row;
        best_col = col;
        for (m = 0; m < NUM_MOVES; m++) {
            r = row + MOVE_ROW[m];
            c = col + MOVE_COL[m];
            if (in_grid(grid, r, c) && counter[r*grid->cols + c] != NOT_QUEUED
                && (best < 0 || counter[r*grid->cols + c] < best)) {
                best = counter[r*grid->cols + c];
                best_row = r;
                best_col = c;
            }
        }
        row = best_row;
        col = best_col;
        if (best == 0) { // reached s
            break;
        }
        insert_at_head(path, make_cell(row, col));
    }
    return path;
}

/* OUTPUT --------------------------------------------------------------------*/

void
print_grid_info(grid_t *grid) {
    printf("The grid has %d rows and %d columns.\n", grid->rows, grid->cols);
    printf("The grid has %d block(s).\n", grid->nblocks);
    printf("The initial cell in the grid is [%d,%d].\n",
        grid->initial.row, grid->initial.col);
    printf("The goal cell in the grid is [%d,%d].\n",
        grid->goal.row, grid->goal.col);
}

// prints a route, at most MAX_CELLS_PER_LINE cells per line
void
print_route(route_t *route) {
    state_t *s;
    int count = 0;
    for (s = route->head; s != NULL; s = s->next) {
        printf("[%d,%d]", s->cell->row, s->cell->col);
        count++;
        if (s->next == NULL) {
            printf(".\n");
        } else {
            printf("->");
            if (count%MAX_CELLS_PER_LINE == 0) {
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

// draws the grid: I, G and # take priority over * for visited cells
void
print_grid(grid_t *grid, route_t *route) {
    char *codes;
    state_t *s;
    int r, c, i, size = grid->rows*grid->cols;

    codes = (char*)malloc(size);
    assert(codes != NULL);
    for (i = 0; i < size; i++) {
        codes[i] = CELL_CODE_EMPTY;
    }
    for (s = route->head; s != NULL; s = s->next) {
        if (in_grid(grid, s->cell->row, s->cell->col)) {
            codes[s->cell->row*grid->cols + s->cell->col] = CELL_CODE_VISITED;
        }
    }
    for (i = 0; i < size; i++) {
        if (grid->blocked[i]) {
            codes[i] = CELL_CODE_BLOCK;
        }
    }
    if (in_grid(grid, grid->initial.row, grid->initial.col)) {
        codes[grid->initial.row*grid->cols + grid->initial.col] =
            CELL_CODE_INITIAL;
    }
    if (in_grid(grid, grid->goal.row, grid->goal.col)) {
        codes[grid->goal.row*grid->cols + grid->goal.col] = CELL_CODE_GOAL;
    }

    printf(" ");
    for (c = 0; c < grid->cols; c++) {
        printf("%d", c%10);
    }
    printf("\n");
    for (r = 0; r < grid->rows; r++) {
        printf("%d", r%10);
        for (c = 0; c < grid->cols; c++) {
            putchar(codes[r*grid->cols + c]);
        }
        printf("\n");
    }
    free(codes);
}
