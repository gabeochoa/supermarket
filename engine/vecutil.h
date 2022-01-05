#pragma once

#include "external_include.h"
#include "math.h"

inline glm::vec2 lerp(const glm::vec2& x, const glm::vec2& y, float t) {
    return x * (1.f - t) + y * t;
}

inline glm::vec2 gen_rand_vec2(float min, float max) {
    return {
        glm::linearRand(min, max),
        glm::linearRand(min, max),
    };
}

inline glm::vec3 gen_rand_vec3(float min, float max) {
    return {
        glm::linearRand(min, max),
        glm::linearRand(min, max),
        glm::linearRand(min, max),
    };
}

inline glm::vec4 gen_rand_vec4(float min, float max) {
    return {
        glm::linearRand(min, max),
        glm::linearRand(min, max),
        glm::linearRand(min, max),
        glm::linearRand(min, max),
    };
}

struct VectorHash {
    std::size_t operator()(const glm::vec2& a) const {
        return std::hash<float>()(a.x) ^ ((std::hash<float>()(a.y) << 1) >> 1);
    }
};

inline glm::vec3 worldToScreen(const glm::vec3& position, const glm::mat4& view,
                               const glm::mat4& projection,
                               const glm::vec4& viewport) {
    return glm::project(position, view, projection, viewport);
}

inline glm::vec3 screenToWorld(const glm::vec3& position, const glm::mat4& view,
                               const glm::mat4& projection,
                               const glm::vec4& viewport) {
    return glm::unProject(position, view, projection, viewport);
}

template <>
inline glm::vec2 round_higher(glm::vec2 val) {
    return glm::vec2{round_higher(val.x), round_higher(val.y)};
}

inline glm::vec4 posSizeToRect(glm::vec2 pos, glm::vec2 size) {
    return glm::vec4{pos, pos.x + size.x, pos.y + size.y};
}

inline bool aabb(glm::vec4 a, glm::vec4 b) {
    return (          //
        a.x < b.z &&  //
        a.z > b.x &&  //
        a.y < b.w &&  //
        a.w > b.y     //
    );
}

inline bool aabb(glm::vec2 apos, glm::vec2 asize, glm::vec2 bpos,
                 glm::vec2 bsize) {
    return aabb(posSizeToRect(apos, asize), posSizeToRect(bpos, bsize));
}

