#include <stdio.h>
#include <string.h>
#include <stdbool.h>

typedef struct Coordinates {
    int x;
    int y;
    int z;
} coordinates_t;

// int main() {
//     printf("Hello World\n");
//     int arr[5] = {1,2,3,4,5};
//     int val = arr[1];
//     printf("size of int = %zu\n", sizeof(int));
//     printf("size of array = %zu\n", sizeof(arr));
//     printf("size of a bool = %zu\n", sizeof(bool));
//     printf("size of a string 'word' = %zu\n", sizeof("word"));
//     printf("size of size_t on this machine? = %zu\n", sizeof(size_t));
//     printf("length of a str 'word' = %zu\n", strlen("word"));
//     return 0;
// }

// only works with null terminated arrays ...
size_t char_arr_len(char **arr) {
    size_t count = 0;
    while(1) {
        char *v = arr[count];
        if (v == NULL) {
            return count;
        }
        count++;
    }
}

int main(int argc, char **argv) {
    printf("number of args = %d. size of argv in bytes = %zu\n", argc, sizeof(argv));
    // size_t count = char_arr_len(argv);
    // printf("runtime calculated count of argv, not bytes = %zu\n", count);
    for (size_t i = 0; i < argc; i++) {
        char *v = argv[i];
        if (v == NULL) {

        }
        printf("arg %zu, %s\n", i, argv[i]);
    }
    return 0;
}
