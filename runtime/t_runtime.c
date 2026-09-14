#include "t_runtime.h"

#include <stdio.h>

void te_runtime_print_number(double value) {
    printf("%g\n", value);
}

void te_runtime_print_string(const char* value) {
    printf("%s\n", value);
}

void te_runtime_print_boolean(int value) {
    puts(value ? "true" : "false");
}
