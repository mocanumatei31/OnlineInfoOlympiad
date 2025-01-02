#include <stdio.h>
#include <stdlib.h>

int main() {
    int num;
    int sum = 0;
    while(scanf("%d", &num) != EOF) {
        sum += num;
    }
    if(sum > 100) {
        sum /= 2;
    }
    printf("%d\n", sum);
    return 0;
}