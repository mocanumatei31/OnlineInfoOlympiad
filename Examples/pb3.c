#include <stdio.h>
#include <stdlib.h>

int main() {
    int num;
    int mn = 1e9;
    while(scanf("%d", &num) != EOF) {
        if(num < mn) mn = num;
    }
    printf("%d\n", mn);
    return 0;
}