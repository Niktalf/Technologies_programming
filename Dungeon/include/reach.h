#ifndef REACH_H
#define REACH_H

typedef struct {
    int cells;
    int stairs_reachable;
    int max_depth;
} ReachReport;

ReachReport reach_check(int from_x, int from_y);

#endif // REACH_H
