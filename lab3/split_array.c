#include <stdio.h>
#include <stdlib.h>

/* Return a pointer to an array of two dynamically allocated arrays of ints.
   The first array contains the elements of the input array s that are
   at even indices.  The second array contains the elements of the input
   array s that are at odd indices.

   Do not allocate any more memory than necessary. You are not permitted
   to include math.h.  You can do the math with modulo arithmetic and integer
   division.
*/
int **split_array(const int *s, int length) {
    int* a1;
    int* a2;
    if (length % 2 == 0) {
        a1 = (int*)malloc(sizeof(int) * (length / 2));
        a2 = (int*)malloc(sizeof(int) * (length / 2));
    } else {
        a1 = (int*)malloc(sizeof(int) * ((length + 1) / 2));
        a2 = (int*)malloc(sizeof(int) * ((length - 1) / 2));
    }
    for (int i = 0; i < length; i++) {
        if (i % 2 == 0) {
            a1[i/2] = s[i];
        } else {
            a2[(i - 1) / 2] = s[i];
        }
    }
    int** res = (int**)malloc(sizeof(int*) * 2);
    res[0] = a1;
    res[1] = a2;
    return res;
}

/* Return a pointer to an array of ints with size elements.
   - strs is an array of strings where each element is the string
     representation of an integer.
   - size is the size of the array
 */

int *build_array(char **strs, int size) {
    int* res = (int*)malloc(size * sizeof(int));
    for (int i = 0; i < size; i++) {
        int num = strtol(strs[i], NULL, 10);
        res[i] = num;
    }
    return res;
}


int main(int argc, char **argv) {
    /* Replace the comments in the next two lines with the appropriate
       arguments.  Do not add any additional lines of code to the main
       function or make other changes.
     */
    int *full_array = build_array(argv + 1, argc - 1);
    int **result = split_array(full_array, argc - 1);

    printf("Original array:\n");
    for (int i = 0; i < argc - 1; i++) {
        printf("%d ", full_array[i]);
    }
    printf("\n");

    printf("result[0]:\n");
    for (int i = 0; i < argc / 2; i++) {
        printf("%d ", result[0][i]);
    }
    printf("\n");

    printf("result[1]:\n");
    for (int i = 0; i < (argc - 1) / 2; i++) {
        printf("%d ", result[1][i]);
    }
    printf("\n");
    free(full_array);
    free(result[0]);
    free(result[1]);
    free(result);
    return 0;
}
