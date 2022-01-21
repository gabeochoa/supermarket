
#pragma once

#include "../vendor/supermarket-engine/engine/log.h"
#include "../vendor/supermarket-engine/engine/pch.hpp"

// Lower number means lower priority
enum JobType {
    None = 0,
    // Employee job types
    IdleWalk,
    DirectedWalk,
    Empty,
    Fill,

    // delineates job group
    INVALID_Customer_Boundary,
    // Customer job types
    LeaveStore,
    GotoRegister,
    // TODO make these the same so they have the same probability of being
    // chosen?
    IdleShop,
    FindItem,

    // always last
    MAX_JOB_TYPE,
};

constexpr inline const char* jobTypeToString(JobType t) {
    switch (t) {
        case JobType::None:
            return "None";
        case JobType::Fill:
            return "Fill";
        case JobType::Empty:
            return "Empty";
        case JobType::IdleWalk:
            return "IdleWalk";
        case JobType::DirectedWalk:
            return "DirectedWalk";
        case JobType::INVALID_Customer_Boundary:
            return "INVALID_Customer_Boundary";
        case JobType::FindItem:
            return "FindItem";
        case JobType::GotoRegister:
            return "GotoRegister";
        case JobType::IdleShop:
            return "IdleShop";
        case JobType::LeaveStore:
            return "LeaveStore";
        case JobType::MAX_JOB_TYPE:
            return "MAX_JOB_TYPE";
        default:
            log_warn("JobType {} has no toString handler", t);
            return "UNKNOWN TYPE";
    }
}

struct Job {
    JobType type;
    bool isComplete;
    bool isAssigned;
    int reserved = -1;
    glm::vec2 startPosition;
    glm::vec2 endPosition;
    int seconds;

    //
    int itemID;
    int itemAmount;

    // TODO do we need dynamic types so that we can enforce extra data?
    // just gotta remember which you are using
    // if you mess it up well too bad
    int jobStatus = 0;
    std::map<std::string, int> reg;
};

template <>
struct fmt::formatter<Job> {
    template <typename ParseContext>
    constexpr auto parse(ParseContext& ctx) {
        return ctx.begin();
    }

    template <typename FormatContext>
    constexpr auto format(Job const& j, FormatContext& ctx) {
        return fmt::format_to(ctx.out(),  //
                              "Job({}) {}->{} duration {}",
                              jobTypeToString(j.type), j.startPosition,
                              j.endPosition, j.seconds);
    }
};

struct JobRange {
    JobType start;
    JobType end;
};

static std::map<int, std::vector<std::shared_ptr<Job>>> jobs;
struct JobQueue {
    static void addJob(JobType t, const std::shared_ptr<Job>& j) {
        jobs[(int)t].push_back(j);
    }

    static int numOfJobsWithType(JobType t) { return jobs[(int)t].size(); }

    static std::vector<std::shared_ptr<Job>>::iterator getNextMatching(
        JobType t) {
        auto js = jobs[(int)t];
        for (auto it = js.begin(); it != js.end(); it++) {
            if ((*it)->isAssigned || (*it)->isComplete) continue;
            return it;
        }
        return jobs[(int)t].end();
    }

    static std::shared_ptr<Job> getNextInRange(int e_id, JobRange jr) {
        // note that we iterate backwards so that higher pri
        // gets chosen first
        // TODO - do we need to set a timer so that some jobs eventually
        // get forced to do
        for (int i = (int)jr.end; i >= jr.start; i--) {
            auto js = jobs[i];
            for (auto it = js.begin(); it != js.end(); it++) {
                if ((*it)->isAssigned || (*it)->isComplete) continue;
                if ((*it)->reserved != -1 && (*it)->reserved != e_id) continue;
                if ((*it)->type <= jr.end && (*it)->type >= jr.start)
                    return *it;
            }
        }
        return nullptr;
    }

    static void cleanup() {
        for (auto& kv : jobs) {
            auto it = kv.second.begin();
            while (it != kv.second.end()) {
                if ((*it)->isComplete) {
                    it = kv.second.erase(it);
                } else {
                    it++;
                }
            }
        }
        auto it = jobs.begin();
        while (it != jobs.end()) {
            if (it->second.empty()) {
                it = jobs.erase(it);
            } else {
                it++;
            }
        }
    }
};

struct WorkInput {
    Time dt;
};

typedef std::function<bool(const std::shared_ptr<Job>&, WorkInput input)>
    JobHandlerFn;

struct JobHandler {
    std::unordered_map<int, JobHandlerFn> job_mapping;

    JobHandler() {}

    void registerJobHandler(JobType jt, JobHandlerFn func) {
        job_mapping[(int)jt] = func;
    }

    void handle(const std::shared_ptr<Job>& j, const WorkInput& input) {
        if (job_mapping.find((int)j->type) == job_mapping.end()) {
            log_warn("Got job of type {} but dont have a handler for it",
                     jobTypeToString(j->type));
            return;
        }
        job_mapping[(int)j->type](j, input);
    }
};
