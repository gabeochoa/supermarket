
#pragma once

#include <atomic>

#include "../engine/edit.h"
#include "../engine/pch.hpp"
#include "../engine/renderer.h"

struct Storable;

constexpr inline std::array<glm::vec2, 4> getBoundingBox(glm::vec2 position,
                                                         glm::vec2 size) {
    return {position,
            {position.x + size.x, position.y + 0},
            {position.x, position.y + size.y},
            {position.x + size.x, position.y + size.y}};
}

static std::atomic_int ENTITY_ID_GEN;
struct Entity {
    int id;
    glm::vec2 position;
    glm::vec2 size;
    float angle;
    glm::vec4 color;
    std::string textureName;
    bool center = true;
    bool cleanup = false;

    Entity()
        : id(ENTITY_ID_GEN++),
          position({0.f, 0.f}),
          size({1.f, 1.f}),
          angle(0.f),
          color({1.f, 1.f, 1.f, 1.f}),
          textureName("white") {}

    Entity(const glm::vec2& position_, const glm::vec2& size_, float angle_,
           const glm::vec4& color_, const std::string& textureName_)
        : position(position_),
          size(size_),
          angle(angle_),
          color(color_),
          textureName(textureName_) {}

    virtual inline bool canMove() const { return false; }

    virtual ~Entity() {}

    virtual void onUpdate(Time dt) { (void)dt; }

    virtual bool pointCollides(const glm::vec2& m) const {
        auto [a, b, c, d] = getBoundingBox(position, size);

        auto ab = b - a;
        auto am = m - a;
        auto bc = c - b;
        auto bm = m - b;

        auto abam = glm::dot(ab, am);
        auto abab = glm::dot(ab, ab);
        auto bcbm = glm::dot(bc, bm);
        auto bcbc = glm::dot(bc, bc);

        return (             //
            0 <= abam &&     //
            abam <= abab &&  //
            0 < bcbm &&      //
            bcbm <= bcbc     //
        );                   //
    }

    constexpr inline glm::vec4 getRect() const {
        return posSizeToRect(this->position, this->size);
    }

    constexpr inline bool entityCollides(const glm::vec2 position,
                                         const glm::vec2 size) const {
        return aabb(this->position, this->size, position, size);
    }

    constexpr inline bool entityCollides(
        const std::shared_ptr<Entity>& other) const {
        return entityCollides(other->position, other->size);
    }

    struct RenderOptions {
        std::optional<glm::vec2> position;
        std::optional<glm::vec2> size;
        std::optional<glm::vec4> color;
        std::optional<std::string> textureName;
        std::optional<bool> center;
    };

    virtual void render(const RenderOptions& ro = RenderOptions()) {
        glm::vec2 position = this->position;
        glm::vec2 size = this->size;
        glm::vec4 color = this->color;
        bool center = this->center;
        float angle = this->angle;
        std::string textureName = this->textureName;

        if (ro.position) position = ro.position.value();
        if (ro.size) size = ro.size.value();
        if (ro.color) color = ro.color.value();
        if (ro.textureName) textureName = ro.textureName.value();
        if (ro.center) center = ro.center.value();

        // computing angle transforms are expensive so
        // if the angle is under thresh, just render it square
        if (angle <= 5.f) {
            auto loc = glm::vec2{
                position.x,
                position.y,
            };
            if (center) {
                loc = loc + glm::vec2{size.x / 2, size.y / 2};
            }
            Renderer::drawQuad(loc, size, color, textureName);
        } else {
            Renderer::drawQuadRotated(position, size, glm::radians(angle),
                                      color, textureName);
        }
    }

    virtual const Entity* asEntity() const { return this; }

    constexpr inline void announce(const std::string& tosay) const {
        log_info("{}: {}", *asEntity(), tosay);
    }

    virtual const char* typeString() const = 0;

    bool operator==(const Entity& other) { return this->id == other.id; }
};

template <>
struct fmt::formatter<Entity> {
    template <typename ParseContext>
    constexpr auto parse(ParseContext& ctx) {
        return ctx.begin();
    }

    template <typename FormatContext>
    constexpr auto format(Entity const& e, FormatContext& ctx) {
        return fmt::format_to(ctx.out(), "{}{} at {}", e.typeString(), e.id,
                              e.position);
    }
};

typedef quadtree::Quadtree<
    std::shared_ptr<Entity>,
    std::function<quadtree::Box<float>(const std::shared_ptr<Entity>&)>>
    EntityQT;

struct EntityHelper {
    static inline EntityQT* getQuadTreePtr() {
        return GLOBALS.get_ptr<EntityQT>("quadtree");
    }

    static void forEachEntityQT(
        const std::function<void(std::shared_ptr<Entity>)>& cb) {
        for (auto& e : *getQuadTreePtr()) {
            cb(e);
        }
    }

    static void forEachEntityVector(
        std::vector<std::shared_ptr<Entity>> entities,
        const std::function<void(std::shared_ptr<Entity>)>& cb) {
        for (auto& e : entities) {
            cb(e);
        }
    }

    static bool entityInLocation(glm::vec4 rect) {
        return !getQuadTreePtr()
                    ->query(quadtree::Box<float>::create(rect))
                    .empty();
    }

    static constexpr bool entityInLocation(glm::vec2 pos, glm::vec2 size) {
        return EntityHelper::entityInLocation(posSizeToRect(pos, size));
    }

    template <typename T>
    static constexpr std::shared_ptr<T> getRandomEntity(
        const glm::vec2& notpos = {-99.f, -99.f}) {
        std::vector<std::shared_ptr<T>> matching;
        forEachEntityQT([&](const std::shared_ptr<Entity>& e) {
            auto s = dynamic_pointer_cast<T>(e);
            if (!s) return;
            if (glm::distance(s->position, notpos) > 1.f) matching.push_back(s);
        });
        int i = randIn(0, matching.size() - 1);
        return matching[i];
    }

    template <typename T>
    static constexpr std::vector<std::shared_ptr<T>> getEntitiesInRange(
        glm::vec2 pos, float range) {
        std::vector<std::shared_ptr<T>> matching;
        forEachEntityQT([&](const std::shared_ptr<Entity>& e) {
            auto s = dynamic_pointer_cast<T>(e);
            if (!s) return;
            if (glm::distance(pos, e->position) < range) {
                matching.push_back(s);
            }
        });
        return matching;
    }

    template <typename T>
    static std::vector<std::shared_ptr<T>> getEntityInRangeWithItem(
        glm::vec2 pos, int itemID, float range) {
        static_assert(
            std::is_base_of<Storable, T>::value,
            "Can only be called with a T that is a child of Storable");
        std::vector<std::shared_ptr<T>> matching;
        forEachEntityQT([&](const std::shared_ptr<Entity>& e) {
            auto s = dynamic_pointer_cast<T>(e);
            if (!s) return;
            if (glm::distance(pos, e->position) > range) return;
            if (s->contents.find(itemID) != s->contents.end()) {
                matching.push_back(s);
            }
        });
        return matching;
    }

    template <typename T>
    static std::vector<std::shared_ptr<T>> getEntityInSelection(
        glm::vec4 rect) {
        std::vector<std::shared_ptr<T>> matching;
        forEachEntityQT([&](const std::shared_ptr<Entity>& e) {
            auto s = dynamic_pointer_cast<T>(e);
            if (!s) return;
            if (aabb(s->getRect(), rect)) {
                matching.push_back(s);
            }
        });
        return matching;
    }

    static bool isWalkable(const glm::vec2& pos, const glm::vec2 size) {
        return getQuadTreePtr()->query(quadtree::Box<float>(pos, size)).empty();
    }

    static bool isWalkableEntities(
        std::vector<std::shared_ptr<Entity>> entities, const glm::vec2& pos,
        const glm::vec2 size) {
        for (auto& e : entities) {
            if (e->canMove()) {
                continue;
            }
            if (aabb(e->getRect(), posSizeToRect(pos, size))) {
                return false;
            }
        }
        return true;
    }
};

