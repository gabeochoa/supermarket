
#pragma once

#include "../vendor/supermarket-engine/engine/external_include.h"
#include "../vendor/supermarket-engine/engine/thetastar.h"
#include "../vendor/supermarket-engine/engine/trie.h"
#include "entities.h"
#include "entity.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"

void theta_test() {
    // Walk straight through
    //  - i expect it to just walk around

    // TODO add a way for us to test this without clobbering entities....

    auto ll = LOG_LEVEL;
    LOG_LEVEL = LogLevel::ALL;
    {
        auto shelf = std::make_shared<Shelf>(
            glm::vec2{1.f, 0.f}, glm::vec2{1.f, 1.f}, 0.f,
            glm::vec4{1.0f, 1.0f, 1.0f, 1.0f}, "box");
        entities_DO_NOT_USE.push_back(shelf);

        auto shelf2 = std::make_shared<Shelf>(
            glm::vec2{3.f, 0.f}, glm::vec2{1.f, 1.f}, 0.f,
            glm::vec4{1.0f, 1.0f, 1.0f, 1.0f}, "box");
        entities_DO_NOT_USE.push_back(shelf2);

        glm::vec2 start = {0.f, 0.f};
        glm::vec2 end = {6.f, 0.f};

        auto emp = Employee();
        emp.position = start;
        emp.size = {0.6f, 0.6f};
        entities_DO_NOT_USE.push_back(std::make_shared<Employee>(emp));

        LazyTheta t(start, end, glm::vec4{-2.f, -2.f, 10.f, 10.f},
                    std::bind(EntityHelper::isWalkable, std::placeholders::_1,
                              emp.size));
        auto result = t.go();
        std::reverse(result.begin(), result.end());
        for (auto i : result) {
            log_info("{}", i);
        }
        M_ASSERT(result.size(), "Path is empty but shouldnt be");
        entities_DO_NOT_USE.clear();
    }
    LOG_LEVEL = ll;
    return;
}

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

void all_tests() {
    prof give_me_a_name(__PROFILE_FUNC__);
    theta_test();
    point_collision_test();

    {  // make sure linear interp always goes up
        float c = 0.f;
        auto a = LinearInterp(0.f, 1.f, 100);
        for (int i = 0; i < 100; i++) {
            auto b = a.next();
            M_ASSERT(c - b < 0.1, "should linearly interpolate");
            c += 0.01;
        }
    }
    // test_file_functionality();

    test_global_commands();
    test_polygon_hull();
    test_navmesh();
    test_trie();
    log_info("Finished running all tests successfully");
}

#pragma clang diagnostic pop
