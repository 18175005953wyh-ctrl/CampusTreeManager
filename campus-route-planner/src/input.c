#include "input.h"
#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

int read_line(FILE *stream, char *buffer, int capacity)
{
    size_t length;
    int ch;
    if (capacity < 2 || fgets(buffer, capacity, stream) == NULL) return 0;
    length = strlen(buffer);
    if (length > 0 && buffer[length - 1] == '\n') {
        buffer[--length] = '\0';
    } else {
        ch = fgetc(stream);
        if (ch != '\n' && ch != EOF) {
            while ((ch = fgetc(stream)) != '\n' && ch != EOF) { }
            return -1;
        }
    }
    if (length > 0 && buffer[length - 1] == '\r') buffer[length - 1] = '\0';
    return 1;
}

char *trim_text(char *text)
{
    char *end;
    while (isspace((unsigned char)*text)) ++text;
    end = text + strlen(text);
    while (end > text && isspace((unsigned char)end[-1])) --end;
    *end = '\0';
    return text;
}

int parse_integer(const char *text, int minimum, int maximum, int *value)
{
    char *end;
    long number;
    errno = 0;
    number = strtol(text, &end, 10);
    if (end == text || errno == ERANGE) return 0;
    while (isspace((unsigned char)*end)) ++end;
    if (*end != '\0' || number < minimum || number > maximum) return 0;
    *value = (int)number;
    return 1;
}

int read_integer(const char *prompt, int minimum, int maximum, int *value)
{
    char buffer[256];
    int status;
    for (;;) {
        fputs(prompt, stdout);
        fflush(stdout);
        status = read_line(stdin, buffer, (int)sizeof buffer);
        if (status == 0) return 0;
        if (status == 1 && parse_integer(buffer, minimum, maximum, value)) return 1;
        printf("Invalid input. Enter an integer from %d to %d.\n", minimum, maximum);
    }
}

int read_keyword(const char *prompt, char *buffer, int capacity)
{
    int status;
    char *value;
    for (;;) {
        fputs(prompt, stdout);
        fflush(stdout);
        status = read_line(stdin, buffer, capacity);
        if (status == 0) return 0;
        if (status == 1) {
            value = trim_text(buffer);
            if (*value != '\0') {
                memmove(buffer, value, strlen(value) + 1);
                return 1;
            }
        }
        printf("Invalid keyword. Enter 1-%d bytes of nonempty text.\n", capacity - 1);
    }
}
