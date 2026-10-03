#include <string.h>

#include "map.h"
#include "reach.h"

static unsigned char visited[MAP_HEIGHT][MAP_WIDTH];

static void fill(const int x, const int y, const int depth, ReachReport *report)
{
    if (!map_walkable(x, y)) {
        return;
    }
    if (visited[y][x]) {
        return;
    }

    visited[y][x] = 1;
    report->cells++;

    if (depth > report->max_depth) {
        report->max_depth = depth;
    }
    if (map_at(x, y) == TILE_STAIRS) {
        report->stairs_reachable = 1;
    }

    fill(x + 1, y, depth + 1, report);
    fill(x - 1, y, depth + 1, report);
    fill(x, y + 1, depth + 1, report);
    fill(x, y - 1, depth + 1, report);
}

ReachReport reach_check(const int from_x, const int from_y)
{
    ReachReport report;

    report.cells = 0;
    report.stairs_reachable = 0;
    report.max_depth = 0;

    memset(visited, 0, sizeof visited);
    fill(from_x, from_y, 1, &report);
    return report;
}
