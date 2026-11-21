#ifndef COHENSUTHERLAND_H
#define COHENSUTHERLAND_H

#include "geom.h"

enum OutCode{
    OC_TOP = 1,     //0001
    OC_BOTTOM = 2, //0010
    OC_RIGHT = 4, //0100
    OC_LEFT = 8  //1000
};

unsigned outcode(const Point& point, const Rect& window);

struct ClipResult {
    bool visible = false;
    Segment segment;
};

ClipResult clipSegmentCS(const Segment& segment, const Rect& window);

#endif // COHENSUTHERLAND_H