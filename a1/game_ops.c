#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_SIZE 20

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
    // TODO: Read rows and cols from first line
    // TODO: Read the board values into the 2D array board
}

/**
 * Initialize the visible array to all zeros (all hidden).
 * rows and cols are the dimensions of the array.
 */
void initialize_visible(int visible[][MAX_SIZE], int rows, int cols) {
    // TODO: Implement this function.
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
    // TODO: Implement this function.
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
    // TODO: Implement this function.
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
    // TODO: Implement this function
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
    return 0; // replace the return value when implemented.
}
