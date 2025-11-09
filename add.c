#include <stdio.h>

int global_var = 42;
static int local_static = 7;
const char message[] = "Hello, ELF World!\n";

int add(int a, int b) {
    return a + b;
}

int main() {
    printf("%s", message);
    printf("Result = %d\n", add(global_var, local_static));
    return 0;
}