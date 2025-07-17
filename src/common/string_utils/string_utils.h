#pragma once

#include <stddef.h>
#include <stdbool.h>

// String utilities with null-safety
char* str_copy(const char* src);
char* str_copy_n(const char* src, size_t n);
void str_free(char* str);

// String manipulation
int str_length(const char* str);
int str_compare(const char* a, const char* b);
int str_compare_n(const char* a, const char* b, size_t n);
bool str_equals(const char* a, const char* b);
bool str_starts_with(const char* str, const char* prefix);
bool str_ends_with(const char* str, const char* suffix);

// String search
char* str_find(const char* haystack, const char* needle);
char* str_find_last(const char* haystack, const char* needle);

// String building
typedef struct {
    char* buffer;
    size_t size;
    size_t capacity;
} str_builder_t;

int str_builder_init(str_builder_t* builder, size_t initial_capacity);
int str_builder_append(str_builder_t* builder, const char* str);
int str_builder_append_format(str_builder_t* builder, const char* format, ...);
char* str_builder_finalize(str_builder_t* builder);
void str_builder_destroy(str_builder_t* builder);

// String trimming
char* str_trim(char* str);
char* str_trim_left(char* str);
char* str_trim_right(char* str);
