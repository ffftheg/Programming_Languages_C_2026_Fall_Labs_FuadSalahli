#include <stdio.h>

long long factorial(int n) {
    long long result = 1;

    for (int i = 1; i <= n; i++) {
        result *= i;
    }

    return result;
}

int main() {
    int n;

    printf("Enter n: ");
    scanf("%d", &n);

    if (n < 0) {
        printf("Error: enter non-negative integer n:\n");
    } else {
        printf("Factorial of %d = %lld\n", n, factorial(n));
    }

    return 0;
}