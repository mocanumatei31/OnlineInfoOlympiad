#include <stdio.h>
#include <stdlib.h>

int main() {
    int num;
    int prod = 1;
    while(scanf("%d", &num) != EOF) {
        prod *= num;
    }
    printf("%d\n", prod);
    return 0;
}