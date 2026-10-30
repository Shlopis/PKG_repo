#include "virtualgrid.h"
#include <algorithm>

VirtualGrid::VirtualGrid(int minX, int minY, int wi, int he)
    : min_x(minX), min_y(minY), w(wi), h(he), data(size_t(h),std::vector<int>(size_t(w), 0)) {}

bool VirtualGrid::contains(int x, int y) const {
    return x >= min_x && x < min_x + w && y >= min_y && y < min_y + h;
}

void VirtualGrid::SetPixel(int x, int y, double intensity) {
    if (!contains(x, y))
        return;
    int value = int(std::clamp(intensity, 0.0, 1.0) * 255.0 + 0.5);
    int& cell = data[size_t(y - min_y)][size_t(x - min_x)];
    cell = std::max(cell, value);
}

void VirtualGrid::clear() {
    for (auto& row : data)
        std::fill(row.begin(), row.end(), 0);
}

int VirtualGrid::at(int x, int y) const {
    if (!contains(x, y))
        return 0;
    return data[size_t(y - min_y)][size_t(x - min_x)];
}
