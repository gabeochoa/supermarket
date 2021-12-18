
#pragma once

#include "../engine/camera.h"
#include "../engine/layer.h"
#include "../engine/pch.hpp"
#include "../engine/ui.h"
//
#include "entities.h"

struct UITestLayer : public Layer {
    float value = 0.08f;
    std::string content = "";
    const int TYPABLE_START = 32;
    const int TYPABLE_END = 126;

    UITestLayer() : Layer("UI Test") {
        Menu::get().state = Menu::State::UITest;

        uiTestCameraController.reset(
            new OrthoCameraController(WIN_RATIO, 10.f, 0.f, 0.f));
        uiTestCameraController->camera.setPosition(glm::vec3{15.f, 0.f, 0.f});

        IUI::init_context();
    }

    virtual ~UITestLayer() {}
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
        if (IUI::get()->textfieldMod.count(
                static_cast<Key::KeyCode>(event.keycode)) == 1) {
            IUI::get()->modchar = static_cast<Key::KeyCode>(event.keycode);
        }
        return false;
    }

    virtual void onUpdate(Time dt) override {
        log_trace("{:.2}s ({:.2} ms) ", dt.s(), dt.ms());
        prof(__PROFILE_FUNC__);

        if (Menu::get().state != Menu::State::UITest) {
            return;
        }
        uiTestCameraController->onUpdate(dt);

        gltInit();
        gltBeginDraw();
        gltColor(1.0f, 1.0f, 1.0f, 1.0f);
        GLTtext* text = gltCreateText();
        gltSetText(text, stateToString(Menu::get().state));
        gltDrawText2D(text, 150, 150, 5.f);
        gltEndDraw();
        gltDeleteText(text);
        gltTerminate();

        Renderer::begin(uiTestCameraController->camera);
        ui_test(dt);
        Renderer::end();
    }

    void ui_test(Time dt) {
        using namespace IUI;
        int item = 0;
        begin(uiTestCameraController);
        {
            if (button(uuid({0, item++, 0}),
                       WidgetConfig({.position = glm::vec2{0.f, 0.f},
                                     .size = glm::vec2{2.f, 1.f}}))) {
                log_info("clicked button");
                Menu::get().state = Menu::State::Root;
            }
            if (button(uuid({0, item++, 0}),
                       WidgetConfig({.position = glm::vec2{3.f, 0.f},
                                     .size = glm::vec2{2.f, 1.f}}))) {
                log_info("clicked button 2");
            }
            if (button(uuid({0, item++, 0}),
                       WidgetConfig({.position = glm::vec2{6.f, 0.f},
                                     .size = glm::vec2{2.f, 1.f}}))) {
                log_info("clicked button 3");
            }

            if (slider(uuid({0, item++, 0}),
                       WidgetConfig({
                           .position = glm::vec2{9.f, 0.f},
                           .size = glm::vec2{1.f, 3.f},
                       }),
                       &value, 0.08f, 0.95f)) {
                // log_info("idk moved slider? ");
            }

            auto upperCaseConfig =
                WidgetConfig({.text = "THE FIVE BOXING WIZARDS JUMP QUICKLY",
                              .color = glm::vec4{1.0, 0.8f, 0.5f, 1.0f},
                              .position = glm::vec2{0.f, -4.f},
                              .size = glm::vec2{1.f, 1.f}});

            auto lowerCaseConfig =
                WidgetConfig({.text = "the five boxing wizards jump quickly",
                              .color = glm::vec4{0.8, 0.3f, 0.7f, 1.0f},
                              .position = glm::vec2{0.f, -6.f},
                              .size = glm::vec2{1.f, 1.f}});

            auto numbersConfig =
                WidgetConfig({.text = "0123456789",
                              .color = glm::vec4{0.7, 0.5f, 0.8f, 1.0f},
                              .position = glm::vec2{0.f, -8.f},
                              .size = glm::vec2{1.f, 1.f}});

            auto extrasConfig =
                WidgetConfig({.text = " !\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~",
                              .color = glm::vec4{0.2, 0.7f, 0.4f, 1.0f},
                              .position = glm::vec2{0.f, -10.f},
                              .size = glm::vec2{1.f, 1.f}});

            text(uuid({0, item++, 0}), upperCaseConfig);
            text(uuid({0, item++, 0}), lowerCaseConfig);
            text(uuid({0, item++, 0}), numbersConfig);
            text(uuid({0, item++, 0}), extrasConfig);

            if (textfield(uuid({0, item++, 0}), glm::vec2{2.f, 2.f},
                          glm::vec2{3.f, 1.f}, content)) {
                log_info("{}", content);
            }
        }

        end();
    }

    bool onMouseButtonPressed(Mouse::MouseButtonPressedEvent& e) {
        (void)e;
        if (e.GetMouseButton() == Mouse::MouseCode::ButtonRight) {
            Menu::get().state = Menu::State::Game;
        }
        return false;
    }

    virtual void onEvent(Event& event) override {
        // log_warn(event.toString().c_str());
        if (Menu::get().state == Menu::State::Game) return;

        uiTestCameraController->onEvent(event);
        EventDispatcher dispatcher(event);
        dispatcher.dispatch<Mouse::MouseButtonPressedEvent>(std::bind(
            &UITestLayer::onMouseButtonPressed, this, std::placeholders::_1));
        dispatcher.dispatch<KeyPressedEvent>(
            std::bind(&UITestLayer::onKeyPressed, this, std::placeholders::_1));
        dispatcher.dispatch<CharPressedEvent>(std::bind(
            &UITestLayer::onCharPressed, this, std::placeholders::_1));
    }
};
