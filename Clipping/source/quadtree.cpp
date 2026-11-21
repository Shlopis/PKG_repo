#include "quadtree.h"

void QuadTree::clear() {
    nodes.clear();
    segments = nullptr;
}

int QuadTree::createNode(const Rect& region, int depth) {
    Node node;
    node.region = region;
    node.depth = depth;
    nodes.push_back(node);
    return nodes.size() - 1;
}

int QuadTree::childContaining(const Node& node, const Rect& aabb) const { // в какой квадрат попадает отрезок
    const double midX = (node.region.xmin + node.region.xmax) * 0.5;
    const double midY = (node.region.ymin + node.region.ymax) * 0.5;
    const bool left = Eps::le(aabb.xmax, midX);
    const bool right = Eps::ge(aabb.xmin, midX);
    const bool below = Eps::le(aabb.ymax, midY);
    const bool above = Eps::ge(aabb.ymin, midY);
    if (left && below) return 0;
    if (right && below) return 1;
    if (left && above) return 2;
    if (right && above) return 3;
    return -1;
}

void QuadTree::splitNode(int index) {
    const Rect region = nodes[index].region;
    const double midX = (region.xmin + region.xmax) * 0.5;
    const double midY = (region.ymin + region.ymax) * 0.5;
    const int childDepth = nodes[index].depth + 1;
    const Rect childRegions[4] = {
        { region.xmin, region.ymin, midX, midY },
        { midX, region.ymin, region.xmax, midY },
        { region.xmin, midY, midX, region.ymax },
        { midX, midY, region.xmax, region.ymax }
    };
    for (int i = 0; i < 4; ++i)
        nodes[index].children[i] = createNode(childRegions[i], childDepth);
    nodes[index].leaf = false;

    const QVector<int> oldIds = nodes[index].ids;
    nodes[index].ids.clear();
    for (int segmentId : oldIds) {
        const Rect aabb = segAABB((*segments)[segmentId]);
        const int child = childContaining(nodes[index], aabb);
        if (child >= 0)
            insertSegment(nodes[index].children[child], segmentId, aabb);
        else
            nodes[index].ids.push_back(segmentId);
    }
}

void QuadTree::insertSegment(int index, int segmentId, const Rect& aabb) {
    Node& node = nodes[index];
    if (!node.leaf) {
        const int child = childContaining(node, aabb);
        if (child >= 0) {
            insertSegment(node.children[child], segmentId, aabb);
            return;
        }
        node.ids.push_back(segmentId);
        return;
    }
    if (node.ids.size() < itemLimit || node.depth >= depthLimit) {
        node.ids.push_back(segmentId);
        return;
    }
    splitNode(index);
    const int child = childContaining(nodes[index], aabb);
    if (child >= 0)
        insertSegment(nodes[index].children[child], segmentId, aabb);
    else
        nodes[index].ids.push_back(segmentId);
}

void QuadTree::build(const QVector<Segment>& inputSegments, const Rect& domain,  int maxDepth, int maxItems) {
    clear();
    segments = &inputSegments;
    depthLimit = maxDepth;
    itemLimit = maxItems;
    const int root = createNode(domain, 0);
    for (int i = 0; i < inputSegments.size(); ++i) {
        const Rect aabb = segAABB(inputSegments[i]);
        if (rectIntersect(domain, aabb))
            insertSegment(root, i, aabb);
    }
}

QVector<int> QuadTree::query(const Rect& rect) {
    QVector<int> result;
    if (!segments || nodes.isEmpty()) return result;
    dedupMask.fill(0, segments->size());
    queryNode(0, rect, result);
    return result;
}

void QuadTree::queryNode(int index, const Rect& rect, QVector<int>& out) {
    const Node& node = nodes[index];
    if (!rectIntersect(node.region, rect)) return;
    for (int segmentId : node.ids)
        if (!dedupMask[segmentId]) {
            dedupMask[segmentId] = 1;
            out.push_back(segmentId);
        }
    if (!node.leaf)
        for (int child : node.children)
            queryNode(child, rect, out);
}

QVector<Rect> QuadTree::nodeRects() const {
    QVector<Rect> result;
    if (!nodes.isEmpty()) collectRects(0, result);
    return result;
}

void QuadTree::collectRects(int index, QVector<Rect>& out) const {
    const Node& node = nodes[index];
    if (node.depth <= 5 || (node.leaf && !node.ids.isEmpty())) {
        out.push_back(node.region);
    }
    if (!node.leaf)
        for (int child : node.children) collectRects(child, out);
}