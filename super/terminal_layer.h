
#pragma once

#include "../vendor/supermarket-engine/engine/camera.h"
#include "../vendor/supermarket-engine/engine/layer.h"
#include "../vendor/supermarket-engine/engine/pch.hpp"
#include "../vendor/supermarket-engine/engine/time.h"
#include "../vendor/supermarket-engine/engine/ui.h"
#include "global.h"

struct TerminalLayer : public Layer {
    const glm::vec2 camTopLeft = {35.f, 19.5f};
    const glm::vec2 camBottomRight = {35.f, -18.f};
    glm::vec4 rect = glm::vec4{200.f, 1000.f, 1500.f, 200.f};
    IUI::uuid drawer_uuid = IUI::MK_UUID(id);
    IUI::uuid command_field_id = IUI::MK_UUID(id);
    std::wstring commandContent = L"test";
    float drawerPctOpen = 0.f;
    std::shared_ptr<IUI::UIContext> uicontext;

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

        uicontext.reset(new IUI::UIContext());
        uicontext->init();
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

    std::array<glm::vec2, 2> getPositionSizeForUIRect(glm::vec4 uirect) {
        glm::vec2 position = convertUIPos(glm::vec2{uirect.x, uirect.y});
        glm::vec2 size = convertUIPos(glm::vec2{uirect.z, uirect.w});
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
        {
            using namespace IUI;
            uicontext->begin(terminalCameraController);

            float h1_fs = 64.f;

            auto drawer_location = getPositionSizeForUIRect({0, 0, WIN_W, 400});

            if (drawer(drawer_uuid,
                       WidgetConfig({
                           .color = blue,
                           .position = drawer_location[0],
                           .size = drawer_location[1],
                       }),
                       &drawerPctOpen)) {
                auto textConfig = WidgetConfig({
                    .color = glm::vec4{0.2, 0.7f, 0.4f, 1.0f},
                    .position = convertUIPos({0, h1_fs + 1.f}),
                    .size = glm::vec2{h1_fs, h1_fs},
                    .text = "Terminal",
                    .flipTextY = true,
                });
                text(MK_UUID(id), textConfig);

                uicontext->kbFocusID = command_field_id;
                auto cfsize = glm::vec2{drawer_location[1].x, h1_fs};
                auto commandFieldConfig = WidgetConfig({
                    .color = glm::vec4{0.4f},
                    .flipTextY = true,
                    .position =
                        glm::vec2{0.f, drawer_location[0].y +
                                           (drawer_location[1].y / 2.f)},
                    .size = cfsize,
                });
                if (commandfield(command_field_id, commandFieldConfig,
                                 commandContent)) {
                    log_info("command field: {}",
                             EDITOR_COMMANDS.command_history.back());
                }

            }  // end drawer
            uicontext->end();
        }
        Renderer::end();
    }

    bool onKeyPressed(KeyPressedEvent event) {
        if (isMinimized == false &&
            event.keycode == Key::getMapping("Exit Debugger")) {
            isMinimized = true;
            return true;
        }
        if (event.keycode == Key::getMapping("Toggle Debugger")) {
            isMinimized = !isMinimized;
            drawerPctOpen = 0.f;
            return true;
        }

        if (isMinimized) {
            return false;
        }

        // TODO is there a way for us to not have to do this?
        // or make it required so you cant even start without it accidentally
        if (uicontext->processKeyPressEvent(event)) {
            return true;
        }
        return false;
    }

    virtual void onEvent(Event& event) override {
        EventDispatcher dispatcher(event);
        terminalCameraController->onEvent(event);
        dispatcher.dispatch<KeyPressedEvent>(std::bind(
            &TerminalLayer::onKeyPressed, this, std::placeholders::_1));
        if (isMinimized) return;
        dispatcher.dispatch<CharPressedEvent>(uicontext->getCharPressHandler());
    }
};

