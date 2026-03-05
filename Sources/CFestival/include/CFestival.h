#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef enum CFestivalSolveStatus {
    CFESTIVAL_STATUS_SOLVED = 0,
    CFESTIVAL_STATUS_UNSOLVED = 1,
    CFESTIVAL_STATUS_ERROR = 2,
} CFestivalSolveStatus;

typedef struct CFestivalSolveResult {
    int status;
    int minimum_moves;
    int minimum_pushes;
    char *lurd;
    char *message;
} CFestivalSolveResult;

int cfestival_solve_level(
    const char *level_text,
    int time_limit_seconds,
    int cores,
    CFestivalSolveResult *out_result
);

void cfestival_free_result(CFestivalSolveResult *result);

#ifdef __cplusplus
}
#endif
