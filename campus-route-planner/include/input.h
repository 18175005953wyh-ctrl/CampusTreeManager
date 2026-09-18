#ifndef INPUT_H
#define INPUT_H

#include <stdio.h>

/* 1 = complete line, 0 = EOF/read error, -1 = line too long (discarded). */
int read_line(FILE *stream, char *buffer, int capacity);
char *trim_text(char *text);
int parse_integer(const char *text, int minimum, int maximum, int *value);
int read_integer(const char *prompt, int minimum, int maximum, int *value);
int read_keyword(const char *prompt, char *buffer, int capacity);

#endif
