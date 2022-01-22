
#pragma once

#include "../vendor/supermarket-engine/engine/entity.h"
#include "../vendor/supermarket-engine/engine/pch.hpp"
#include "../vendor/supermarket-engine/engine/ui.h"
#include "movable_entities.h"

struct DragArea : public Entity {
    bool isMouseDragging = false;
    glm::vec2 mouseDragStart;
    glm::vec2 mouseDragEnd;
    int tool = 0;

    std::vector<std::shared_ptr<Entity>> selected;

    DragArea(const glm::vec2& position, const glm::vec2& size, float angle,
             const glm::vec4& color, const std::string& textureName = "white")
        : Entity(position, size, angle, color, textureName) {
        mouseDragStart = glm::vec2{0.f};
        mouseDragEnd = glm::vec2{0.f};
    }

    virtual ~DragArea() {}
    virtual const char* typeString() const override { return "DragArea"; }

    void place(int selectedTool, std::string object) {
        tool = selectedTool;
        textureName = object;
    }

    virtual void onUpdate(Time dt) override {
        (void)dt;
        position = round_higher(mouseDragStart);
        size = round_higher(mouseDragEnd - mouseDragStart);

        if (tool != 0 && tool != 3) {
            if (abs(size.y) > abs(size.x)) {
                size = glm::vec2{1.f, size.y};
            } else {
                size = glm::vec2{size.x, 1.f};
            }
        }
    }

    void clear() {
        // clear drag area
        size = glm::vec2{0.f};
        position = glm::vec2{0.f};
        mouseDragStart = glm::vec2{0.f};
        mouseDragEnd = glm::vec2{0.f};
        tool = 0;
        textureName = "white";
    }

    void onDragStart(glm::vec3 mouse) {
        selected.clear();
        mouseDragStart = mouse;
        mouseDragEnd = mouse;
    }

    void onDragEnd() {
        if (tool != 0 && tool != 3) {
            if (0) {
            } else if (textureName == "shelf") {
                forEachPlaced(false, [](glm::vec2 pos) {
                    if (EntityHelper::entityInLocation(pos, glm::vec2{0.5f}))
                        return;
                    EntityHelper::addEntity(std::make_shared<Shelf>(Shelf(
                        pos, glm::vec2{1.f}, 0.f, glm::vec4{1.f}, "shelf")));
                });
            } else if (textureName == "box") {
                forEachPlaced(false, [](glm::vec2 pos) {
                    if (EntityHelper::entityInLocation(pos, glm::vec2{0.5f}))
                        return;
                    EntityHelper::addEntity(std::make_shared<Storage>(Storage(
                        pos, glm::vec2{1.f}, 0.f, glm::vec4{1.f}, "box")));
                });
            }
            clear();
            return;
        }

        auto a = mouseDragStart;
        auto b = mouseDragEnd;

        // we need to figure out the rect from the
        // bottom left to top right so finding works correctly
        // theres 4 ways you can drag the box out
        //  1. top left to bottom right
        //  2. top right to bottom left
        //  3. bottom left to top right
        //  4. bottom right to top left
        //
        auto rect = glm::vec4{0.f};
        if (0) {
        } else if (a.x <= b.x && a.y >= b.y) {
            rect = glm::vec4{a.x, b.y, b.x, a.y};
        } else if (a.x >= b.x && a.y >= b.y) {
            rect = glm::vec4{b.x, b.y, a.x, a.y};
        } else if (a.x <= b.x && a.y <= b.y) {
            rect = glm::vec4{a, b};
        } else if (a.x >= b.x && a.y <= b.y) {
            rect = glm::vec4{b.x, a.y, a.x, b.y};
        }
        selected = EntityHelper::getEntityInSelection<Entity>(rect);

        if (tool == 3) {
            delete_selected();
            selected.clear();
        }

        clear();
    }

    void forEachPlaced(bool center, std::function<void(glm::vec2)> cb) {
        // TODO theres something wrong with the selection,
        // it seems to move my selection up a block but not every time
        auto sx = sgn(size.x);
        auto sy = sgn(size.y);
        auto loc = position;
        if (center) loc = loc + glm::vec2{sx * 0.5f, sy * 0.5f};
        for (int i = 0; i < abs(size.x); i++) {
            for (int j = 0; j < abs(size.y); j++) {
                auto pos = loc + glm::vec2{sx * i, sy * j};
                cb(pos);
            }
        }
    }

    virtual void render(const RenderOptions& = RenderOptions()) override {
        if (textureName == "white") {
            // dont draw if we are 0 size
            if (size.x == 0 && size.y == 0) return;

            auto loc = position;
            if (center) {
                loc = loc + glm::vec2{size.x / 2, size.y / 2};
            }
            Renderer::drawQuad(loc, size, color, textureName);
            return;
        }

        // TODO better or worse than just creating a temporary item and
        // delegating rendering to that
        forEachPlaced(true, [&](glm::vec2 pos) {
            Renderer::drawQuad(  //
                pos,             //
                glm::vec2{1.f},  //
                color,           //
                textureName      //
            );
        });
    }

    void delete_selected() {
        for (auto e : selected) {
            e->cleanup = true;
        }
    }

    void render_selected() {
        // TODO should we just do "selected" in renderoptions directly
        for (auto& entity : selected) {
            entity->render(RenderOptions({
                .position = entity->position + (0.5f * glm::vec2{entity->size}),
                .color = std::make_optional(IUI::teal),
                .textureName = std::make_optional("white"),
                .size = entity->size + (entity->angle <= 5.f ? glm::vec2{0.1f}
                                                             : glm::vec2{0.0f}),
                .center = false,
            }));
        }
    }
};

