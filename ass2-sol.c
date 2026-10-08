#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
/* DEFINITIONS ------------------------------------------------------------*/
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
#define NUM_MOVES 4 // moves are up, down, left, right (in this order)

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
// grid (including initial cell, goal cell and blocks)
typedef struct {
    int rows; // the number of rows in the grid
    int cols; // the number of columns in the grid
    cell_t initial; // the initial cell (I)
    cell_t goal; // the goal cell (G)
    char *blocked; // rows * cols flags, non-zero where a cell is blocked
    int nblocks; // the number of blocked cells
} grid_t;

// row & column offsets of the moves: up, down, left, right
const int MOVE_ROW[NUM_MOVES] = {-1, 1, 0, 0};
const int MOVE_COL[NUM_MOVES] = {0, 0, -1, 1};

/* FUNCTION PROTOTYPES -------------------------------------------------------*/
route_t* make_empty_route(void);
void free_state(state_t*);
void free_route(route_t* R);
route_t* insert_at_head(route_t*, cell_t*);
route_t* insert_at_tail(route_t*, cell_t*);

cell_t* make_cell(short row, short col);
int read_cell(cell_t *cell);
void free_grid(grid_t *grid);
int route_status(grid_t *grid, route_t *route);
int same_cell(cell_t *a, cell_t *b);
int in_grid(grid_t *grid, cell_t *cell);
int is_block(grid_t *grid, cell_t *cell);
void print_grid_info(grid_t *grid);
void print_route(route_t *route);
void print_route_status(int status);
void print_grid(grid_t *grid, route_t *route);
int is_legal_move(cell_t *from, cell_t *to);
void read_header(grid_t *grid);
int read_blocks(grid_t *grid);
int read_route(route_t *route);
int stage_one(grid_t *grid, route_t *route);
int stage_two_case(grid_t *grid, route_t *route);
int repair_route(grid_t *grid, route_t *route);



/* WHERE IT ALL HAPPENS ------------------------------------------------------*/
int
main(void) {
    grid_t grid;
    route_t *route;
    int more, status, ok, first;
    /* STAGE 0: ---------------------------------------------------------------*/
    read_header(&grid);
    read_blocks(&grid);
    route = make_empty_route();
    more = (read_route(route) == 0);
    printf(SEP1, 0);
    print_grid_info(&grid);
    printf("The proposed route in the grid is:\n");
    print_route(route);
    status = route_status(&grid, route);
    print_route_status(status);
    /* STAGE 1: ---------------------------------------------------------------*/
    printf(SEP1, 1);
    ok = stage_one(&grid, route);
    /* STAGE 2: ---------------------------------------------------------------*/
    if (ok && more){
        printf(SEP1, 2);
        first = 1;
        while (ok && more){
            more = (read_blocks(&grid) == 0);
            if (!first){
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
*make_empty_route(void){
    route_t *R;
    R = (route_t*)malloc(sizeof(*R));
    assert(R != NULL);
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
*insert_at_head(route_t* R, cell_t* addr){
    assert(R != NULL);
    state_t *new;
    new = (state_t*)malloc(sizeof(*new));
    assert(new != NULL);
    new->cell = addr;
    new->next = R->head;
    R->head = new;
    if (R->tail == NULL){ /* This is the first insertion into the route*/
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

/* INPUTTING ---------------------------------------------------------*/
// To allocate a new cell with the given coordinates.
cell_t
*make_cell(short row, short col){
    cell_t *cell;
    cell = (cell_t*)malloc(sizeof(*cell));
    assert(cell != NULL);
    cell->row = row;
    cell->col = col;
    cell->counter = 0;
    return cell;
}
// Reads the next "[r, c]" pattern from the input, and ignoring everything
// that apppears before it.
int
read_cell(cell_t *cell){
    int c;
    while ((c=getchar()) != EOF && c!= '['){
        if (c == SEP0){
            return 0;
        }
    }
    if (c == EOF || scanf("%hd, %hd]", &cell->row, &cell->col) != 2){
        return -1;
    }
    cell->counter = 0;
    return 1;
}

void
read_header(grid_t *grid){
    if (scanf(" %dx%d", &grid->rows, &grid->cols) != 2){
        grid->rows = 10;
        grid->cols = 10;
    }
    if (read_cell(&grid->initial) != 1 || read_cell(&grid->goal) != 1){
        printf("malformed input\n");
        exit(EXIT_FAILURE);
    }
    grid->blocked = (char*)malloc(grid->rows*grid->cols);
    assert(grid->blocked != NULL);
    grid->nblocks = 0;
}
int
read_blocks(grid_t *grid){
    cell_t cell;
    int i, result;
    for (i = 0; i<grid->rows*grid->cols; i++){
        grid->blocked[i] = 0;
    }
    grid->nblocks = 0;
    while ((result = read_cell(&cell)) == 1){
        if (in_grid(grid, &cell)){
            grid->blocked[cell.row*grid-> cols + cell.col] = 1;
        }
        grid->nblocks++;
    }
    return result;
}
int
read_route(route_t *route){
    cell_t cell;
    int result;
    while ((result = read_cell(&cell)) == 1){
        insert_at_tail(route, make_cell(cell.row, cell.col));
    }
    return result;
}

void free_grid(grid_t *grid){
    free(grid->blocked);
    grid->blocked = NULL;
    grid->nblocks = 0;
}
/*ROUTE CHECKING ---------------------------------------------------------*/
int
same_cell(cell_t *a, cell_t *b){
    return a->row==b->row && a->col == b->col;
}
int
in_grid(grid_t *grid, cell_t *cell){
    return cell->row>=0 && cell->row<grid->rows
    && cell->col>=0 && cell->col<grid->cols;
}
int
is_block(grid_t *grid, cell_t *cell){
    return in_grid(grid, cell) && grid->blocked[cell->row*grid->cols
    + cell->col];
}
// Can only move one cell exactly (up, down, left or right)
int
is_legal_move(cell_t *from, cell_t *to){
    return abs(from->row - to->row) + abs(from->col - to->col) == 1;
}
int
route_status(grid_t *grid, route_t *route){
    state_t *s;

    if (route->head == NULL || !same_cell(route->head->cell, &grid->initial)){
        return ROUTE_INVALID_INITIAL;
    }
    if (!same_cell(route->tail->cell, &grid->goal)){
        return ROUTE_INVALID_GOAL;
    }
    for (s = route->head; s != NULL; s = s->next){
        if (!in_grid(grid, s->cell)
        || (s->next != NULL && !is_legal_move(s->cell, s->next->cell))){
            return ROUTE_INVALID_MOVE;
        }
    }
    for (s = route->head; s!= NULL; s = s->next){
        if (is_block(grid, s->cell)){
            return ROUTE_INVALID_BLOCK;
        }
    }
    return ROUTE_VALID;
}
/* ROUTE REPAIR --------------------------------------------------------*/
int
repair_route(grid_t *grid, route_t *route){
    state_t *start = NULL, *s, *end = NULL, *next;
    cell_t *queue, cur, nbr;
    int *dist, *target, size = grid->rows*grid->cols;
    int head = 0, tail = 0, found = 0, i, m, idx;
    route_t *path;
    for (s = route->head; s!= NULL && !is_block(grid, s->cell);
    s = s->next){
        start = s;
    }
    if (s == NULL){
        return 0;
    }
    if (start == NULL){
        return 2;
    }
    dist = (int*)malloc(size*sizeof(*dist));
    target = (int*)malloc(size*sizeof(*target));
    queue = (cell_t*)malloc(size*sizeof(*queue));
    assert(dist != NULL && target != NULL && queue != NULL);
    for (i = 0; i<size; i++){
        dist[i] = -1;
        target[i] = 0;
    }
    for (; s != NULL; s = s->next){
        if (in_grid(grid, s->cell) && !is_block(grid, s->cell)){
            target[s->cell->row*grid->cols + s->cell->col] = 1;
        }
    }
    cur = *start->cell;
    cur.counter = 0;
    dist[cur.row*grid->cols + cur.col] = 0;
    queue[tail++] = cur;
    while (head<tail && !found){
        cur = queue[head++];
        for (m = 0; m<NUM_MOVES && !found; m++){
            nbr.row = cur.row + MOVE_ROW[m];
            nbr.col = cur.col + MOVE_COL[m];
            nbr.counter = cur.counter + 1;
            idx = nbr.row*grid->cols + nbr.col;
            if (!in_grid(grid, &nbr) || is_block(grid, &nbr) || dist[idx] >= 0){
                continue;
            }
            dist[idx] = nbr.counter;
            queue[tail++] = nbr;
            if (target[idx]){
                found = 1;
            }
        }
    }
    if (!found){
        free(dist);
        free(target);
        free(queue);
        return 2;
    }
    path = make_empty_route();
    cur = nbr;
    while (cur.counter>0){
        insert_at_head(path, make_cell(cur.row, cur.col));
        for (m = 0; m<NUM_MOVES; m++){
            nbr.row = cur.row + MOVE_ROW[m];
            nbr.col = cur.col + MOVE_COL[m];
            if (in_grid(grid, &nbr) && dist[nbr.row*grid->cols + nbr.col]
            == (int)cur.counter -1){
                break;
            }
        }
        nbr.counter = cur.counter - 1;
        cur = nbr;
    }
    for (s = start->next->next; s != NULL && end == NULL; s = s->next){
        if (!is_block(grid, s->cell) && same_cell(s->cell, path->tail->cell)){
            end = s;
        }
    }
    assert(end != NULL);
    s = start->next;
    while (s != end){
        next = s->next;
        free_state(s);
        s = next;
    }
    start->next = path->head;
    path->tail->next = end->next;
    if (route->tail == end){
        route->tail = path->tail;
    }
    free_state(end);
    free(path);
    free(dist);
    free(target);
    free(queue);
    return 1;
}
// Stage 1: drawing the route and repair the first broken segment, and
// if the route can't be repaired, return 0
int
stage_one(grid_t *grid, route_t *route){
    int result;
    print_grid(grid, route);
    if (route_status(grid, route) != ROUTE_INVALID_BLOCK){
        return route_status(grid, route) == ROUTE_VALID;
    }
    printf(SEP2);
    result = repair_route(grid, route);
    print_grid(grid, route);
    printf(SEP2);
    if (result == 2){
        printf("The route cannot be repaired!\n");
        return 0;
    }
    print_route(route);
    print_route_status(route_status(grid, route));
    return 1;
}
// Stage 2: drawing the route with the current blocks and repairs every broken segment of it;
// if the route cannot be repaired, return 0.
int
stage_two_case(grid_t *grid, route_t *route){
    int result;
    print_grid(grid, route);
    if (route_status(grid, route) != ROUTE_INVALID_BLOCK){
        return 1;
    }
    printf(SEP2);
    while ((result = repair_route(grid, route)) == 1){

    }
    print_grid(grid, route);
    printf(SEP2);
    if (result == 2){
        printf("The route cannot be repaired!\n");
        return 0;
    }
    print_route(route);
    print_route_status(route_status(grid, route));
    return 1;
}
/* OUTPUT ----------------------------------------------------------------*/
void
print_grid_info(grid_t *grid){
    printf("The grid has %d rows and %d columns.\n",
    grid->rows, grid->cols);
    printf("The grid has %d block(s).\n", grid->nblocks);
    printf("The initial cell in the grid is [%d,%d].\n",
    grid->initial.row, grid->initial.col);
    printf("The goal cell in the grid is [%d,%d].\n",
    grid->goal.row, grid->goal.col);
}

void
print_route(route_t *route){
    state_t *s;
    int count = 0;
    for (s = route->head; s!= NULL; s = s->next){
        printf("[%d,%d]", s->cell->row, s->cell->col);
        count++;
        if (s->next == NULL){
            printf(".\n");
        } else {
            printf("->");
            if (count%MAX_CELLS_PER_LINE == 0){
                printf("\n");
            }
        }
    }
}

void
print_route_status(int status){
    switch (status){
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
print_grid(grid_t *grid, route_t *route){
    char *codes;
    state_t *s;
    int r, c, i, last, size = grid->rows*grid->cols;
    codes = (char*)malloc(size);
    assert(codes!= NULL);
    for (i=0; i<size; i++){
        codes[i] = CELL_CODE_EMPTY;
    }
    for (s=route->head; s!=NULL; s=s->next){
        if (in_grid(grid, s->cell)){
            codes[s->cell->row*grid->cols + s->cell->col] = CELL_CODE_VISITED;
        }
    }
    for (i=0; i<size; i++){
        if (grid->blocked[i]){
            codes[i] = CELL_CODE_BLOCK;
        }
    }
    if (in_grid(grid, &grid->initial)){
        codes[grid->initial.row*grid->cols + grid->initial.col] = CELL_CODE_INITIAL;
    }
    if (in_grid(grid, &grid->goal)){
        codes[grid->goal.row*grid->cols + grid->goal.col] = CELL_CODE_GOAL;
    }
    printf(" ");
    for (c=0; c<grid->cols; c++){
        printf("%d", c%10);
    }
    printf("\n");
    for (r=0; r<grid->rows; r++){
        printf("%d", r%10);
        /* find the last non-empty cell, so no trailing spaces are printed */
        last = grid->cols - 1;
        while (last>=0 && codes[r*grid->cols + last] == CELL_CODE_EMPTY){
            last--;
        }
        for (c=0; c<=last; c++){
            putchar(codes[r*grid->cols + c]);
        }
        printf("\n");
    }
    free(codes);
}
