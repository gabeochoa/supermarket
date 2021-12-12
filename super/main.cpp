

#include "../engine/app.h"
#include "../engine/input.h"
#include "../engine/pch.hpp"
#include "custom_fmt.h"
#include "entities.h"
#include "job.h"
#include "util.h"

// TODO at some point we should have some way to send this to app
constexpr int WIN_W = 1920;
constexpr int WIN_H = 1080;
constexpr float WIN_RATIO = (WIN_W * 1.f) / WIN_H;
constexpr bool IS_DEBUG = true;

static std::shared_ptr<OrthoCameraController> cameraController;

// Requires access to the camera and entitites
#include "debug_layers.h"

struct SuperLayer : public Layer {
    SuperLayer() : Layer("Supermarket") {
        cameraController.reset(new OrthoCameraController(WIN_RATIO, true));

        Renderer::addTexture("./resources/box.png", 2.f);
        Renderer::addTexture("./resources/shelf.png", 2.f);
        Renderer::addTexture("./resources/face.png");
        Renderer::addTexture("./resources/screen.png");

        ////////////////////////////////////////////////////////

        // auto billy = std::make_shared<Billboard>(
        // glm::vec2{0.f, 0.f}, glm::vec2{1.f, 1.f}, 45.f,
        // glm::vec4{0.0f, 1.0f, 1.0f, 1.0f}, "face");
        // entities.push_back(billy);

        auto storage = std::make_shared<Storage>(
            glm::vec2{1.f, 1.f}, glm::vec2{1.f, 1.f}, 0.f,
            glm::vec4{1.0f, 1.0f, 1.0f, 1.0f}, "box");
        storage->contents.addItem(0, 1);
        storage->contents.addItem(1, 6);
        storage->contents.addItem(2, 7);
        storage->contents.addItem(3, 9);
        entities.push_back(storage);

        for (int i = 0; i < 5; i++) {
            for (int j = 0; j < 10; j += 2) {
                auto shelf2 = std::make_shared<Shelf>(
                    glm::vec2{1.f + i, -3.f + j},  //
                    glm::vec2{1.f, 1.f}, 0.f,      //
                    glm::vec4{1.0f, 1.0f, 1.0f, 1.0f}, "shelf");
                entities.push_back(shelf2);
            }
        }

        for (int i = 0; i < 1; i++) {
            auto emp = Employee();
            // TODO eventually get a texture
            // and fix colored textures
            emp.textureName = "__INVALID__";
            emp.color = gen_rand_vec4(0.3f, 1.0f);
            emp.color.w = 1.f;
            emp.size = {0.6f, 0.6f};
            entities.push_back(std::make_shared<Employee>(emp));
        }
    }

    virtual ~SuperLayer() {}
    virtual void onAttach() override {}
    virtual void onDetach() override {}

    virtual void render() {
        prof(__PROFILE_FUNC__);
        Renderer::resetStats();
        Renderer::begin(cameraController->camera);
        for (auto& entity : entities) {
            entity->render();
        }
        Renderer::end();
        Renderer::calculateEndFrameStats();
    }

    virtual void onUpdate(Time dt) override {
        log_trace(fmt::format("{:.2}s ({:.2} ms) ", dt.s(), dt.ms()));
        prof(__PROFILE_FUNC__);

        cameraController->onUpdate(dt);
        for (auto& entity : entities) {
            entity->onUpdate(dt);
        }

        // if (JobQueue::numOfJobsWithType(JobType::IdleWalk) < 5) {
        // JobQueue::addJob(
        // JobType::IdleWalk,
        // std::make_shared<Job>(
        // Job({.type = JobType::IdleWalk,
        // .endPosition = glm::circularRand<float>(5.f)})));
        // }
        // Cleanup all completed jobs
        JobQueue::cleanup();

        render();
    }

    bool onMouseButtonPressed(Mouse::MouseButtonPressedEvent& e) {
        auto mouse = Input::getMousePosition();
        glm::vec4 viewport = {0, 0, WIN_W, WIN_H};
        glm::vec3 mouseInWorld =
            glm::unProject(glm::vec3{mouse.x, WIN_H - mouse.y, 0.f},
                           cameraController->camera.view,
                           cameraController->camera.projection, viewport);

        // TODO allow people to remap their mouse buttons?
        if (e.GetMouseButton() == Mouse::MouseCode::ButtonLeft) {
            log_info(fmt::format("{} {}", mouse, mouseInWorld));
            JobQueue::addJob(
                JobType::DirectedWalk,
                std::make_shared<Job>(Job({.type = JobType::DirectedWalk,
                                           .endPosition = mouseInWorld})));
        }
        if (e.GetMouseButton() == Mouse::MouseCode::ButtonRight) {
            for (auto& entity : entities) {
                auto storage = dynamic_pointer_cast<Storage>(entity);
                if (storage) {
                    // TODO for now just keep queue jobs until we are empty
                    if (!storage->contents.empty() &&
                        JobQueue::numOfJobsWithType(JobType::Fill) <
                            (int)storage->contents.size()) {
                        // TODO getting random shelf probably not the best
                        // idea.. .
                        Job j = {
                            .type = JobType::Fill,
                            .startPosition = storage->position,
                            .endPosition =
                                Storage::getRandomStorage<Shelf>()->position,
                            .itemID = storage->contents.rbegin()->first,
                            .itemAmount = storage->contents.rbegin()->second,
                        };
                        JobQueue::addJob(JobType::Fill,
                                         std::make_shared<Job>(j));
                    }
                }
            }
        }
        return false;
    }

    virtual void onEvent(Event& event) override {
        log_trace(event.toString());
        cameraController->onEvent(event);
        EventDispatcher dispatcher(event);
        dispatcher.dispatch<Mouse::MouseButtonPressedEvent>(std::bind(
            &SuperLayer::onMouseButtonPressed, this, std::placeholders::_1));
    }
};

void theta_test() {
    // Walk straight through
    //  - i expect it to just walk around
    {
        auto shelf = std::make_shared<Shelf>(
            glm::vec2{1.f, 0.f}, glm::vec2{1.f, 1.f}, 0.f,
            glm::vec4{1.0f, 1.0f, 1.0f, 1.0f}, "box");
        entities.push_back(shelf);

        auto shelf2 = std::make_shared<Shelf>(
            glm::vec2{3.f, 0.f}, glm::vec2{1.f, 1.f}, 0.f,
            glm::vec4{1.0f, 1.0f, 1.0f, 1.0f}, "box");
        entities.push_back(shelf2);

        glm::vec2 start = {0.f, 0.f};
        glm::vec2 end = {6.f, 0.f};

        auto emp = Employee();
        emp.position = start;
        emp.size = {0.6f, 0.6f};
        entities.push_back(std::make_shared<Employee>(emp));

        LazyTheta t(start, end,
                    std::bind(isWalkable, emp.size, std::placeholders::_1));
        auto result = t.go();
        std::reverse(result.begin(), result.end());
        for (auto i : result) {
            log_info(fmt::format("{}", i));
        }
        M_ASSERT(result.size(), "Path is empty but shouldnt be");
        entities.clear();
    }
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

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;
    theta_test();
    point_collision_test();

    app.reset(App::create({
        .width = 1920,
        .height = 1080,
        .title = "SuperMarket",
        .clearEnabled = true,
    }));

    Layer* super = new SuperLayer();
    App::get().pushLayer(super);

    Layer* profile = new ProfileLayer();
    App::get().pushLayer(profile);

    Layer* entityDebug = new EntityDebugLayer();
    App::get().pushLayer(entityDebug);

    Layer* jobLayer = new JobLayer();
    App::get().pushLayer(jobLayer);

    App::get().run();
    return 0;
}
