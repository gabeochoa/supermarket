
#pragma once

#include "../vendor/supermarket-engine/engine/camera.h"
#include "../vendor/supermarket-engine/engine/layer.h"
#include "../vendor/supermarket-engine/engine/pch.hpp"
#include "../vendor/supermarket-engine/engine/renderer.h"
//
#include "global.h"
//
#include "customer.h"
#include "drag_area.h"
#include "employee.h"
#include "entities.h"
#include "job.h"
#include "menu.h"
#include "navmesh.h"

//

enum FurnitureTool {
    SELECTION = 0,
    STORAGE = 1,
    SHELF = 2,
    DELETE = 3,
};

constexpr inline const char* furnitureToolToTexture(FurnitureTool id) {
    switch (id) {
        // TODO adda like a red x texture
        case FurnitureTool::DELETE:
        case FurnitureTool::SELECTION:
            return "white";
        case FurnitureTool::STORAGE:
            return "box";
        case FurnitureTool::SHELF:
            return "shelf";
    }
}

struct GameUILayer : public Layer {
    const glm::vec2 camTopLeft = {35.f, 19.5f};
    const glm::vec2 camBottomRight = {35.f, -18.f};
    glm::vec4 rect = glm::vec4{200.f, 1000.f, 1500.f, 200.f};
    std::shared_ptr<IUI::UIContext> uicontext;
    bool dropdownState = false;
    int dropdownIndex = 0;
    FurnitureTool selectedTool;
    ItemManager* itemManager;
    std::shared_ptr<OrthoCameraController> gameUICameraController;

    const float H1_FS = 64.f;
    const float P_FS = 32.f;

    GameUILayer() : Layer("Game UI") {
        isMinimized = true;
        gameUICameraController.reset(new OrthoCameraController(WIN_RATIO));
        GLOBALS.set<OrthoCameraController>("gameUICameraController",
                                           gameUICameraController.get());

        gameUICameraController->setZoomLevel(20.f);
        gameUICameraController->camera.setViewport({0, 0, WIN_W, WIN_H});
        gameUICameraController->movementEnabled = false;
        gameUICameraController->rotationEnabled = false;
        gameUICameraController->zoomEnabled = false;
        gameUICameraController->resizeEnabled = false;

        uicontext.reset(new IUI::UIContext());
        uicontext->init();
        GLOBALS.set("selected_tool", &selectedTool);

        itemManager = GLOBALS.get_ptr<ItemManager>("item_manager");
    }

    virtual ~GameUILayer() {}
    virtual void onAttach() override {}
    virtual void onDetach() override {}

    glm::vec2 convertUIPos(glm::vec2 pos, bool flipy = true) {
        auto y = flipy ? WIN_H - pos.y : pos.y;
        return screenToWorld(glm::vec3{pos.x, y, 0.f},
                             gameUICameraController->camera.view,
                             gameUICameraController->camera.projection,
                             gameUICameraController->camera.viewport);
    }

    std::array<glm::vec2, 2> getPositionSizeForUIRect(glm::vec4 uirect) {
        glm::vec2 position = convertUIPos(glm::vec2{uirect.x, uirect.y});
        glm::vec2 size = convertUIPos(glm::vec2{uirect.z, uirect.w});
        return std::array{
            position + (size * 0.5f),
            size,
        };
    }

    ItemGroup getTotalInventory() {
        ItemGroup ig;
        EntityHelper::forEach<Storable>([&](auto s) {
            for (auto kv : s->contents) {
                ig.addItem(kv.first, kv.second);
            }
        });

        EntityHelper::forEach<Employee>([&](auto emp) {
            for (auto kv : emp->inventory) {
                ig.addItem(kv.first, kv.second);
            }
        });
        return ig;
    }

    void render_item_row(int index, int item_id, int amountInInventory) {
        using namespace IUI;

        std::shared_ptr<Item> item = itemManager->get_ptr(item_id);

        // ${price} {name:<{length}}: {amountInInventory}
        auto formatstr = "${0:.2f} {1}({2})";
        auto str = fmt::format(formatstr,         //
                               item->price,       //
                               item->name,        //
                               amountInInventory  //
        );

        auto plusButtonPosition_raw = glm::vec2{P_FS, 200.f + (P_FS * index)};
        auto plusButtonConfig = WidgetConfig({
            .position = convertUIPos(plusButtonPosition_raw),
            .color = glm::vec4{0.2, 0.7f, 0.4f, 1.0f},
            .flipTextY = true,
            .size = glm::vec2{P_FS, P_FS},
            .text = "+",
        });

        if (button_with_label(MK_UUID_LOOP(id, index), plusButtonConfig)) {
            // TODO should we have a max price
            float MAX_ITEM_PRICE = 10.f;
            itemManager->update_price(item_id,
                                      fmin(item->price + 0.1, MAX_ITEM_PRICE));
        }

        auto minusButtonConfig = WidgetConfig({
            .position = convertUIPos(plusButtonPosition_raw +
                                     glm::vec2{P_FS + P_FS / 2, 0.f}),
            .color = glm::vec4{0.2, 0.7f, 0.4f, 1.0f},
            .flipTextY = true,
            .size = glm::vec2{P_FS, P_FS},
            .text = "-",
        });

        if (button_with_label(MK_UUID_LOOP(id, index), minusButtonConfig)) {
            // TODO where should this live
            float MIN_ITEM_PRICE = 0.f;
            itemManager->update_price(item_id,
                                      fmax(item->price - 0.1, MIN_ITEM_PRICE));
        }

        text(MK_UUID_LOOP(id, index),
             WidgetConfig({
                 .position = convertUIPos(plusButtonPosition_raw +
                                          glm::vec2{P_FS * 3, P_FS}),
                 .color = glm::vec4{0.3, 0.5, 0.2, 1.f},
                 .flipTextY = true,
                 .size = glm::vec2{P_FS, P_FS},
                 .text = str,
             }));
    }

    void render() {
        gameUICameraController->camera.setProjection(0.f, WIN_W, WIN_H, 0.f);
        Renderer::begin(gameUICameraController->camera);
        using namespace IUI;
        uicontext->begin(gameUICameraController);

        std::vector<std::function<bool(uuid)>> children;

        auto window_location = getPositionSizeForUIRect({0, 100, 500, 500});
        uuid window_id = MK_UUID(id);
        if (window(window_id, WidgetConfig({
                                  .color = blue,
                                  .position = window_location[0],
                                  .size = window_location[1],
                              })  //
                   )) {
            auto textConfig = WidgetConfig({
                .color = glm::vec4{0.2, 0.7f, 0.4f, 1.0f},
                .position = convertUIPos({0, 100.f + H1_FS + 1.f}),
                .size = glm::vec2{H1_FS, H1_FS},
                .text = "Inventory",
                .flipTextY = true,
            });
            text(MK_UUID(id), textConfig);

            // TODO replace with list view when exists
            int i = 0;
            for (auto kv : getTotalInventory()) {
                render_item_row(i++, kv.first, kv.second);
            }
        }
        std::vector<WidgetConfig> dropdownConfigs;
        dropdownConfigs.push_back(IUI::WidgetConfig({.text = "Selection"}));
        dropdownConfigs.push_back(IUI::WidgetConfig({.text = "Storage"}));
        dropdownConfigs.push_back(IUI::WidgetConfig({.text = "Shelf"}));
        dropdownConfigs.push_back(IUI::WidgetConfig({.text = "Delete"}));

        WidgetConfig dropdownMain = IUI::WidgetConfig({
            .color = glm::vec4{0.3f, 0.9f, 0.5f, 1.f},         //
            .position = convertUIPos(glm::vec2{P_FS, 500.f}),  //
            .size = glm::vec2{H1_FS * 3, H1_FS},               //
            .transparent = false,                              //
            .flipTextY = true,
        });

        if (dropdown(MK_UUID(id), dropdownMain, dropdownConfigs, &dropdownState,
                     &dropdownIndex)) {
        }

        uicontext->end();
        Renderer::end();

        auto tool = GLOBALS.update<FurnitureTool>(
            "selected_tool", static_cast<FurnitureTool>(dropdownIndex));
        if (tool != FurnitureTool::SELECTION) {
            GLOBALS.get_ptr<DragArea>("drag_area")
                ->place(dropdownIndex, furnitureToolToTexture(tool));
        }
    }

    virtual void onUpdate(Time dt) override {
        if (Menu::get().state != Menu::State::Game) return;

        log_trace("{:.2}s ({:.2} ms) ", dt.s(), dt.ms());
        prof give_me_a_name(__PROFILE_FUNC__);  //

        gameUICameraController->onUpdate(dt);
        render();  // draw everything
    }

    virtual void onEvent(Event& event) override {
        if (Menu::get().state != Menu::State::Game) return;
        gameUICameraController->onEvent(event);
    }
};

struct SuperLayer : public Layer {
    std::shared_ptr<DragArea> dragArea;
    glm::vec4 viewport = {0, 0, WIN_W, WIN_H};
    std::shared_ptr<OrthoCameraController> cameraController;

    SuperLayer() : Layer("Supermarket") {
        isMinimized = true;

        cameraController.reset(new OrthoCameraController(WIN_RATIO));
        GLOBALS.set<OrthoCameraController>("superCameraController",
                                           cameraController.get());

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

        // NOTE: Superlayer owns this static so its okay to use directly
        GLOBALS.set("navmesh", &__navmesh___DO_NOT_USE_DIRECTLY);

        for (int i = 0; i < 5; i++) {
            for (int j = 0; j < 10; j += 2) {
                if (i == 0 && j == 4) {
                    continue;
                }
                auto shelf2 = std::make_shared<Shelf>(
                    glm::vec2{1.f + i, -3.f + j},  //
                    glm::vec2{1.f, 1.f}, 0.f,      //
                    glm::vec4{1.0f, 1.0f, 1.0f, 1.0f}, "shelf");
                EntityHelper::addEntity(shelf2);
            }
        }

        auto storage = std::make_shared<Storage>(
            glm::vec2{1.f, 1.f}, glm::vec2{1.f, 1.f}, 0.f,
            glm::vec4{1.0f, 1.0f, 1.0f, 1.0f}, "box");
        storage->contents.addItem(0, 1);
        storage->contents.addItem(1, 6);
        storage->contents.addItem(2, 7);
        storage->contents.addItem(3, 9);
        EntityHelper::addEntity(storage);

        const int num_people_sprites = 3;
        std::array<std::string, num_people_sprites> peopleSprites = {
            "player",
            "player2",
            "player3",
        };

        for (int i = 0; i < 1; i++) {
            auto emp = Employee();
            emp.color = gen_rand_vec4(0.3f, 1.0f);
            emp.color.w = 1.f;
            emp.size = {0.6f, 0.6f};
            emp.textureName = peopleSprites[0];
            EntityHelper::addEntity(std::make_shared<Employee>(emp));
        }

        for (int i = 0; i < 1; i++) {
            auto cust = Customer();
            cust.color = gen_rand_vec4(0.3f, 1.0f);
            cust.color.w = 1.f;
            cust.size = {0.6f, 0.6f};
            cust.textureName =
                peopleSprites[(i % (num_people_sprites - 1)) + 1];
            EntityHelper::addEntity(std::make_shared<Customer>(cust));
        }

        dragArea.reset(new DragArea(glm::vec2{0.f}, glm::vec2{0.f}, 0.f,
                                    glm::vec4{0.75f}));
        GLOBALS.set("drag_area", dragArea.get());
    }

    virtual ~SuperLayer() {}
    virtual void onAttach() override {}
    virtual void onDetach() override {}

    void child_updates(Time dt) {
        if (GLOBALS.get<bool>("terminal_closed")) {
            cameraController->onUpdate(dt);
        }

        EntityHelper::forEachEntity([&](auto entity) {  //
            entity->onUpdate(dt);
        });

        dragArea->onUpdate(dt);
    }

    void render() {
        Renderer::begin(cameraController->camera);
        // should go underneath entities also
        dragArea->render_selected();

        EntityHelper::forEachEntity([&](auto entity) {  //
            entity->render();
        });

        // render above items
        dragArea->render();

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

        if (JobQueue::numOfJobsWithType(JobType::IdleShop) < 5) {
            JobQueue::addJob(
                JobType::IdleShop,
                std::make_shared<Job>(
                    Job({.type = JobType::IdleShop,
                         .endPosition = glm::circularRand<float>(5.f)})));
        }

        EntityHelper::forEach<Storage>([](auto storage) {
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
                        EntityHelper::getRandomEntity<Shelf>()->position,
                    .itemID = storage->contents.rbegin()->first,
                    .itemAmount = storage->contents.rbegin()->second,
                };
                JobQueue::addJob(JobType::Fill, std::make_shared<Job>(j));
            }
        });
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
            dragArea->onDragStart(mouseInWorld);
        }
        if (e.GetMouseButton() == Mouse::MouseCode::ButtonRight) {
            JobQueue::addJob(
                JobType::DirectedWalk,
                std::make_shared<Job>(Job({.type = JobType::DirectedWalk,
                                           .endPosition = mouseInWorld})));
        }
        return false;
    }
    bool onMouseMoved(Mouse::MouseMovedEvent&) {
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
        if (event.keycode == Key::getMapping("Esc")) {
            Menu::get().state = Menu::State::Root;
            return true;
        }
        return false;
    }

    virtual void onUpdate(Time dt) override {
        if (Menu::get().state != Menu::State::Game) return;

        log_trace("{:.2}s ({:.2} ms) ", dt.s(), dt.ms());
        prof give_me_a_name(__PROFILE_FUNC__);

        child_updates(dt);        // move things around
        render();                 // draw everything
        fillJobQueue();           // add more jobs if needed
        JobQueue::cleanup();      // Cleanup all completed jobs
        EntityHelper::cleanup();  // Cleanup dead entities
    }

    virtual void onEvent(Event& event) override {
        if (Menu::get().state != Menu::State::Game) return;
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

