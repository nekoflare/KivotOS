//
// Created by neko on 6/7/25.
//

#ifndef LOG_H
#define LOG_H

void debug_log(const char *s);
void debug_print(const char* fmt, ...) __attribute__((format(printf, 1, 2)));

#endif //LOG_H
