#ifndef TENGINE_RUNTIME_H
#define TENGINE_RUNTIME_H

#ifdef __cplusplus
extern "C" {
#endif

void te_runtime_print_number(double value);
void te_runtime_print_string(const char* value);
void te_runtime_print_boolean(int value);

#ifdef __cplusplus
}
#endif

#endif
