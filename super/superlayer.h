
#pragma once

#include "../engine/camera.h"
#include "../engine/layer.h"
#include "../engine/pch.hpp"
#include "../engine/renderer.h"
//
#include "global.h"
//
#include "drag_area.h"
#include "entities.h"
#include "job.h"
#include "menu.h"

struct GameUILayer : public Layer {
    const glm::vec2 camTopLeft = {35.f, 19.5f};
    const glm::vec2 camBottomRight = {35.f, -18.f};
    glm::vec2 viewportSize;
    std::shared_ptr<FrameBuffer> frameBuffer;

    GameUILayer() : Layer("Game UI") {
        isMinimized = true;

        viewportSize = glm::vec2{WIN_W, WIN_H};
        gameUICameraController.reset(new OrthoCameraController(WIN_RATIO));
        gameUICameraController->camera.setViewport(
            {0, 0, viewportSize.x, viewportSize.y});
        gameUICameraController->setZoomLevel(20.f);
        // move the camera so 0,0 is top left
        gameUICameraController->camera.setPosition(glm::vec3{camTopLeft, 0.f});
        gameUICameraController->movementEnabled = false;
        gameUICameraController->rotationEnabled = false;
        gameUICameraController->zoomEnabled = false;

        FrameBufferSpec fbSpec({
            .width = WIN_W,
            .height = WIN_H,
        });
        frameBuffer.reset(new OpenGLFrameBuffer(fbSpec));
    }

    virtual ~GameUILayer() {}
    virtual void onAttach() override {}
    virtual void onDetach() override {}

    void render_to_fb() {
        frameBuffer->bind();
        // Renderer::clear([> color <] {0.1f, 0.1f, 0.1f, 1.0f});
        Renderer::begin(gameUICameraController->camera);
        using namespace IUI;
        UIFrame BandE(gameUICameraController);
        int item = 0;

        const glm::vec2 window_top_right = {71.f, 40.f};

        std::vector<std::function<bool(uuid)>> children;

        std::function<bool(uuid)> textFunc = [](uuid id) {
            auto textConfig = WidgetConfig({
                .color = glm::vec4{0.2, 0.7f, 0.4f, 1.0f},
                .position = glm::vec2{0.f, 0.f},
                .size = glm::vec2{1.f, 1.f},
                .text = "With great power comes great responsibility",
            });
            return text(id, textConfig);
        };
        children.push_back(textFunc);

        auto rect = glm::vec4{100.f, 100.f, 200.f, 200.f};

        auto ui_cvt_pos = [&](glm::vec2 pos) {
            return screenToWorld(glm::vec3{pos.x, pos.y, 0.f},
                                 gameUICameraController->camera.view,
                                 gameUICameraController->camera.projection,
                                 gameUICameraController->camera.viewport);
        };

        glm::vec2 window_position = ui_cvt_pos(glm::vec2{rect.x, rect.y});
        glm::vec2 window_size =
            ui_cvt_pos(glm::vec2{rect.z - rect.x, rect.w - rect.y});

        window(uuid({0, item++, 0}),
               WidgetConfig({
                   .color = blue,
                   .position = window_position,
                   .size = window_size,
               }),
               children);

        Renderer::end();
        frameBuffer->unbind();
    }

    void render() {
        render_to_fb();
        // auto textureID = frameBuffer->colorAttachment;
    }

    virtual void onUpdate(Time dt) override {
        if (Menu::get().state != Menu::State::Game) return;

        log_trace("{:.2}s ({:.2} ms) ", dt.s(), dt.ms());
        prof give_me_a_name(__PROFILE_FUNC__);  //

        Renderer::stats.reset();
        Renderer::stats.begin();

        gameUICameraController->onUpdate(dt);
        render();  // draw everything

        //
        Renderer::stats.end();
    }

    bool onWindowResized(WindowResizeEvent& e) {
        // viewportSize = glm::vec2{e.width(), e.height()};
        // frameBuffer->resize(viewportSize);
        return false;
    }

    virtual void onEvent(Event& event) override {
        if (Menu::get().state != Menu::State::Game) return;
        gameUICameraController->onEvent(event);
        EventDispatcher dispatcher(event);
        dispatcher.dispatch<WindowResizeEvent>(std::bind(
            &GameUILayer::onWindowResized, this, std::placeholders::_1));
    }
};

struct SuperLayer : public Layer {
    // TODO should we add some system for "named" entities
    // allowing to store this globally and hide it from the layer
    std::shared_ptr<DragArea> dragArea;
    glm::vec4 viewport = {0, 0, WIN_W, WIN_H};

    SuperLayer() : Layer("Supermarket") {
        isMinimized = true;

        cameraController.reset(new OrthoCameraController(WIN_RATIO));
        cameraController->camera.setViewport(viewport);
        cameraController->rotationEnabled = false;

        Renderer::addTexture("./resources/face.png");
        Renderer::addTexture("./resources/box.png");
        Renderer::addTexture("./resources/shelf.png");
        Renderer::addTexture("./resources/screen.png");

        // 918 × 203 pixels at 16 x 16 with margin 1
        float playerSprite = 16.f;
        Renderer::addTexture("./resources/character_tilesheet.png");
        Renderer::addSubtexture("character_tilesheet", "player", 0, 0,
                                playerSprite, playerSprite);
        Renderer::addSubtexture("character_tilesheet", "player2", 0, 1,
                                playerSprite, playerSprite);
        Renderer::addSubtexture("character_tilesheet", "player3", 1, 1,
                                playerSprite, playerSprite);

        Renderer::addTexture("./resources/item_sheet.png");
        Renderer::addSubtexture("item_sheet", "egg", 0, 0, 16.f, 16.f);
        Renderer::addSubtexture("item_sheet", "milk", 1, 0, 16.f, 16.f);
        Renderer::addSubtexture("item_sheet", "peanutbutter", 2, 0, 16.f, 16.f);
        Renderer::addSubtexture("item_sheet", "pizza", 3, 0, 16.f, 16.f);

        ////////////////////////////////////////////////////////
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

        const int num_people_sprites = 3;
        std::array<std::string, num_people_sprites> peopleSprites = {
            "player",
            "player2",
            "player3",
        };

        for (int i = 0; i < 5; i++) {
            auto emp = Employee();
            emp.color = gen_rand_vec4(0.3f, 1.0f);
            emp.color.w = 1.f;
            emp.size = {0.6f, 0.6f};
            emp.textureName = peopleSprites[i % num_people_sprites];
            entities.push_back(std::make_shared<Employee>(emp));
        }

        dragArea.reset(new DragArea(glm::vec2{0.f}, glm::vec2{0.f}, 0.f,
                                    glm::vec4{0.75f}));
    }

    virtual ~SuperLayer() {}
    virtual void onAttach() override {}
    virtual void onDetach() override {}

    void child_updates(Time dt) {
        cameraController->onUpdate(dt);
        for (auto& entity : entities) {
            entity->onUpdate(dt);
        }
        dragArea->onUpdate(dt);
    }

    void render() {
        Renderer::begin(cameraController->camera);
        // render above any other items
        dragArea->render();

        for (auto& entity : entities) {
            entity->render();
        }
        // should go underneath entities
        dragArea->render_selected();
        Renderer::end();
    }

    void fillJobQueue() {
        if (JobQueue::numOfJobsWithType(JobType::IdleWalk) < 5) {
            JobQueue::addJob(
                JobType::IdleWalk,
                std::make_shared<Job>(
                    Job({.type = JobType::IdleWalk,
                         .endPosition = glm::circularRand<float>(5.f)})));
        }

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
                    JobQueue::addJob(JobType::Fill, std::make_shared<Job>(j));
                }
            }
        }
    }

    glm::vec3 getMouseInWorld() {
        auto mouse = Input::getMousePosition();
        return screenToWorld(glm::vec3{mouse.x, WIN_H - mouse.y, 0.f},
                             cameraController->camera.view,
                             cameraController->camera.projection, viewport);
    }

    bool onMouseButtonPressed(Mouse::MouseButtonPressedEvent& e) {
        glm::vec3 mouseInWorld = getMouseInWorld();

        // TODO allow people to remap their mouse buttons?
        if (e.GetMouseButton() == Mouse::MouseCode::ButtonLeft) {
            dragArea->mouseDragStart = mouseInWorld;
            dragArea->mouseDragEnd = mouseInWorld;
        }
        if (e.GetMouseButton() == Mouse::MouseCode::ButtonRight) {
            JobQueue::addJob(
                JobType::DirectedWalk,
                std::make_shared<Job>(Job({.type = JobType::DirectedWalk,
                                           .endPosition = mouseInWorld})));
        }
        return false;
    }
    bool onMouseMoved(Mouse::MouseMovedEvent& e) {
        glm::vec3 mouseInWorld = getMouseInWorld();

        if (Input::isMouseButtonPressed(Mouse::MouseCode::ButtonLeft)) {
            dragArea->isMouseDragging = true;
            dragArea->mouseDragEnd = mouseInWorld;
        }
        return false;
    }

    bool onMouseButtonReleased(Mouse::MouseButtonReleasedEvent& e) {
        glm::vec3 mouseInWorld = getMouseInWorld();

        if (e.GetMouseButton() == Mouse::MouseCode::ButtonLeft) {
            dragArea->mouseDragEnd = mouseInWorld;
            // TODO should this live in mouseMoved?
            dragArea->isMouseDragging = false;
            dragArea->onDragEnd();
        }
        return false;
    }

    bool onKeyPressed(KeyPressedEvent& event) {
        if (event.keycode == Key::mapping["Esc"]) {
            Menu::get().state = Menu::State::Root;
            return true;
        }
        return false;
    }

    virtual void onUpdate(Time dt) override {
        if (Menu::get().state != Menu::State::Game) return;

        log_trace("{:.2}s ({:.2} ms) ", dt.s(), dt.ms());
        prof(__PROFILE_FUNC__);  //

        Renderer::stats.reset();
        Renderer::stats.begin();
        child_updates(dt);    // move things around
        render();             // draw everything
        fillJobQueue();       // add more jobs if needed
        JobQueue::cleanup();  // Cleanup all completed jobs
        Renderer::stats.end();
    }

    virtual void onEvent(Event& event) override {
        if (Menu::get().state != Menu::State::Game) return;
        // log_warn(event.toString().c_str());
        cameraController->onEvent(event);
        EventDispatcher dispatcher(event);
        dispatcher.dispatch<Mouse::MouseButtonPressedEvent>(std::bind(
            &SuperLayer::onMouseButtonPressed, this, std::placeholders::_1));
        dispatcher.dispatch<Mouse::MouseButtonReleasedEvent>(std::bind(
            &SuperLayer::onMouseButtonReleased, this, std::placeholders::_1));
        dispatcher.dispatch<Mouse::MouseMovedEvent>(
            std::bind(&SuperLayer::onMouseMoved, this, std::placeholders::_1));
        dispatcher.dispatch<KeyPressedEvent>(
            std::bind(&SuperLayer::onKeyPressed, this, std::placeholders::_1));
    }
};

