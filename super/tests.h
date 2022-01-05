
#pragma once

#include <chrono>

#include "../engine/external_include.h"
#include "../engine/thetastar.h"
#include "entities.h"
#include "entity.h"

inline float run_pathfinding(
    const std::function<bool(const glm::vec2&)> canwalk) {
    auto start_tm = std::chrono::high_resolution_clock::now();
    {
        glm::vec2 start = {0.f, 0.f};
        glm::vec2 end = {6.f, 0.f};
        auto emp = Employee();
        emp.position = start;
        emp.size = {0.6f, 0.6f};
        LazyTheta t(start, end, canwalk);
        auto result = t.go();
        std::reverse(result.begin(), result.end());
        // for (auto i : result) log_info("{}", i);
        M_ASSERT(result.size(), "Path is empty but shouldnt be");
    }
    auto end = std::chrono::high_resolution_clock::now();
    long long start_as_ms =
        std::chrono::time_point_cast<std::chrono::microseconds>(start_tm)
            .time_since_epoch()
            .count();
    long long end_as_ms =
        std::chrono::time_point_cast<std::chrono::microseconds>(end)
            .time_since_epoch()
            .count();
    float duration = (end_as_ms - start_as_ms);
    return duration;
}

inline void quadtree_test() {
    log_trace("quadtree_test() start");

    using namespace quadtree;
    auto entityToQuad = [](const std::shared_ptr<Entity>& e) {
        return Box<float>(e->position.x, e->position.y, e->size.x, e->size.y);
    };
    auto box = [](const glm::vec2 pos, const glm::vec2 size) {
        return Box<float>(pos.x, pos.y, size.x, size.y);
    };

    Quadtree<std::shared_ptr<Entity>, decltype(entityToQuad)> qt(
        Box<float>(0.f, 0.f, 20.f, 20.f), entityToQuad);

    auto shelf =
        std::make_shared<Shelf>(glm::vec2{1.f, 0.f}, glm::vec2{1.f, 1.f}, 0.f,
                                glm::vec4{1.0f, 1.0f, 1.0f, 1.0f}, "box");
    qt.add(shelf);

    auto shelf2 =
        std::make_shared<Shelf>(glm::vec2{3.f, 0.f}, glm::vec2{1.f, 1.f}, 0.f,
                                glm::vec4{1.0f, 1.0f, 1.0f, 1.0f}, "box");
    qt.add(shelf2);

    // Check an area with no items in it
    // -10, -10, -9, -9
    auto overlaps = qt.query(box(glm::vec2{9.f}, glm::vec2{1.f}));
    M_ASSERT(overlaps.size() == 0, "First query should have no matches");

    // 1, 0, 2, 1
    overlaps = qt.query(box(glm::vec2{1.f, 0.f}, glm::vec2{1.f}));
    M_ASSERT(overlaps.size() == 1, "Second query should have a match");
    M_ASSERT(overlaps[0]->id == shelf->id, "Second query should match shelf");

    log_trace("QT queries worked correctly");
    // Now try the path finding
    auto canwalk = [&](const glm::vec2& pos) {
        return qt.query(box(pos, glm::vec2{1.f})).empty();
    };
    auto elapsed = run_pathfinding(canwalk);
    log_trace("Using QT for ThetaStar with {} entities took {}", 3, elapsed);

    for (int i = 6; i < 20; i++) {
        for (int j = 6; j < 20; j++) {
            qt.add(std::make_shared<Shelf>(
                glm::vec2{(float)i, (float)j},      //
                glm::vec2{1.f, 1.f},                //
                0.f,                                //
                glm::vec4{1.0f, 1.0f, 1.0f, 1.0f},  //
                "box"                               //
                ));
        }
    }

    elapsed = run_pathfinding(canwalk);
    log_trace("Using QT for ThetaStar with {} entities took {}", qt.num_t,
              elapsed);

    log_trace("quadtree_test() end");
}

inline void theta_test() {
    log_trace("theta_test() start");

    // Walk straight through
    //  - i expect it to just walk around

    auto shelf =
        std::make_shared<Shelf>(glm::vec2{1.f, 0.f}, glm::vec2{1.f, 1.f}, 0.f,
                                glm::vec4{1.0f, 1.0f, 1.0f, 1.0f}, "box");
    entities.push_back(shelf);

    auto shelf2 =
        std::make_shared<Shelf>(glm::vec2{3.f, 0.f}, glm::vec2{1.f, 1.f}, 0.f,
                                glm::vec4{1.0f, 1.0f, 1.0f, 1.0f}, "box");
    entities.push_back(shelf2);

    auto duration = run_pathfinding(std::bind(
        EntityHelper::isWalkable, std::placeholders::_1, glm::vec2{1.f}));
    log_trace("ThetaStar with EntityHelper with {} entities took: {}",
              entities.size(), duration);

    for (int i = 6; i < 20; i++) {
        for (int j = 6; j < 20; j++) {
            entities.push_back(std::make_shared<Shelf>(
                glm::vec2{i, j},                    //
                glm::vec2{1.f, 1.f},                //
                0.f,                                //
                glm::vec4{1.0f, 1.0f, 1.0f, 1.0f},  //
                "box"                               //
                ));
        }
    }
    duration = run_pathfinding(std::bind(
        EntityHelper::isWalkable, std::placeholders::_1, glm::vec2{1.f}));
    log_trace("ThetaStar with EntityHelper with {} entities took: {}",
              entities.size(), duration);

    entities.clear();
    log_trace("theta_test() end");
    return;
}

// This is actually being used in main.cpp
// but Coc::Clang doesnt seem to care :)
__attribute__((unused))  //
void point_collision_test() {
    auto shelf =
        std::make_shared<Shelf>(glm::vec2{0.f, 0.f}, glm::vec2{2.f, 2.f}, 0.f,
                                glm::vec4{1.0f, 1.0f, 1.0f, 1.0f}, "box");

    M_ASSERT(shelf->pointCollides(glm::vec2{0.f, 0.f}) == true, "00");
    M_ASSERT(shelf->pointCollides(glm::vec2{1.f, 0.f}) == true, "10");
    M_ASSERT(shelf->pointCollides(glm::vec2{0.f, 1.f}) == true, "01");
    M_ASSERT(shelf->pointCollides(glm::vec2{1.f, 1.f}) == true, "11");
    M_ASSERT(shelf->pointCollides(glm::vec2{3.f, 3.f}) == false, "33");
    M_ASSERT(shelf->pointCollides(glm::vec2{01.f, 0.f}) == true, "010");
    M_ASSERT(shelf->pointCollides(glm::vec2{1.9f, 0.f}) == true, "110");
    M_ASSERT(shelf->pointCollides(glm::vec2{01.f, 1.f}) == true, "011");
    M_ASSERT(shelf->pointCollides(glm::vec2{1.1f, 1.f}) == true, "111");
    M_ASSERT(shelf->pointCollides(glm::vec2{2.0001f, 3.f}) == false, "200013");
}

