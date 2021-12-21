
#pragma once

#include "../engine/app.h"
#include "../engine/camera.h"
#include "../engine/font.h"
#include "../engine/layer.h"
#include "../engine/pch.hpp"
#include "../engine/ui.h"
//
#include "global.h"
//
#include "entities.h"
#include "menu.h"

struct UITestLayer : public Layer {
    float value = 0.08f;
    std::string content = "";
    const int TYPABLE_START = 32;
    const int TYPABLE_END = 126;
    bool checkbox_state = false;
    int dropdownIndex = 0;
    bool dropdownState = false;

    std::shared_ptr<Billboard> billy;

    UITestLayer() : Layer("UI Test") {
        Menu::get().state = Menu::State::UITest;

        uiTestCameraController.reset(
            new OrthoCameraController(WIN_RATIO, 10.f, 5.f, 0.f));
        uiTestCameraController->camera.setPosition(glm::vec3{15.f, 0.f, 0.f});
        uiTestCameraController->camera.setViewport(
            glm::vec4{0, 0, WIN_W, WIN_H});

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
        ui_test();
        Renderer::end();
    }

    void ui_test() {
        using namespace IUI;
        int item = 0;
        {
            // uses RAII to handle begin()/end() automatically
            UIFrame BandE(uiTestCameraController);

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
                              .font = "Roboto-Regular",
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

            if (textfield(uuid({0, item++, 0}),
                          WidgetConfig({.position = glm::vec2{2.f, 2.f},
                                        .size = glm::vec2{3.f, 1.f}}),
                          content)) {
                log_info("{}", content);
            }

            auto textConfig = IUI::WidgetConfig({
                .text = "Tap to continue",
                .position = glm::vec2{0.f, 0.5f},          //
                .size = glm::vec2{0.5f, 0.5f},             //
                .color = glm::vec4{1.f, 1.0f, 1.0f, 1.f},  //
            });

            if (IUI::button_with_label(
                    IUI::uuid({0, item++, 0}),
                    IUI::WidgetConfig({
                        .position = glm::vec2{12.f, 1.f},           //
                        .size = glm::vec2{8.f, 1.f},                //
                        .transparent = false,                       //
                        .color = glm::vec4{0.3f, 0.9f, 0.5f, 1.f},  //
                        .child = &textConfig,                       //
                        .text = ""                                  //
                    })                                              //
                    )) {
                Menu::get().state = Menu::State::UITest;
            }

            if (IUI::checkbox(                  //
                    IUI::uuid({0, item++, 0}),  //
                    IUI::WidgetConfig({
                        .position = glm::vec2{4.f, 4.f},            //
                        .size = glm::vec2{1.f, 1.f},                //
                        .transparent = false,                       //
                        .color = glm::vec4{0.6f, 0.3f, 0.3f, 1.f},  //
                    }),                                             //
                    &checkbox_state)) {
                log_info("checkbox changed {}", checkbox_state);
            }

            {
                std::vector<WidgetConfig> dropdownConfigs;
                dropdownConfigs.push_back(
                    IUI::WidgetConfig({.text = "option1"}));
                dropdownConfigs.push_back(
                    IUI::WidgetConfig({.text = "option2"}));

                WidgetConfig dropdownMain = IUI::WidgetConfig({
                    .position = glm::vec2{12.f, -1.f},          //
                    .size = glm::vec2{8.f, 1.f},                //
                    .transparent = false,                       //
                    .color = glm::vec4{0.3f, 0.9f, 0.5f, 1.f},  //
                    .child = &textConfig,                       //
                    .text = ""                                  //
                });

                if (IUI::dropdown(IUI::uuid({0, item++, 0}), dropdownMain,
                                  dropdownConfigs, &dropdownState,
                                  &dropdownIndex)) {
                    dropdownState = !dropdownState;
                }
            }
            // end with BandE
        }
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
        if (Menu::get().state != Menu::State::UITest) return;

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
