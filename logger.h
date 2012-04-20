#ifndef __LOGGER_H
#define __LOGGER_H

#include <stdio.h>
#include <stdlib.h>
#include <execinfo.h>

#define TRACE(f, args...) do { \
    /*logger("trace", __FILE__, __LINE__, f, ##args);*/ } while(0);

#define DEBUG(f, args...) do { \
    logger("debug", __FILE__, __LINE__, f, ##args); } while(0);

#define WARN(f, args...) do { \
    logger("warning", __FILE__, __LINE__, f, ##args); } while(0);

#define ERROR(f, args...) do { \
    logger("error", __FILE__, __LINE__, f, ##args); } while(0);

#define PANIC(f, args...) do { \
    logger("panic", __FILE__, __LINE__, f, ##args); \
    BACKTRACE; exit(EXIT_FAILURE); } while(0);

#define BACKTRACE_MAX 100

#define BACKTRACE do { \
    void *buffer[BACKTRACE_MAX]; \
    int j, nptrs = backtrace(buffer, BACKTRACE_MAX); \
    printf("backtrace() returned %d addresses\n", nptrs); \
    char **strings = backtrace_symbols(buffer, nptrs); \
    if(strings == NULL) { perror("backtrace_symbols"); exit(EXIT_FAILURE); } \
    for (j = 0; j < nptrs; j++) printf("%s\n", strings[j]); \
    free(strings); } while(0);

int logger(const char *, const char *, const int, const char *, ...);

#endif
