#pragma once

#include <algorithm>
#include <cstdlib>

#include "../vendor/supermarket-engine/engine/edit.h"

// grahamScan
// https://www.geeksforgeeks.org/dynamic-convex-hull-adding-points-existing-convex-hull/?ref=rp

// testing points
// https://www.nayuki.io/page/convex-hull-algorithm

struct Polygon {
    std::vector<glm::vec2> points;
    std::vector<glm::vec2> hull;

    // checks whether the point crosses the convex hull
    // or not
    int orientation(glm::vec2 a, glm::vec2 b, glm::vec2 c) const {
        int res = (b.y - a.y) * (c.x - b.x) - (c.y - b.y) * (b.x - a.x);
        if (res == 0) return 0;
        if (res > 0) return 1;
        return -1;
    }

    // Returns the square of distance between two input points
    int sqDist(glm::vec2 p1, glm::vec2 p2) const {
        return (p1.x - p2.x) * (p1.x - p2.x) + (p1.y - p2.y) * (p1.y - p2.y);
    }

    glm::vec2 centroid() const {
        glm::vec2 mid = {0, 0};
        for (size_t i = 0; i < hull.size(); i++) {
            mid.x += hull[i].x;
            mid.y += hull[i].y;
        }
        return mid;
    }

    float maxradius() const {
        glm::vec2 mid = centroid();
        float maxdst = 0.f;

        for (size_t i = 0; i < hull.size(); i++) {
            float dst = sqDist(mid, hull[i]);
            maxdst = fmax(maxdst, dst);
        }

        return sqrt(maxdst);
    }

    // Checks whether the point is inside the convex hull or not
    bool inside(glm::vec2 p) const {
        // Initialize the centroid of the convex hull
        glm::vec2 mid = {0, 0};

        if (hull.size() < 3) return false;

        std::vector<glm::vec2> hullcopy(hull);

        int n = hullcopy.size();

        // Multiplying with n to avoid floating point
        // arithmetic.
        p.x *= n;
        p.y *= n;
        for (int i = 0; i < n; i++) {
            mid.x += hullcopy[i].x;
            mid.y += hullcopy[i].y;
            hullcopy[i].x *= n;
            hullcopy[i].y *= n;
        }

        // if the mid and the given point lies always
        // on the same side w.r.t every edge of the
        // convex hullcopy, then the point lies inside
        // the convex hullcopy
        for (int i = 0, j; i < n; i++) {
            j = (i + 1) % n;
            int x1 = hullcopy[i].x, x2 = hullcopy[j].x;
            int y1 = hullcopy[i].y, y2 = hullcopy[j].y;
            int a1 = y1 - y2;
            int b1 = x2 - x1;
            int c1 = x1 * y2 - y1 * x2;
            int for_mid = a1 * mid.x + b1 * mid.y + c1;
            int for_p = a1 * p.x + b1 * p.y + c1;
            if (for_mid * for_p < 0) return false;
        }

        return true;
    }

    // TODO split work into add / compute
    // so that we can add 4 points at a time (at least)

    // Adds a point p to given convex hull a[]
    void add(glm::vec2 p) {
        points.push_back(p);
        if (hull.size() <= 3) {
            hull.push_back(p);
            return;
        }

        andrew_chain();
        // grahamScan(p);
        return;
    }

    float cross(glm::vec2 o, glm::vec2 a, glm::vec2 b) const {
        return (a.x - o.x) * (b.y - o.y) - (a.y - o.y) * (b.x - o.x);
    }

    struct CompareVec {
        bool operator()(const glm::vec2& a, const glm::vec2& b) const {
            return a.x < b.x || (a.x == b.x && a.y < b.y);
        }
    };

    void remove(glm::vec2 p) {
        auto hasp = std::find(points.begin(), points.end(), p);
        if (hasp == points.end()) {
            if (inside(p)) {
                log_warn(
                    "Trying to remove a point that is inside the nav mesh");
            } else {
                // log_warn(
                // "Trying to remove a point that isnt in the original nav "
                // "set mesh");
            }
            return;
        }
        points.erase(hasp);
        andrew_chain();
    }

    // https://en.wikibooks.org/wiki/Algorithm_Implementation/Geometry/Convex_hull/Monotone_chain#C++
    void andrew_chain() {
        int n = points.size();
        if (n <= 3) {
            hull = points;
            return;
        }
        int k = 0;

        std::vector<glm::vec2> h(2 * n);
        std::sort(points.begin(), points.end(), CompareVec());

        for (int i = 0; i < n; i++) {
            while (k >= 2 && cross(h[k - 2], h[k - 1], points[i]) <= 0) k--;
            h[k++] = points[i];
        }

        for (int i = n - 1, t = k + 1; i > 0; i--) {
            while (k >= t && cross(h[k - 2], h[k - 1], points[i - 1]) <= 0) k--;
            h[k++] = points[i - 1];
        }

        h.resize(k - 1);
        hull = h;
    }

    void grahamScan(glm::vec2 p) {
        // If point is inside p
        if (inside(p)) return;

        // point having minimum distance from the point p
        int ind = 0;
        int n = hull.size();
        for (int i = 1; i < n; i++)
            if (sqDist(p, hull[i]) < sqDist(p, hull[ind])) ind = i;

        // Find the upper tangent
        int up = ind;
        while (orientation(p, hull[up], hull[(up + 1) % n]) >= 0)
            up = (up + 1) % n;

        // Find the lower tangent
        int low = ind;
        while (orientation(p, hull[low], hull[(n + low - 1) % n]) <= 0)
            low = (n + low - 1) % n;

        // Initialize result
        std::vector<glm::vec2> ret;

        // making the final hull by traversing points
        // from up to low of given convex hull.
        int curr = up;
        ret.push_back(hull[curr]);
        while (curr != low) {
            curr = (curr + 1) % n;
            ret.push_back(hull[curr]);
        }

        // Modify the original vector
        ret.push_back(p);
        hull.clear();
        for (size_t i = 0; i < ret.size(); i++) hull.push_back(ret[i]);
    }
};

struct NavMesh {
    std::map<int, Polygon> entityShapes;
    std::vector<Polygon> shapes;

    void addEntity(int e_id, Polygon p) { entityShapes.insert({e_id, p}); }
    void removeEntity(int e_id) {
        auto it = entityShapes.find(e_id);
        if (it != entityShapes.end()) entityShapes.erase(it);
        return;
    }

    void addShape(Polygon p, bool merge = true) {
        shapes.push_back(p);

        if (!merge) return;

        size_t i = 0;
        size_t j = 1;
        while (i < shapes.size()) {
            j = i + 1;
            while (j < shapes.size()) {
                if (overlap(shapes[i], shapes[j])) {
                    for (auto pt : shapes[j].points) {
                        shapes[i].add(pt);
                    }
                    shapes.erase(shapes.begin() + j);
                } else {
                    j++;
                }
            }
            i++;
        }
    }

    void removeShape(Polygon p) {
        // TODO this might split the shape in two
        // how do we figure out if we need to split ...
        for (auto& s : shapes) {
            if (overlap(s, p)) {
                for (auto pt : p.points) {
                    s.remove(pt);
                }
            }
        }
    }

    bool overlap(Polygon a, Polygon b) const {
        // first check if the two max radii circles overlap
        for (size_t i = 0; i < a.points.size(); i++) {
            if (b.inside(a.points[i])) return true;
        }
        return false;
    }
};
static NavMesh __navmesh___DO_NOT_USE_DIRECTLY;

static void test_polygon_hull() {
    Polygon polygon;
    glm::vec2 points[] = {{0, 3}, {1, 1}, {2, 2}, {4, 4},
                          {0, 0}, {1, 2}, {3, 1}, {3, 3}};
    std::vector<glm::vec2> hull;

    for (auto aa : points) {
        polygon.add(aa);
        // Print the modified Convex Hull
        // for (auto e : polygon.hull)
        // std::cout << "(" << e.x << ", " << e.y << ") ";
        // std::cout << std::endl;
    }

    std::array<glm::vec2, 4> ans = {
        glm::vec2{4.f, 4.f},
        glm::vec2{0.f, 3.f},
        glm::vec2{0.f, 0.f},
        glm::vec2{3.f, 1.f},
    };
    int found = 0;
    for (auto i : ans) {
        for (auto e : polygon.hull) {
            if (i == e) {
                found++;
                break;
            }
        }
    }
    M_ASSERT(found == 4, "found all points and no more");
    M_ASSERT(polygon.hull.size() == 4, "found all points and no more");
}

static void test_navmesh_shape_nomerge() {
    Polygon a;
    a.add(glm::vec2{0.f, 0.f});
    a.add(glm::vec2{0.f, 1.f});
    a.add(glm::vec2{1.f, 1.f});
    a.add(glm::vec2{1.f, 0.f});
    // a box 00 to 11

    Polygon b;
    b.add(glm::vec2{2.f, 2.f});
    b.add(glm::vec2{2.f, 3.f});
    b.add(glm::vec2{3.f, 3.f});
    b.add(glm::vec2{3.f, 2.f});
    // a box 22 to 33

    NavMesh nm;
    nm.addShape(a);
    M_ASSERT(nm.shapes.size() == 1, "NavMesh should have one shape");
    nm.addShape(b);
    M_ASSERT(nm.shapes.size() == 2, "NavMesh should have two shapes");
}

static void test_navmesh_shape_merge_same() {
    Polygon a;
    a.add(glm::vec2{0.f, 0.f});
    a.add(glm::vec2{0.f, 1.f});
    a.add(glm::vec2{1.f, 1.f});
    a.add(glm::vec2{1.f, 0.f});
    // a box 00 to 11

    Polygon b;
    b.add(glm::vec2{0.f, 0.f});
    b.add(glm::vec2{0.f, 1.f});
    b.add(glm::vec2{1.f, 1.f});
    b.add(glm::vec2{1.f, 0.f});
    // another box from 00 to 11

    NavMesh nm;
    nm.addShape(a);
    M_ASSERT(nm.shapes.size() == 1, "NavMesh should have one shape");
    M_ASSERT(nm.shapes[0].points.size() == a.points.size(),
             "NavMesh should have one shape with points matching a");
    nm.addShape(b);
    M_ASSERT(nm.shapes.size() == 1, "NavMesh should have one shapes");

    std::array<glm::vec2, 4> ans = {
        glm::vec2{0.f, 0.f},
        glm::vec2{0.f, 1.f},
        glm::vec2{1.f, 1.f},
        glm::vec2{1.f, 0.f},
    };
    int found = 0;
    for (auto i : ans) {
        for (auto e : nm.shapes[0].hull) {
            if (i == e) {
                found++;
                break;
            }
        }
    }
    M_ASSERT(found == 4, "found all points and no more");
    M_ASSERT(nm.shapes[0].hull.size() == 4, "found all points and no more");
}

static void test_navmesh_shape_merge_overlap() {
    Polygon a;
    a.add(glm::vec2{0.f, 0.f});
    a.add(glm::vec2{0.f, 2.f});
    a.add(glm::vec2{2.f, 2.f});
    a.add(glm::vec2{2.f, 0.f});
    // a box 00 to 22

    Polygon b;
    b.add(glm::vec2{1.f, 1.f});
    b.add(glm::vec2{1.f, 3.f});
    b.add(glm::vec2{3.f, 3.f});
    b.add(glm::vec2{3.f, 1.f});
    // another box from 11 to 33

    NavMesh nm;

    M_ASSERT(nm.overlap(a, b), "shapes should overlap");

    nm.addShape(a);
    M_ASSERT(nm.shapes.size() == 1, "NavMesh should have one shape");
    M_ASSERT(nm.shapes[0].points.size() == a.points.size(),
             "NavMesh should have one shape with points matching a");
    nm.addShape(b);
    M_ASSERT(nm.shapes.size() == 1, "NavMesh should have one shape");

    // for (auto e : nm.shapes[0].points)
    // std::cout << "(" << e.x << ", " << e.y << ") ";
    // std::cout << std::endl;
    M_ASSERT(nm.shapes[0].points.size() == (a.points.size() + b.points.size()),
             "NavMesh should have one shape with points matching a and b");

    // for (auto e : nm.shapes[0].hull)
    // std::cout << "(" << e.x << ", " << e.y << ") ";
    // std::cout << std::endl;

    /*
                B++++++++++B
                +          +
                +          +
           A----+-----A    +
           |    +     |    +
           |    B+++++|++++B
           |          |
           |          |
           A----------A

                B++++++++++B
              /            +
             /             +
           A          A    +
           |               +
           |    B          B
           |             /
           |            /
           A----------A
    */

    std::array<glm::vec2, 6> ans = {
        glm::vec2{0.f, 0.f}, glm::vec2{2.f, 0.f}, glm::vec2{3.f, 1.f},
        glm::vec2{3.f, 3.f}, glm::vec2{1.f, 3.f}, glm::vec2{0.f, 2.f},
    };
    int found = 0;
    for (auto i : ans) {
        for (auto e : nm.shapes[0].hull) {
            if (i == e) {
                found++;
                break;
            }
        }
    }
    M_ASSERT(found == ans.size(), "found all points and no more");
    M_ASSERT(nm.shapes[0].hull.size() == ans.size(),
             "found all points and no more");
}

inline void test_navmesh() {
    test_navmesh_shape_nomerge();
    test_navmesh_shape_merge_same();
    test_navmesh_shape_merge_overlap();
}
