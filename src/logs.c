#define _POSIX_C_SOURCE 200809L
#include "logs.h"
#include <stdio.h>
#include <time.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <string.h>

static void ensure_logs_dir(const char *path) {
    (void)path;
    struct stat st = {0};
    if (stat("logs", &st) == -1) {
        mkdir("logs", 0755);
    }
}

int log_event(const char *logfile, int userId, const char *event, const char *desc) {
    if (!logfile || logfile[0] == 0) return -1;
    ensure_logs_dir("logs");
    char path[1024];
    if (strchr(logfile, '/') || strchr(logfile, '\\')) strncpy(path, logfile, sizeof(path)-1);
    else snprintf(path, sizeof(path), "logs/%s", logfile);
    FILE *f = fopen(path, "a");
    if (!f) return -1;
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    char timestr[64];
    snprintf(timestr, sizeof(timestr), "%04d-%02d-%02d %02d:%02d:%02d",
             tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
             tm.tm_hour, tm.tm_min, tm.tm_sec);
    if (desc)
        fprintf(f, "[%s] (user:%d) EVENT=%s DESC=\"%s\"\n", timestr, userId, event, desc);
    else
        fprintf(f, "[%s] (user:%d) EVENT=%s\n", timestr, userId, event);
    fclose(f);
    return 0;
}
