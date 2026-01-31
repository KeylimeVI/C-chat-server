#include <stdio.h>
// Do not add any other header files

/* basename_inplace returns the file-name component of path. It is similar to
 * the library function described in the man pages on teach.cs (man 3 basename)
 * The return value of this function will be exactly the same as running the
 * system program "basename" with no additional arguments on the commandline 
 * on teach.cs, so you have a convenient way to test it
 *  - If path is "/" then return "/".
 *  - Return the component of path following the final '/'. Trailing '/' 
 *    characters are not counted as part of the pathname.
 *  - If path does not contain a slash, return path.
 *  - If path is a null pointer or points to an empty string, then 
 *    return ".".
 * 
 * Write this function without declaring any character arrays, or 
 * dynamically allocating memory. You may need to modify path.  
 * Do not use any string functions from string.h, not even strlen()
 *
 * Remember to run this program as "./basename" since there is a system
 * level program with the same name.
 */




int main(int argc, char **argv) {

    if(argc != 2) {
        fprintf(stderr, "Usage: basename <path>\n");
        return 1;
    }

    printf("%s\n", basename_inplace(argv[1]));

    return 0;
}
