#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "reading.h"

/* Read all words from filename and return them in an array of strings. */
char **read_words(char *filename) {
    char buffer[MAX_WORD_LENGTH + 1];
    FILE *fp;
    int word_count;
    char **words;

    fp = fopen(filename, "r");
    if (!fp) {
      perror("fopen");
      exit(1);
    }

    word_count = 0;
    // Allocate space for the array of pointers
    words = malloc(MAX_WORDS * sizeof(*words));
    if (words == NULL) {
        perror("malloc");
        exit(1);
    }

    // Get and store all words
    while (fgets(buffer, MAX_WORD_LENGTH, fp)) {
        if(buffer[strlen(buffer) - 1] == '\n') {
            buffer[strlen(buffer) - 1] = '\0'; // Delete newline
        } else {
            // Ensure buffer is null-terminated
            buffer[MAX_WORD_LENGTH - 1] = '\0';
        }

        // Allocate space for each word
        words[word_count] = malloc(strlen(buffer) + 1);
        if (words[word_count] == NULL) {
            perror("malloc");
            exit(1);
        }

        // strcpy is safe because we just allocated exactly enough space. 
        strcpy(words[word_count], buffer);
        word_count++;
    }
    words[word_count] = NULL;

    fclose(fp);
    return words;
}

/* Deallocate all memory acquired by read_words. */
void deallocate_words(char **words) {
    char **p = words;
    while(*p) {
        free(*p);
        p++;
    }
    free(words);
}
