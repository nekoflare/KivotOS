//
// Created by neko on 6/15/25.
//

#include <math.h>

double __truncxfdf2(long double a) {
    double da = (double)a;
    if (da > 0.0) {
        return floorl(da);
    } else {
        return ceill(da);
    }
}

long double floorl(long double x) {
    long double truncated = (long double)(long long)x;

    if (x < 0 && truncated != x) {
        truncated -= 1.0L;
    }

    return truncated;
}

long double ceill(long double x) {
    long double truncated = (long double)(long long)x;

    if (x > 0 && truncated != x) {
        truncated += 1.0L;
    }

    return truncated;
}