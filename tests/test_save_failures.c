/* Test-only I/O fault injection; the production executable has no fault switches. */
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { NORMAL, WRITE_FAILURE, FLUSH_FAILURE, CLOSE_FAILURE };
static int fault;
static FILE *temporary_stream;

static FILE *fault_fopen(const char *path, const char *mode)
{
    FILE *stream = fopen(path, mode);
    if (strcmp(mode, "wx") == 0) temporary_stream = stream;
    return stream;
}

static int fault_fprintf(FILE *stream, const char *format, ...)
{
    int result;
    va_list args;
    if (stream == temporary_stream && fault == WRITE_FAILURE) {
        errno = ENOSPC;
        return -1;
    }
    va_start(args, format);
    result = vfprintf(stream, format, args);
    va_end(args);
    return result;
}

static int fault_fflush(FILE *stream)
{
    if (stream == temporary_stream && fault == FLUSH_FAILURE) {
        errno = ENOSPC;
        return EOF;
    }
    return fflush(stream);
}

static int fault_fclose(FILE *stream)
{
    int fail = stream == temporary_stream && fault == CLOSE_FAILURE;
    int result = fclose(stream);
    if (fail) { errno = EIO; return EOF; }
    return result;
}

#define fopen fault_fopen
#define fprintf fault_fprintf
#define fflush fault_fflush
#define fclose fault_fclose
#include "../src/tree_manager.c"
#undef fopen
#undef fprintf
#undef fflush
#undef fclose

static void require(int condition, const char *label)
{
    if (!condition) { fprintf(stderr, "FAIL: %s\n", label); exit(EXIT_FAILURE); }
}

int main(void)
{
    const char *target = "save-fault-test.csv";
    const char *temp = "save-fault-test.csv.tmp";
    const char *original = "1,Oak,Gate,20,1\n";
    const char *names[] = {"normal", "fprintf", "fflush", "fclose"};
    Tree updated = {1, "Birch", "Gate", 25.0f, 2};
    char contents[128];
    FILE *stream;
    int mode;
    for (mode = WRITE_FAILURE; mode <= CLOSE_FAILURE; ++mode) {
        stream = fopen(target, "w");
        require(stream != NULL, "create fixture");
        require(fputs(original, stream) >= 0, "write fixture");
        require(fclose(stream) == 0, "close fixture");
        fault = mode;
        require(save_to_file(&updated, 1, target) == 0, "failure propagated");
        temporary_stream = NULL;
        stream = fopen(target, "r");
        require(stream != NULL, "original remains readable");
        require(fgets(contents, sizeof contents, stream) != NULL, "read original");
        require(strcmp(contents, original) == 0 && fgetc(stream) == EOF, "original bytes retained");
        require(fclose(stream) == 0, "close original");
        stream = fopen(temp, "r");
        require(stream == NULL && errno == ENOENT, "owned temporary removed");
        printf("PASS: %s failure preserves original and removes temporary\n", names[mode]);
    }
    fault = NORMAL;
    require(save_to_file(&updated, 1, target) == 1, "normal save after failures");
    stream = fopen(target, "r");
    require(stream != NULL && fgets(contents, sizeof contents, stream) != NULL, "read updated");
    require(strcmp(contents, "1,Birch,Gate,25,2\n") == 0, "new contents after recovery");
    require(fclose(stream) == 0, "close updated");
    require(remove(target) == 0, "cleanup fixture");
    puts("PASS: normal save after injected failures");
    puts("All 4 safe-save fault checks passed.");
    return EXIT_SUCCESS;
}
