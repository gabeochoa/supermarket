
#pragma once

#include "../engine/camera.h"
#include "../engine/layer.h"
#include "../engine/pch.hpp"
#include "../engine/time.h"
#include "../engine/ui.h"
#include "global.h"

struct TerminalLayer : public Layer {
    const glm::vec2 camTopLeft = {35.f, 19.5f};
    const glm::vec2 camBottomRight = {35.f, -18.f};
    glm::vec4 rect = glm::vec4{200.f, 1000.f, 1500.f, 200.f};
    // TODO ive just realized that UUID 000 is basically every layer
    // probably instead of using parent we should use layer as the first id...
    IUI::uuid drawer_uuid = IUI::uuid({id, -1, 0});
    IUI::uuid command_field_id = IUI::uuid({id, -2, 0});
    std::wstring commandContent;
    float drawerPctOpen = 0.f;

    TerminalLayer() : Layer("Debug Terminal") {
        isMinimized = true;
        GLOBALS.set<bool>("terminal_closed", &isMinimized);

        terminalCameraController.reset(new OrthoCameraController(WIN_RATIO));
        terminalCameraController->setZoomLevel(20.f);
        terminalCameraController->camera.setViewport({0, 0, WIN_W, WIN_H});
        terminalCameraController->movementEnabled = false;
        terminalCameraController->rotationEnabled = false;
        terminalCameraController->zoomEnabled = false;
        terminalCameraController->resizeEnabled = false;
    }

    virtual ~TerminalLayer() {}
    virtual void onAttach() override {}
    virtual void onDetach() override {}

    glm::vec2 convertUIPos(glm::vec2 pos, bool flipy = true) {
        auto y = flipy ? WIN_H - pos.y : pos.y;
        return screenToWorld(glm::vec3{pos.x, y, 0.f},
                             terminalCameraController->camera.view,
                             terminalCameraController->camera.projection,
                             terminalCameraController->camera.viewport);
    }

    std::array<glm::vec2, 2> getPositionSizeForUIRect(glm::vec4 rect) {
        glm::vec2 position = convertUIPos(glm::vec2{rect.x, rect.y});
        glm::vec2 size = convertUIPos(glm::vec2{rect.z, rect.w});
        return std::array{
            position + (size * 0.5f),
            size,
        };
    }

    virtual void onUpdate(Time dt) override {
        if (isMinimized) {
            return;
        }

        log_trace("{:.2}s ({:.2} ms) ", dt.s(), dt.ms());
        prof give_me_a_name(__PROFILE_FUNC__);  //
        terminalCameraController->onUpdate(dt);
        terminalCameraController->camera.setProjection(0.f, WIN_W, WIN_H, 0.f);

        Renderer::begin(terminalCameraController->camera);

        using namespace IUI;
        UIFrame BandE(terminalCameraController);
        IUI::get()->kbFocusID = command_field_id;

        float h1_fs = 64.f;

        auto drawer_location = getPositionSizeForUIRect({0, 0, WIN_W, 400});

        if (drawer(drawer_uuid,
                   WidgetConfig({
                       .color = blue,
                       .position = drawer_location[0],
                       .size = drawer_location[1],
                   }),
                   &drawerPctOpen)) {
            int index = 0;

            auto textConfig = WidgetConfig({
                .color = glm::vec4{0.2, 0.7f, 0.4f, 1.0f},
                .position = convertUIPos({0, h1_fs + 1.f}),
                .size = glm::vec2{h1_fs, h1_fs},
                .text = "Terminal",
                .flipTextY = true,
            });
            text(uuid({id, drawer_uuid.item, index++}), textConfig);

            auto cfsize = glm::vec2{drawer_location[1].x, h1_fs};
            auto commandFieldConfig = WidgetConfig({
                .color = glm::vec4{0.4f},
                .flipTextY = true,
                .position = glm::vec2{0.f, drawer_location[0].y +
                                               (drawer_location[1].y / 2.f)},
                .size = cfsize,
            });
            commandfield(command_field_id, commandFieldConfig, commandContent);

        }  // end drawer
        Renderer::end();
    }

    bool onKeyPressed(KeyPressedEvent event) {
        if (event.keycode == Key::mapping["Exit Debugger"]) {
            isMinimized = false;
            return false;
        }
        if (event.keycode == Key::mapping["Toggle Debugger"]) {
            isMinimized = !isMinimized;
            drawerPctOpen = 0.f;
        }
        // TODO is there a way for us to not have to do this?
        // or make it required so you cant even start without it accidentally
        if (IUI::get()->processKeyPressEvent(event)) {
            return true;
        }
        // TODO since we have no way to have IUI return true (yet!)
        // eat all keypresses while we are open
        if (!isMinimized) return true;
        return false;
    }

    virtual void onEvent(Event& event) override {
        EventDispatcher dispatcher(event);
        dispatcher.dispatch<KeyPressedEvent>(std::bind(
            &TerminalLayer::onKeyPressed, this, std::placeholders::_1));
        terminalCameraController->onEvent(event);
        dispatcher.dispatch<CharPressedEvent>(
            IUI::get()->getCharPressHandler());
    }
};

