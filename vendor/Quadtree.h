#pragma once

#include <algorithm>
#include <cassert>
#include <memory>
#include <type_traits>
#include <vector>

// By default disabling query bounds checking
// #define SUPERMARKET_QUADTREE_ENFORCE_INSIDE_DURING_QUERY
// to re-enable it

// Also I added entity count just so I can understand how many entities there
// are
// #define SUPERMARKET_REMOVE_ENTITY_COUNT
// to disable it

namespace quadtree {

template <typename T>
class Vector2 {
   public:
    T x;
    T y;

    constexpr Vector2<T>(T X = 0, T Y = 0) noexcept : x(X), y(Y) {}

    constexpr Vector2<T>& operator+=(const Vector2<T>& other) noexcept {
        x += other.x;
        y += other.y;
        return *this;
    }

    constexpr Vector2<T>& operator/=(T t) noexcept {
        x /= t;
        y /= t;
        return *this;
    }
};

template <typename T>
constexpr Vector2<T> operator+(Vector2<T> lhs, const Vector2<T>& rhs) noexcept {
    lhs += rhs;
    return lhs;
}

template <typename T>
constexpr Vector2<T> operator/(Vector2<T> vec, T t) noexcept {
    vec /= t;
    return vec;
}

}  // namespace quadtree

namespace quadtree {

template <typename T>
class Box {
   public:
    T left;
    T top;
    T width;   // Must be positive
    T height;  // Must be positive

    constexpr Box(T Left = 0, T Top = 0, T Width = 0, T Height = 0) noexcept
        : left(Left), top(Top), width(Width), height(Height) {}

    constexpr Box(const Vector2<T>& position, const Vector2<T>& size) noexcept
        : left(position.x), top(position.y), width(size.x), height(size.y) {}

    constexpr T getRight() const noexcept { return left + width; }

    constexpr T getBottom() const noexcept { return top + height; }

    constexpr Vector2<T> getTopLeft() const noexcept {
        return Vector2<T>(left, top);
    }

    constexpr Vector2<T> getCenter() const noexcept {
        return Vector2<T>(left + width / 2, top + height / 2);
    }

    constexpr Vector2<T> getSize() const noexcept {
        return Vector2<T>(width, height);
    }

    constexpr bool contains(const Box<T>& box) const noexcept {
        return left <= box.left && box.getRight() <= getRight() &&
               top <= box.top && box.getBottom() <= getBottom();
    }

    constexpr bool intersects(const Box<T>& box) const noexcept {
        return !(left >= box.getRight() || getRight() <= box.left ||
                 top >= box.getBottom() || getBottom() <= box.top);
    }
};

}  // namespace quadtree

namespace quadtree {

template <typename T, typename GetBox, typename Equal = std::equal_to<T>,
          typename Float = float>
class Quadtree {
    static_assert(
        std::is_convertible_v<std::invoke_result_t<GetBox, const T&>,
                              Box<Float>>,
        "GetBox must be a callable of signature Box<Float>(const T&)");
    static_assert(
        std::is_convertible_v<std::invoke_result_t<Equal, const T&, const T&>,
                              bool>,
        "Equal must be a callable of signature bool(const T&, const T&)");
    static_assert(std::is_arithmetic_v<Float>);

   public:
    Quadtree(const Box<Float>& box, const GetBox& getBox = GetBox(),
             const Equal& equal = Equal())
        : mBox(box),
          mRoot(std::make_unique<Node>()),
          mGetBox(getBox),
          mEqual(equal) {}

#ifndef SUPERMARKET_REMOVE_ENTITY_COUNT
    int num_t = 0;
#endif

    void add(const T& value) {
#ifndef SUPERMARKET_REMOVE_ENTITY_COUNT
        num_t++;
#endif
        add(mRoot.get(), 0, mBox, value);
    }

    void remove(const T& value) {
#ifndef SUPERMARKET_REMOVE_ENTITY_COUNT
        num_t--;
#endif
        remove(mRoot.get(), mBox, value);
    }

    std::vector<T> query(const Box<Float>& box) const {
        auto values = std::vector<T>();
        query(mRoot.get(), mBox, box, values);
        return values;
    }

    std::vector<std::pair<T, T>> findAllIntersections() const {
        auto intersections = std::vector<std::pair<T, T>>();
        findAllIntersections(mRoot.get(), intersections);
        return intersections;
    }

   private:
    static constexpr auto Threshold = std::size_t(16);
    static constexpr auto MaxDepth = std::size_t(8);

    struct Node {
        std::array<std::unique_ptr<Node>, 4> children;
        std::vector<T> values;
    };

    Box<Float> mBox;
    std::unique_ptr<Node> mRoot;
    GetBox mGetBox;
    Equal mEqual;

    bool isLeaf(const Node* node) const {
        return !static_cast<bool>(node->children[0]);
    }

    Box<Float> computeBox(const Box<Float>& box, int i) const {
        auto origin = box.getTopLeft();
        auto childSize = box.getSize() / static_cast<Float>(2);
        switch (i) {
            // North West
            case 0:
                return Box<Float>(origin, childSize);
            // Norst East
            case 1:
                return Box<Float>(
                    Vector2<Float>(origin.x + childSize.x, origin.y),
                    childSize);
            // South West
            case 2:
                return Box<Float>(
                    Vector2<Float>(origin.x, origin.y + childSize.y),
                    childSize);
            // South East
            case 3:
                return Box<Float>(origin + childSize, childSize);
            default:
                assert(false && "Invalid child index");
                return Box<Float>();
        }
    }

    int getQuadrant(const Box<Float>& nodeBox,
                    const Box<Float>& valueBox) const {
        auto center = nodeBox.getCenter();
        // West
        if (valueBox.getRight() < center.x) {
            // North West
            if (valueBox.getBottom() < center.y) return 0;
            // South West
            else if (valueBox.top >= center.y)
                return 2;
            // Not contained in any quadrant
            else
                return -1;
        }
        // East
        else if (valueBox.left >= center.x) {
            // North East
            if (valueBox.getBottom() < center.y) return 1;
            // South East
            else if (valueBox.top >= center.y)
                return 3;
            // Not contained in any quadrant
            else
                return -1;
        }
        // Not contained in any quadrant
        else
            return -1;
    }

    void add(Node* node, std::size_t depth, const Box<Float>& box,
             const T& value) {
        assert(node != nullptr);
        assert(box.contains(mGetBox(value)));
        if (isLeaf(node)) {
            // Insert the value in this node if possible
            if (depth >= MaxDepth || node->values.size() < Threshold)
                node->values.push_back(value);
            // Otherwise, we split and we try again
            else {
                split(node, box);
                add(node, depth, box, value);
            }
        } else {
            auto i = getQuadrant(box, mGetBox(value));
            // Add the value in a child if the value is entirely contained in it
            if (i != -1)
                add(node->children[static_cast<std::size_t>(i)].get(),
                    depth + 1, computeBox(box, i), value);
            // Otherwise, we add the value in the current node
            else
                node->values.push_back(value);
        }
    }

    void split(Node* node, const Box<Float>& box) {
        assert(node != nullptr);
        assert(isLeaf(node) && "Only leaves can be split");
        // Create children
        for (auto& child : node->children) child = std::make_unique<Node>();
        // Assign values to children
        auto newValues = std::vector<T>();  // New values for this node
        for (const auto& value : node->values) {
            auto i = getQuadrant(box, mGetBox(value));
            if (i != -1)
                node->children[static_cast<std::size_t>(i)]->values.push_back(
                    value);
            else
                newValues.push_back(value);
        }
        node->values = std::move(newValues);
    }

    bool remove(Node* node, const Box<Float>& box, const T& value) {
        assert(node != nullptr);
        assert(box.contains(mGetBox(value)));
        if (isLeaf(node)) {
            // Remove the value from node
            removeValue(node, value);
            return true;
        } else {
            // Remove the value in a child if the value is entirely contained in
            // it
            auto i = getQuadrant(box, mGetBox(value));
            if (i != -1) {
                if (remove(node->children[static_cast<std::size_t>(i)].get(),
                           computeBox(box, i), value))
                    return tryMerge(node);
            }
            // Otherwise, we remove the value from the current node
            else
                removeValue(node, value);
            return false;
        }
    }

    void removeValue(Node* node, const T& value) {
        // Find the value in node->values
        auto it = std::find_if(
            std::begin(node->values), std::end(node->values),
            [this, &value](const auto& rhs) { return mEqual(value, rhs); });
        assert(it != std::end(node->values) &&
               "Trying to remove a value that is not present in the node");
        // Swap with the last element and pop back
        *it = std::move(node->values.back());
        node->values.pop_back();
    }

    bool tryMerge(Node* node) {
        assert(node != nullptr);
        assert(!isLeaf(node) && "Only interior nodes can be merged");
        auto nbValues = node->values.size();
        for (const auto& child : node->children) {
            if (!isLeaf(child.get())) return false;
            nbValues += child->values.size();
        }
        if (nbValues <= Threshold) {
            node->values.reserve(nbValues);
            // Merge the values of all the children
            for (const auto& child : node->children) {
                for (const auto& value : child->values)
                    node->values.push_back(value);
            }
            // Remove the children
            for (auto& child : node->children) child.reset();
            return true;
        } else
            return false;
    }

    void query(Node* node, const Box<Float>& box, const Box<Float>& queryBox,
               std::vector<T>& values) const {
        assert(node != nullptr);
#ifdef SUPERMARKET_QUADTREE_ENFORCE_INSIDE_DURING_QUERY
        assert(queryBox.intersects(box));
#else
        // if we are searching outside our global search area
        // just return early since nothing will match for sure
        if (!queryBox.intersects(box)) return;
#endif
        for (const auto& value : node->values) {
            if (queryBox.intersects(mGetBox(value))) values.push_back(value);
        }
        if (!isLeaf(node)) {
            for (auto i = std::size_t(0); i < node->children.size(); ++i) {
                auto childBox = computeBox(box, static_cast<int>(i));
                if (queryBox.intersects(childBox))
                    query(node->children[i].get(), childBox, queryBox, values);
            }
        }
    }

    void findAllIntersections(
        Node* node, std::vector<std::pair<T, T>>& intersections) const {
        // Find intersections between values stored in this node
        // Make sure to not report the same intersection twice
        for (auto i = std::size_t(0); i < node->values.size(); ++i) {
            for (auto j = std::size_t(0); j < i; ++j) {
                if (mGetBox(node->values[i])
                        .intersects(mGetBox(node->values[j])))
                    intersections.emplace_back(node->values[i],
                                               node->values[j]);
            }
        }
        if (!isLeaf(node)) {
            // Values in this node can intersect values in descendants
            for (const auto& child : node->children) {
                for (const auto& value : node->values)
                    findIntersectionsInDescendants(child.get(), value,
                                                   intersections);
            }
            // Find intersections in children
            for (const auto& child : node->children)
                findAllIntersections(child.get(), intersections);
        }
    }

    void findIntersectionsInDescendants(
        Node* node, const T& value,
        std::vector<std::pair<T, T>>& intersections) const {
        // Test against the values stored in this node
        for (const auto& other : node->values) {
            if (mGetBox(value).intersects(mGetBox(other)))
                intersections.emplace_back(value, other);
        }
        // Test against values stored into descendants of this node
        if (!isLeaf(node)) {
            for (const auto& child : node->children)
                findIntersectionsInDescendants(child.get(), value,
                                               intersections);
        }
    }
};

}  // namespace quadtree