#include <stdio.h>
#include <stdbool.h>

bool j[10];

int main() {
    bool i = true;
    j[1] = true;
    printf("sizeof(j) = %ld\n", sizeof(j));
    printf("sizeof(i) = %ld\n", sizeof(i)); 
}