
#pragma once

#include "../engine/pch.hpp"
#include "entity.h"

struct Billboard : public Entity {
    // Billboard is a textured ent that never moves
    Billboard(const glm::vec2& position, const glm::vec2& size, float angle,
              const glm::vec4& color, const std::string& textureName = "white")
        : Entity(position, size, angle, color, textureName) {}

    virtual ~Billboard() {}
    virtual const char* typeString() const override { return "Billboard"; }
};

#include "item.h"
#include "movable_entities.h"
