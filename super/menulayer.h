
#pragma once

#include "../engine/camera.h"
#include "../engine/layer.h"
#include "../engine/pch.hpp"
#include "../engine/ui.h"
//
#include "entities.h"

struct MenuLayer : public Layer {
    float value = 0.08f;
    std::string content = "";
    const int TYPABLE_START = 32;
    const int TYPABLE_END = 126;

    MenuLayer() : Layer("Supermarket") {
        menuCameraController.reset(
            new OrthoCameraController(WIN_RATIO, true, 10.f, 0.f, 0.f));
        menuCameraController->camera.setPosition(glm::vec3{15.f, 0.f, 0.f});

        Renderer::addTexture("./resources/letters.png");
        int a = 0;
        int b = 0;
        for (int i = TYPABLE_START; i < TYPABLE_END; i++) {
            Renderer::addSubtexture("letters",                //
                                    std::string(1, (char)i),  //
                                    a, b, 16.f, 16.f);
            a++;
            if (a == 20) {
                b++;
                a = 0;
            }
        }
        IUI::init_context();
    }

    virtual ~MenuLayer() {}
    virtual void onAttach() override {}
    virtual void onDetach() override {}

    bool onCharPressed(CharPressedEvent& event) {
        IUI::get()->keychar = static_cast<Key::KeyCode>(event.charcode);
        return false;
    }

    bool onKeyPressed(KeyPressedEvent& event) {
        if (event.keycode == Key::mapping["Esc"]) {
            App::get().running = false;
            return true;
        }
        if (IUI::get()->widgetKeys.count(
                static_cast<Key::KeyCode>(event.keycode)) == 1) {
            IUI::get()->key = static_cast<Key::KeyCode>(event.keycode);
        }
        if (event.keycode == Key::mapping["Widget Mod"]) {
            IUI::get()->mod = static_cast<Key::KeyCode>(event.keycode);
        }
        return false;
    }

    virtual void onUpdate(Time dt) override {
        log_trace("{:.2}s ({:.2} ms) ", dt.s(), dt.ms());
        prof(__PROFILE_FUNC__);

        if (Menu::get().state != Menu::State::Root) {
            return;
        }

        menuCameraController->onUpdate(dt);

        gltInit();
        gltBeginDraw();
        gltColor(1.0f, 1.0f, 1.0f, 1.0f);
        GLTtext* text = gltCreateText();
        gltSetText(text, stateToString(Menu::get().state));
        gltDrawText2D(text, 150, 150, 5.f);
        gltEndDraw();
        gltDeleteText(text);
        gltTerminate();

        Renderer::begin(menuCameraController->camera);
        ui_test(dt);
        Renderer::end();
    }

    void ui_test(Time dt) {
        using namespace IUI;
        int item = 0;
        begin(menuCameraController);
        {
            if (button(uuid({0, item++, 0}),
                       WidgetConfig({.position = glm::vec2{0.f, 0.f},
                                     .size = glm::vec2{2.f, 1.f}}))) {
                Menu::get().state = Menu::State::UITest;
            }
        }
        end();
    }

    bool onMouseButtonPressed(Mouse::MouseButtonPressedEvent& e) {
        (void)e;
        // auto mouse = Input::getMousePosition();
        // glm::vec4 viewport = {0, 0, WIN_W, WIN_H};
        // glm::vec3 mouseInWorld =
        // glm::unProject(glm::vec3{mouse.x, WIN_H - mouse.y, 0.f},
        // cameraController->camera.view,
        // cameraController->camera.projection, viewport);
        if (e.GetMouseButton() == Mouse::MouseCode::ButtonRight) {
            Menu::get().state = Menu::State::Game;
        }
        return false;
    }

    virtual void onEvent(Event& event) override {
        // log_warn(event.toString().c_str());
        if (Menu::get().state == Menu::State::Game) return;

        menuCameraController->onEvent(event);
        EventDispatcher dispatcher(event);
        dispatcher.dispatch<Mouse::MouseButtonPressedEvent>(std::bind(
            &MenuLayer::onMouseButtonPressed, this, std::placeholders::_1));
        dispatcher.dispatch<KeyPressedEvent>(
            std::bind(&MenuLayer::onKeyPressed, this, std::placeholders::_1));
        dispatcher.dispatch<CharPressedEvent>(
            std::bind(&MenuLayer::onCharPressed, this, std::placeholders::_1));
    }
};
