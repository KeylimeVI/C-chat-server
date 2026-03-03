#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "word_list.h"


/* Starting with words (returned by read_words), generate and return
   a new word list with only those words of length len. 

   Allocate exactly enough memory to store only those words of length len.

   Note: Do not make copies of the words.
*/
char **prune_word_list(char **word_list, int len) {
    // TODO: complete this function, replacing the return statement

    return NULL;
}


/* Free memory acquired by prune_word_list.
*/
void deallocate_pruned_word_list(char **word_list) {
    free(word_list);
}
