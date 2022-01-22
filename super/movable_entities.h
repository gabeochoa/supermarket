
#pragma once

#include "../vendor/supermarket-engine/engine/entity.h"
#include "../vendor/supermarket-engine/engine/maputil.h"
#include "../vendor/supermarket-engine/engine/pch.hpp"
#include "../vendor/supermarket-engine/engine/thetastar.h"
#include "item.h"
#include "job.h"

const float REACH_DIST = 1.4f;
const float TRAVEL_DIST = 0.2f;

inline std::vector<glm::vec2> generateWalkablePath(  //
    int skipID,                                      //
    float movement,                                  //
    const glm::vec2& start,                          //
    const glm::vec2& end,                            //
    const glm::vec2& size,                           //
    bool elideLineOfSight = true) {
    (void)skipID;
    (void)movement;

    // TODO @FIX trace actually will still run
    // we gotta do some kind of fancy #if log thing
    log_trace("starting theta");

    Theta t(start, end,
            // TODO figure out a better bounds than this
            glm::vec4{-20.f, -20.f, 20.f, 20.f},
            std::bind(EntityHelper::isWalkable, std::placeholders::_1, size),
            elideLineOfSight);
    auto a = t.go();
    std::reverse(a.begin(), a.end());
    for (auto i : a) {
        log_trace("{}", i);
    }
    return a;
}

struct MovableEntity : public Entity {
    const glm::vec2 INVALID = {-99.f, -99.f};
    glm::vec2 last = glm::vec2(INVALID);
    std::vector<glm::vec2> path;
    float moveSpeed = 0.05f;
    float timeBetweenMoves = 0.025f;
    float timeSinceLastMove = 0.025f;

    virtual inline bool canMove() const override { return true; }

    bool walkToLocation(const glm::vec2 location, const WorkInput& wi) {
        prof give_me_a_name(__PROFILE_FUNC__);
        if (glm::distance(position, location) > 1000.f) {
            // TODO why is this happening
            position = glm::vec2{0.f, 0.f};
            log_warn(
                "We dont support any maps over 1k distances so something is "
                "wrong (trying to walk from {} to {})",
                position, location);
            return true;
        }

        // Have we reached the position yet?
        // or have we just started ?
        if (path.empty()) {
            if (glm::distance(position, location) < TRAVEL_DIST) {
                return true;
            }
            // announce(
            // fmt::format(" distance to location end {}  (need to be within
            // {})", glm::distance(position, location), TRAVEL_DIST));

            path = generateWalkablePath(id, moveSpeed, position, location,
                                        this->size);
            return false;
        }

        // Did we already generate a path?
        if (!path.empty()) {
            // first time we are moving, just set last to our current position
            if (last == INVALID) last = glm::vec2(position);
            // try to grab the next spot in the path
            auto target = path.begin();
            if (target == path.end()) return true;

            if (!EntityHelper::isWalkable(*target, this->size)) {
                announce(fmt::format("my next target isnt walkable.... {}",
                                     *target));
            }

            // reached our local target, erase this
            // and next cycle we'll grab the next POI
            if (distance(position, *target) < TRAVEL_DIST) {
                path.erase(target);
                // announce(fmt::format(" reached local target, {} to go",
                // path.size()));
                return path.empty() ? true : false;
            }

            // TODO I keep seeing the path has some rogue points
            // I tried removing them and replacing the hole
            // with a path but theres issues where
            // when the point is closer than 2.f but further than TRAVEL_DIST
            // and the character will just sit there and do nothing since path
            // is always empty

            // TODO @FIX moveSpeed is not really moveSpeed,
            // its move distance and so we cant actually
            // change the speed without breaking pathing..
            // what we could is track the DT and just only apply the lerp
            // every x seconds instead
            //
            timeSinceLastMove += wi.dt.s();
            if (timeSinceLastMove >= timeBetweenMoves) {
                timeSinceLastMove = 0;
                position = lerp(position, *target, moveSpeed);
            }
            return false;
        }
        return false;
    }

    virtual void onUpdate(Time dt) override {
        (void)dt;
        if (angle >= 360) angle -= 360;
        if (angle < 0) angle += 360;
    }
};

struct Person : public MovableEntity {
    JobHandler handler;
    std::shared_ptr<Job> assignedJob;

    void startJob(const std::shared_ptr<Job> job) {
        assignedJob = job;
        if (!assignedJob) return;
        path.clear();
        assignedJob->isAssigned = true;
        announce(fmt::format("starting job {}", *job));
    }

    void workOrFindMore(Time dt) {
        if (!assignedJob) {
            // announce("finding new job");
            auto range = getJobRange();
            auto ptr = JobQueue::getNextInRange(id, range);
            startJob(ptr);
            return;
        }
        handler.handle(assignedJob, {dt});
        if (assignedJob->isComplete) {
            announce(fmt::format("finished with {}",
                                 jobTypeToString(assignedJob->type)));
            assignedJob.reset();
        }
    }

    virtual void onUpdate(Time dt) {
        if (handler.job_mapping.empty()) {
            registerJobHandlers();
        }
        workOrFindMore(dt);
    }
    virtual void registerJobHandlers() = 0;
    virtual JobRange getJobRange() { return {JobType::None, JobType::None}; }

    bool none(const std::shared_ptr<Job>& j, const WorkInput& input) {
        j->seconds = j->seconds - input.dt.s();
        if (j->seconds <= 0) {
            announce(fmt::format("completed job {}", jobTypeToString(j->type)));
            j->isComplete = true;
            return true;
        }
        return false;
    }

    virtual const char* typeString() const = 0;
};

