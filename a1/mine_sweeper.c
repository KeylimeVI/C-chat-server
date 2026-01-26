#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_SIZE 20

// Function prototypes 
void read_board(FILE *fp, int board[][MAX_SIZE], int *rows, int *cols);
void initialize_visible(int visible[][MAX_SIZE], int rows, int cols);
void calculate_numbers(int board[][MAX_SIZE], int rows, int cols);
void reveal_cell(int board[][MAX_SIZE], int visible[][MAX_SIZE], 
                 int rows, int cols, int row, int col);
void flood_fill(int board[][MAX_SIZE], int visible[][MAX_SIZE],
                int rows, int cols, int row, int col);
void print_board(int board[][MAX_SIZE], int visible[][MAX_SIZE], 
                 int rows, int cols);
int check_game_over(int board[][MAX_SIZE], int visible[][MAX_SIZE], 
                    int rows, int cols);
void print_line(int rows);

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

/* Print a separator line the same width as a board
 */
void print_line(int cols) {
    for (int i = 0; i < cols * 2 - 1; i++) {
        printf("-");
    }
    printf("\n");
}

