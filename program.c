/* Grid path checker: reads a grid description (initial cell, goal cell,
 * blocks) followed by a proposed route, reports on the route's validity,
 * and draws the grid with the route on it.
 *
 * Input format:
 *   [RxC]          optional grid dimensions, e.g. 10x10 (default 10x10)
 *   [r,c]          initial cell
 *   [r,c]          goal cell
 *   [r,c] ...      zero or more blocked cells, one per line
 *   $              separator
 *   [r,c]->[r,c]->...  the proposed route, may be split across lines
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DEFAULT_ROWS 10
#define DEFAULT_COLS 10
#define CELLS_PER_LINE 5
#define LINE_LEN 256
#define SEPARATOR '$'

#define CHAR_EMPTY ' '
#define CHAR_BLOCK '#'
#define CHAR_ROUTE '*'
#define CHAR_INIT 'I'
#define CHAR_GOAL 'G'

#define STATUS_VALID 0
#define STATUS_BAD_INIT 1
#define STATUS_BAD_GOAL 2
#define STATUS_BAD_MOVE 3
#define STATUS_BLOCKED 4

#define BANNER_WIDTH 48

typedef struct {
    int row, col;
} cell_t;

typedef struct node node_t;
struct node {
    cell_t cell;
    node_t *next;
};

typedef struct {
    node_t *head, *foot;
    int len;
} list_t;

typedef struct {
    int rows, cols;
    cell_t init, goal;
    cell_t *blocks;
    int nblocks;
} grid_t;

/* ---------------- linked list ---------------- */

static void list_init(list_t *list) {
    list->head = list->foot = NULL;
    list->len = 0;
}

static void list_append(list_t *list, cell_t cell) {
    node_t *node = malloc(sizeof(*node));
    if (node == NULL) {
        fprintf(stderr, "out of memory\n");
        exit(EXIT_FAILURE);
    }
    node->cell = cell;
    node->next = NULL;
    if (list->foot == NULL) {
        list->head = list->foot = node;
    } else {
        list->foot->next = node;
        list->foot = node;
    }
    list->len++;
}

static void list_free(list_t *list) {
    node_t *curr = list->head, *next;
    while (curr != NULL) {
        next = curr->next;
        free(curr);
        curr = next;
    }
    list_init(list);
}

/* ---------------- input ---------------- */

/* Reads the next "[r,c]" cell from stdin, skipping anything before it.
 * Returns 1 on success, 0 if the separator or end of input is reached. */
static int read_cell(cell_t *cell) {
    int c;
    while ((c = getchar()) != EOF && c != '[') {
        if (c == SEPARATOR) {
            return 0;
        }
    }
    if (c == EOF) {
        return 0;
    }
    if (scanf("%d,%d]", &cell->row, &cell->col) != 2) {
        return 0;
    }
    return 1;
}

static void read_grid(grid_t *grid) {
    char line[LINE_LEN];
    int capacity = 4;
    cell_t cell;

    grid->rows = DEFAULT_ROWS;
    grid->cols = DEFAULT_COLS;

    /* the first non-empty line is either the dimensions or the initial cell */
    do {
        if (fgets(line, LINE_LEN, stdin) == NULL) {
            fprintf(stderr, "unexpected end of input\n");
            exit(EXIT_FAILURE);
        }
    } while (strspn(line, " \t\r\n") == strlen(line));

    if (sscanf(line, " [%d,%d]", &grid->init.row, &grid->init.col) != 2) {
        if (sscanf(line, "%dx%d", &grid->rows, &grid->cols) != 2
            || !read_cell(&grid->init)) {
            fprintf(stderr, "malformed input\n");
            exit(EXIT_FAILURE);
        }
    }
    if (!read_cell(&grid->goal)) {
        fprintf(stderr, "malformed input\n");
        exit(EXIT_FAILURE);
    }

    grid->nblocks = 0;
    grid->blocks = malloc(capacity * sizeof(*grid->blocks));
    if (grid->blocks == NULL) {
        fprintf(stderr, "out of memory\n");
        exit(EXIT_FAILURE);
    }
    /* blocks continue until the separator line */
    while (read_cell(&cell)) {
        if (grid->nblocks == capacity) {
            capacity *= 2;
            grid->blocks = realloc(grid->blocks,
                                   capacity * sizeof(*grid->blocks));
            if (grid->blocks == NULL) {
                fprintf(stderr, "out of memory\n");
                exit(EXIT_FAILURE);
            }
        }
        grid->blocks[grid->nblocks++] = cell;
    }
}

static void read_route(list_t *route) {
    cell_t cell;
    list_init(route);
    while (read_cell(&cell)) {
        list_append(route, cell);
    }
}

/* ---------------- route checks ---------------- */

static int same_cell(cell_t a, cell_t b) {
    return a.row == b.row && a.col == b.col;
}

static int in_grid(const grid_t *grid, cell_t cell) {
    return cell.row >= 0 && cell.row < grid->rows
        && cell.col >= 0 && cell.col < grid->cols;
}

static int is_block(const grid_t *grid, cell_t cell) {
    int i;
    for (i = 0; i < grid->nblocks; i++) {
        if (same_cell(grid->blocks[i], cell)) {
            return 1;
        }
    }
    return 0;
}

static int is_legal_move(cell_t from, cell_t to) {
    return abs(from.row - to.row) + abs(from.col - to.col) == 1;
}

static int route_status(const grid_t *grid, const list_t *route) {
    node_t *curr;

    if (route->head == NULL || !same_cell(route->head->cell, grid->init)) {
        return STATUS_BAD_INIT;
    }
    if (!same_cell(route->foot->cell, grid->goal)) {
        return STATUS_BAD_GOAL;
    }
    for (curr = route->head; curr != NULL; curr = curr->next) {
        if (!in_grid(grid, curr->cell)
            || (curr->next != NULL
                && !is_legal_move(curr->cell, curr->next->cell))) {
            return STATUS_BAD_MOVE;
        }
    }
    for (curr = route->head; curr != NULL; curr = curr->next) {
        if (is_block(grid, curr->cell)) {
            return STATUS_BLOCKED;
        }
    }
    return STATUS_VALID;
}

/* ---------------- output ---------------- */

static void print_route(const list_t *route) {
    node_t *curr;
    int count = 0;
    for (curr = route->head; curr != NULL; curr = curr->next) {
        printf("[%d,%d]", curr->cell.row, curr->cell.col);
        count++;
        if (curr->next == NULL) {
            printf(".\n");
        } else {
            printf("->");
            if (count % CELLS_PER_LINE == 0) {
                printf("\n");
            }
        }
    }
}

static void print_banner(const char *title) {
    int printed = printf("%s", title);
    while (printed++ < BANNER_WIDTH) {
        putchar('=');
    }
    putchar('\n');
}

static void print_grid(const grid_t *grid, const list_t *route) {
    char *cells;
    node_t *curr;
    int r, c, i;

    cells = malloc((size_t)grid->rows * grid->cols);
    if (cells == NULL) {
        fprintf(stderr, "out of memory\n");
        exit(EXIT_FAILURE);
    }
    memset(cells, CHAR_EMPTY, (size_t)grid->rows * grid->cols);

    for (i = 0; i < grid->nblocks; i++) {
        if (in_grid(grid, grid->blocks[i])) {
            cells[grid->blocks[i].row * grid->cols
                  + grid->blocks[i].col] = CHAR_BLOCK;
        }
    }
    for (curr = route->head; curr != NULL; curr = curr->next) {
        if (in_grid(grid, curr->cell)) {
            cells[curr->cell.row * grid->cols + curr->cell.col] = CHAR_ROUTE;
        }
    }
    if (in_grid(grid, grid->init)) {
        cells[grid->init.row * grid->cols + grid->init.col] = CHAR_INIT;
    }
    if (in_grid(grid, grid->goal)) {
        cells[grid->goal.row * grid->cols + grid->goal.col] = CHAR_GOAL;
    }

    putchar(' ');
    for (c = 0; c < grid->cols; c++) {
        printf("%d", c % 10);
    }
    putchar('\n');
    for (r = 0; r < grid->rows; r++) {
        printf("%d", r % 10);
        for (c = 0; c < grid->cols; c++) {
            putchar(cells[r * grid->cols + c]);
        }
        putchar('\n');
    }
    free(cells);
}

/* ---------------- main ---------------- */

int main(void) {
    grid_t grid;
    list_t route;
    int status;

    read_grid(&grid);
    read_route(&route);

    printf("The grid has %d rows and %d columns.\n", grid.rows, grid.cols);
    printf("The grid has %d block(s).\n", grid.nblocks);
    printf("The initial cell in the grid is [%d,%d].\n",
           grid.init.row, grid.init.col);
    printf("The goal cell in the grid is [%d,%d].\n",
           grid.goal.row, grid.goal.col);
    printf("The proposed route in the grid is:\n");
    print_route(&route);

    status = route_status(&grid, &route);
    switch (status) {
    case STATUS_BAD_INIT:
        printf("Initial cell in the route is wrong!\n");
        break;
    case STATUS_BAD_GOAL:
        printf("Goal cell in the route is wrong!\n");
        break;
    case STATUS_BAD_MOVE:
        printf("There is an illegal move in this route!\n");
        break;
    case STATUS_BLOCKED:
        printf("There is a block on this route!\n");
        break;
    default:
        printf("The route is valid!\n");
        break;
    }

    print_banner("==STAGE 1");
    print_grid(&grid, &route);
    print_banner("");

    list_free(&route);
    free(grid.blocks);
    return 0;
}
