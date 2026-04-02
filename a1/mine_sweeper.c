#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_SIZE 20

typedef struct List {
    int length;
    int** array;
    int* x;
    int* y;
} List;

typedef struct Board {
    int (*board)[MAX_SIZE];
    int rows;
    int cols;
} Board;

Board make_board(int board[][MAX_SIZE], int rows, int cols) {
    Board result;
    result.board = board;
    result.rows = rows;
    result.cols = cols;
    return result;
}

// getter, using 1 based indexing and x, y order. returns -1 if out of bounds
int get(Board board, int x, int y) {
    if (x > board.cols || x <= 0) {
        return -1;
    }
    else if (y > board.rows || y <= 0) {
        return -1;
    }
    return board.board[y-1][x-1];
}

// get the adress of x, y, NULL if out of bounds
int* get_ptr(Board board, int x, int y) {
    if (x > board.cols || x <= 0) {
        return NULL;
    }
    else if (y > board.rows || y <= 0) {
        return NULL;
    }
    return &board.board[y-1][x-1];
}


/**
 * Read the board dimensions and cell values from the file pointer fp to
 * initialize board.
 *    - fp may point to stdin or to an open file
 *    - fp is not NULL
 * Format: First line contains rows and cols
 *         Following lines contain the values found in each row of the
 *         board: -1 (mine) or 0 (safe)
 * Assume each line after the first contains cols number of values
 * and there are rows number of lines following the first line.
 * Note that fp is already open, so you need to use fscanf() instead of scanf()
 * to read from fp. Also note that this function only reads the board, it does
 * not read any moves.
 */
void read_board(FILE *fp, int board[][MAX_SIZE], int *rows, int *cols) {
    if (fscanf(fp, "%d %d", rows, cols) != 2) return;

    for (int i = 0; i < *rows; i++) {
        for (int j = 0; j < *cols; j++) {
            if (i < MAX_SIZE && j < MAX_SIZE) {
                fscanf(fp, "%d", &board[i][j]);
            }
        }
    }
}

/**
 * Initialize the visible array to all zeros (all hidden).
 * rows and cols are the dimensions of the array.
 */
void initialize_visible(int visible[][MAX_SIZE], int rows, int cols) {
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            visible[i][j] = 0;
        }
    }
}

List get_neighbours(Board board, int x, int y) {
    List result;
    result.array = (int**)malloc(sizeof(int*) * 8);
    result.x = (int*)malloc(sizeof(int) * 8);
    result.y = (int*)malloc(sizeof(int) * 8);
    result.array[0] = get_ptr(board, x+1, y);
    result.array[1] = get_ptr(board, x+1, y+1);
    result.array[2] = get_ptr(board, x, y+1);
    result.array[3] = get_ptr(board, x-1, y+1);
    result.array[4] = get_ptr(board, x-1, y);
    result.array[5] = get_ptr(board, x-1, y-1);
    result.array[6] = get_ptr(board, x, y-1);
    result.array[7] = get_ptr(board, x+1, y-1);
    result.x[0] = x+1;
    result.x[1] = x+1;
    result.x[2] = x;
    result.x[3] = x-1;
    result.x[4] = x-1;
    result.x[5] = x-1;
    result.x[6] = x;
    result.x[7] = x+1;
    result.y[0] = y;
    result.y[1] = y+1;
    result.y[2] = y+1;
    result.y[3] = y+1;
    result.y[4] = y;
    result.y[5] = y-1;
    result.y[6] = y-1;
    result.y[7] = y-1;
    int next = 0;
    for (int i = 0; i < 8; i++) {
        if (result.array[i] != NULL) {
            result.array[next] = result.array[i];
            result.x[next] = result.x[i];
            result.y[next] = result.y[i];
            next += 1;
        }
    }
    result.length = next;
    return result;
}


/**
 * Caculate the number of adjacent mines for each cell.
 * Modify the board array in place:
 *   - Cells with a value of -1 remain the same
 *   - Cells with the initial value of 0 are updated with the number of
 *     adjacent cells containing mines.
 *   - Note that cells not on the boundary have 8 adjacent cells.
 * Hint: Be careful with boundary checks!
 */
void calculate_numbers(int board[][MAX_SIZE], int rows, int cols) {
    Board b = make_board(board, rows, cols);
    for (int y = 1; y <= rows; y++) {
        for (int x = 1; x <= cols; x++) {
            int* cell = get_ptr(b, x, y);
            if (*cell == -1) {
                List neighbours = get_neighbours(b, x, y);
                for (int i = 0; i < neighbours.length; i++) {
                    if (*neighbours.array[i] != -1) {
                        *neighbours.array[i] += 1;
                    }
                }
                free(neighbours.array);
            }
        }
    }
}

/**
 * Given the coordinates (row, col) of a cell that has 0 mines adjacent to it,
 * recursively reveal all connected safe cells. This will make visible all
 * of the cells connected to (row, col) that have the value 0, and their
 * adjacent numbered cells. This will reveal all of the cells that we now know
 * cannot contain a mine given that (row, col) has a value of 0.
 * See example in the handout.
 */
void flood_fill(int board[][MAX_SIZE], int visible[][MAX_SIZE],
                int rows, int cols, int row, int col) {

    // TODO: Implement this function
    Board b = make_board(board, rows, cols);
    Board vis = make_board(visible, rows, cols);
    int x = col + 1;
    int y = row + 1;
    *get_ptr(vis, x, y) = 1;
    List neighbours = get_neighbours(b, x, y);
    for (int i = 0; i < neighbours.length; i++) {
        if (*neighbours.array[i] == 0) {
            flood_fill(board, visible, rows, cols, neighbours.x[i], neighbours.y[i]);
        }
    }
}

/**
 * Set the cell at (row, col) to visible in the visible array.
 * If the revealed cell has 0 adjacent mines, then call flood_fill to
 * reveal the connected set of cells that have 0 adjacent mines and the
 * bordering safe cells.
 * If the cell was already visible, return.
 *
 * Assume row and col are valid coordinates for the arrays
 */
void reveal_cell(int board[][MAX_SIZE], int visible[][MAX_SIZE],
                 int rows, int cols, int row, int col) {
    Board b = make_board(board, rows, cols);
    Board vis = make_board(visible, rows, cols);
    int x = col + 1;
    int y = row + 1;
    if (get(vis, x, y) == 1) {
        return;
    }
    int cell = get(b, x, y);
    if (cell == 0) {
        flood_fill(board, visible, rows, cols, row, col);
    } else {
        *get_ptr(vis, x, y) = 1;
    }
}

/**
 * Print the board showing the value of the visible cells.
 *   - If a cell is visible and has the value -1, print "M" for mine
 *   - If a cell is visible and has any other value, print that value
 *   - If a cell is not visible print "."
 * NOTE: Add a space between each cell in the output. See the handout for
 * example output
 */
void print_board(int board[][MAX_SIZE], int visible[][MAX_SIZE],
                 int rows, int cols) {

    Board b = make_board(board, rows, cols);
    Board vis = make_board(visible, rows, cols);
    char result[2 * rows * (cols + 1)];
    char* write_ptr = result;
    for (int y = 1; y <= rows; y++) {
        for (int x = 1; x <= cols; x++) {
            int cell = get(b, x, y);
            int vis_cell = get(vis, x, y);
            if (vis_cell == 0) {
                strcpy(write_ptr, ".");
            }
            else {
                if (cell == -1) {
                    strcpy(write_ptr, "M");
                }
                else {
                    sprintf(write_ptr, "%d", cell);
                }
            }
            write_ptr += sizeof(char);
            if (x < cols) {
                strcpy(write_ptr, " ");
            } else {
                strcpy(write_ptr, "\n");
            }
            write_ptr += sizeof(char);
        }
    }
    printf("%s", result);
}

/**
 * Check if the game is over (a mine has been revealed).
 * Return
 *    1 if all cells without mines are visible (win),
 *   -1 if a mine is visible (loss), and
 *    0 otherwise.
 */
int check_game_over(int board[][MAX_SIZE], int visible[][MAX_SIZE],
                    int rows, int cols) {
    // TODO: Implement this function
    Board b = make_board(board, rows, cols);
    Board vis = make_board(visible, rows, cols);
    int mines = 0;
    int num_invisible = 0;
    for (int y = 1; y <= rows; y++) {
        for (int x = 1; x <= cols; x++) {
            int cell = get(b, x, y);
            int vis_cell = get(vis, x, y);
            if (vis_cell == 1) {
                if (cell == -1) {
                    return -1;
                }
            }
            else {
                num_invisible += 1;
                if (cell == -1) {
                    mines += 1;
                }
            }
        }
    }
    if (mines == num_invisible) {
        return 1;
    }
    else {
        return 0;
    }
}

/* Print a separator line the same width as a board
 */
void print_line(int cols) {
    for (int i = 0; i < cols * 2 - 1; i++) {
        printf("-");
    }
    printf("\n");
}


int main(int argc, char **argv) {
    int board[MAX_SIZE][MAX_SIZE];      // -1 for mine, 0-8 for safe cells
    int visible[MAX_SIZE][MAX_SIZE];    // 0 for hidden, 1 for visible
    int rows, cols;  // number of rows and columns in the actual board
    FILE *fp = NULL;

    if(argc == 1) {
        // Read the board from stdin
        fp = stdin;
    } else if (argc == 2) {
        // Read the board from a file named in argv[1]
        fp = fopen(argv[1], "r");
        if(fp == NULL) {
            printf("Error: could not open %s\n", argv[1]);
            exit(1);
        }
    } else {
        printf("Usage: mine_sweeper [board_ file]\n");
        exit(1);
    }

    // Initialize the board by reading it from the file pointer
    read_board(fp, board, &rows, &cols);

    // Initialize all cells as hidden
    initialize_visible(visible, rows, cols);

    // Calculate numbers for each cell (modifies board in place)
    calculate_numbers(board, rows, cols);

    // Process each move
    int game_status = check_game_over(board, visible, rows, cols);
    while (game_status == 0){
        int click_row, click_col;

    // When scanf returns EOF there is no more input
        if (scanf("%d %d", &click_row, &click_col) == EOF) {
            break;
        }

        reveal_cell(board, visible, rows, cols, click_row, click_col);

        print_board(board, visible, rows, cols);
        print_line(cols);
        game_status = check_game_over(board, visible, rows, cols);
    }

    if(game_status == 1) {
        printf("Game is won!\n");
    } else if(game_status == 0) {
        printf("Game is incomplete\n");
    } else {
        printf("Game is lost.\n");
    }

    return 0;
}
