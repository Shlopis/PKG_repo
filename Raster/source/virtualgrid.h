#ifndef VIRTUALGRID_H
#define VIRTUALGRID_H

#include "rasterengine.h"
#include <vector>

class VirtualGrid : public IRasterTarget {
public:
    VirtualGrid(int minX, int minY, int w, int h);

    void SetPixel(int x, int y, double intensity) override;
    void clear();
    bool contains(int x, int y) const;
    int at(int x, int y) const;

    int minX() const { return min_x; }
    int minY() const { return min_y; }
    int maxX() const { return min_x + w - 1; }
    int maxY() const { return min_y + h - 1; }
    int width() const { return w; }
    int height() const { return h; }

private:
    int min_x, min_y, w, h;
    std::vector<std::vector<int>> data;
};

#endif // VIRTUALGRID_H
