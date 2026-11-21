#ifndef QUADTREE_H
#define QUADTREE_H

#include "geom.h"

//Вся плоскость представляется в виде одного большого квадрата (корень).
//Если в этом квадрате слишком много объектов или данных, он делится на 4 равные части

class QuadTree {
public:
    void build(const QVector<Segment>& inputSegments, const Rect& domain, int maxDepth = 10, int maxItems = 16);
    QVector<int> query(const Rect& rect);
    QVector<Rect> nodeRects() const;
    int nodeCount() const { return nodes.size(); }
    void clear();

private:
    struct Node {
        Rect region;
        QVector<int> ids;
        int children[4] = { -1, -1, -1, -1 };
        int depth = 0;
        bool leaf = true;
    };

    QVector<Node> nodes;
    QVector<char> dedupMask;
    const QVector<Segment>* segments = nullptr;
    int depthLimit = 10;
    int itemLimit = 16;

    int createNode(const Rect& region, int depth);
    int childContaining(const Node& node, const Rect& aabb) const;
    void splitNode(int index);
    void insertSegment(int index, int segmentId, const Rect& aabb);
    void queryNode(int index, const Rect& rect, QVector<int>& out);
    void collectRects(int index, QVector<Rect>& out) const;
};

#endif // QUADTREE_H
