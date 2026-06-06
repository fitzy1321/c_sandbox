#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "lib.h"

// int main(int argc, char *argv) {
//     printf("\nYo, I heard you like size_t, dawg ...\n");
//     int arr[5] = {1,2,3,4,5};
//     int val = arr[1];
//     printf("size of int = %zu\n", sizeof(int));
//     printf("size of array = %zu\n", sizeof(arr));
//     printf("size of a bool = %zu\n", sizeof(bool));
//     printf("size of a string 'word' = %zu\n", sizeof("word"));
//     printf("size of size_t on this machine? = %zu\n", sizeof(size_t));
//     printf("length of a str 'word' = %zu\n", strlen("word"));
//     printf("end size stuff\n\n");
//     printf("number of args = %d. size of argv in bytes = %zu\n", argc, sizeof(argv));
//     // size_t count = char_arr_len(argv);
//     // printf("runtime calculated count of argv, not bytes = %zu\n", count);
//     for (size_t i = 0; i < argc; i++) {
//         char *v = argv[i];
//         if (v == NULL) {

//         }
//         printf("arg # %zu, value = '%s'\n", i, argv[i]);
//     }
//     return 0;
// }

#ifdef _WIN32
    #include <direct.h>
    #define m_getcwd _getcwd
#else
    #include <unistd.h>
    #define m_getcwd getcwd
#endif

// example of using a scoped function marco.
// Do while loop has block scope and expands to valid single line expressions/statements.
#define something(x)    \
    do {                \
        int y = (x);    \
    } while(0)

void print_cwd() {
    char buff[FILENAME_MAX];
    if (m_getcwd(buff, FILENAME_MAX) != NULL) {
        printf("\nC execution Current working directory: %s\n", buff);
    } else {
        perror("Error getting cwd");
    }
}

int main(void) {
    printf("Messing with pointers and memory!!!\n");
    Point *p = (Point *)malloc(sizeof(Point));
    if (p == NULL) {
        printf("Memory allocation failed! Ending program ...");
        return 1;
    }
    p->x = 69;
    p->y = 42;
    point_dosomething(p);
    printf("updated point.x: %d. updated point.y: %d\n", p->x, p->y);
    free(p);

    // Example of zeroing a password, with generic void *ptr
    const size_t len = 24;
    char *pass = malloc(len);
    if (pass) {
        strcpy(pass, "secret!234");
        printf("Password copied: %s\n", pass);
        printf("Password size: %zu\n", sizeof(pass));
        secure_zero_memory(pass, len);
        printf("Password afer zero'd: %s\n", pass);
        free(pass);
    }
    return 0;
}
