#ifndef RASTERENGINE_H
#define RASTERENGINE_H

class IRasterTarget {
public:
    virtual ~IRasterTarget() = default;
    virtual void SetPixel(int x, int y, double intensity) = 0;
                                                 //0-1
};

namespace RasterEngine {
void lineStep(int x0, int y0, int x1, int y1, IRasterTarget& target);
void lineDDA (int x0, int y0, int x1, int y1, IRasterTarget& target);
void lineBresenham(int x0, int y0, int x1, int y1, IRasterTarget& target);
void circleBresenham(int r, IRasterTarget& target);
void lineCastlePitwey(int x0, int y0, int x1, int y1, IRasterTarget& target);
void lineWu(int x0, int y0, int x1, int y1, IRasterTarget& target);
void circleWu(int r, IRasterTarget& target);
}

#endif // RASTERENGINE_H
