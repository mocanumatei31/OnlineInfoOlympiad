#include <stdio.h>
#include <stdlib.h>

int main() {
    int num;
    int mn = 1e9;
    while(scanf("%d", &num) != EOF) {
        if(num < mn) mn = num;
    }
    if(mn % 2 == 0) mn /= 2;
    printf("%d\n", mn);
    return 0;
}