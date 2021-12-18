
#pragma once

#include "input.h"
#include "pch.hpp"

namespace IUI {

struct uuid {
    int owner;
    int item;
    int index;

    bool operator==(const uuid& other) const {
        return owner == other.owner && item == other.item &&
               index == other.index;
    }

    bool operator<(const uuid& other) const {
        return owner < other.owner && item < other.item && index < other.index;
    }
};

struct UIContext {
    uuid hotID;     // probably about to be touched
    uuid activeID;  // currently being touched

    glm::vec2 mousePosition;
    bool lmouseDown;

    const std::set<Key::KeyCode> widgetKeys = {{
        Key::mapping["Widget Next"],
        Key::mapping["Widget Press"],
        Key::mapping["Value Up"],
        Key::mapping["Value Down"],
    }};
    uuid kbFocusID;
    Key::KeyCode key;
    Key::KeyCode mod;
    uuid lastProcessed;

    const std::set<Key::KeyCode> textfieldMod = {{
        Key::mapping["Text Space"],
        Key::mapping["Text Backspace"],
    }};
    Key::KeyCode keychar;
    Key::KeyCode modchar;

    bool pressed(Key::KeyCode code) {
        bool a = key == code || mod == code;
        if (a) key = Key::KeyCode();
        return a;
    }
};

uuid rootID = uuid({.owner = -1, .item = 0, .index = 0});
uuid fakeID = uuid({.owner = -2, .item = 0, .index = 0});
static std::shared_ptr<UIContext> globalContext;
auto white = glm::vec4{1.0f, 1.0f, 1.0f, 1.0f};
auto red = glm::vec4{1.0f, 0.0f, 0.0f, 1.0f};
auto green = glm::vec4{0.0f, 1.0f, 0.0f, 1.0f};
auto blue = glm::vec4{0.0f, 0.0f, 1.0f, 1.0f};
auto teal = glm::vec4{0.0f, 1.0f, 1.0f, 1.0f};

std::shared_ptr<UIContext> get() {
    if (!globalContext) globalContext.reset(new UIContext());
    return globalContext;
}

void init_context() {
    get()->hotID = rootID;
    get()->activeID = rootID;
    get()->lmouseDown = false;
    get()->mousePosition = Input::getMousePosition();
}

void try_to_grab_kb(uuid id) {
    if (get()->kbFocusID == rootID) {
        get()->kbFocusID = id;
    }
}

bool has_kb_focus(uuid id) { return (get()->kbFocusID == id); }

void draw_if_kb_focus(uuid id, std::function<void(void)> cb) {
    if (has_kb_focus(id)) cb();
}

bool isMouseInside(glm::vec4 rect) {
    auto mouseScreen = glm::vec3{Input::getMousePosition(), 0.f};
    mouseScreen.y = WIN_H - mouseScreen.y;

    auto mouse = screenToWorld(mouseScreen,                              //
                               menuCameraController->camera.view,        //
                               menuCameraController->camera.projection,  //
                               glm::vec4{0, 0, WIN_W, WIN_H});
    // log_warn("{} => {}, inside? {}", Input::getMousePosition(), mouse, rect);
    return mouse.x >= rect.x && mouse.x <= rect.x + rect.z &&
           mouse.y >= rect.y && mouse.y <= rect.y + rect.w;
}

// This couldnt have been done without the tutorial
// from http://sol.gfxile.net/imgui/
// Like actually, I tried 3 times :)

struct WidgetConfig {
    std::string text;
    glm::vec2 position;
    glm::vec2 size;
    glm::vec4 color;
};

bool text(uuid id, WidgetConfig config, glm::vec2 offset = {0.f, 0.f}) {
    int i = 0;
    for (auto c : config.text) {
        i++;
        Renderer::drawQuad(
            offset + config.position + glm::vec2{i * config.size.x, 0.f},
            config.size, config.color, std::string(1, c));
    }
    return false;
}

bool button(uuid id, glm::vec2 position, glm::vec2 size) {
    bool inside =
        isMouseInside(glm::vec4{position.x, position.y, size.x, size.y});

    // everything is drawn from the center so move it so its not the center that
    // way the mouse collision works
    position.x += size.x / 2.f;
    position.y += size.y / 2.f;

    if (inside) {
        get()->hotID = id;
        if (get()->activeID == rootID && get()->lmouseDown) {
            get()->activeID = id;
        }
    }

    // if no one else has keyboard focus
    // dont mind if i do
    try_to_grab_kb(id);

    {  // start render
        Renderer::drawQuad(position, size, white, "white");
        if (get()->hotID == id) {
            if (get()->activeID == id) {
                Renderer::drawQuad(position, size, red, "white");
            } else {
                Renderer::drawQuad(position, size, green, "white");
            }
        } else {
            Renderer::drawQuad(position, size, blue, "white");
        }
        draw_if_kb_focus(id, [&]() {
            Renderer::drawQuad(position, size + glm::vec2{0.1f}, teal, "white");
        });
    }  // end render

    if (has_kb_focus(id)) {
        if (get()->pressed(Key::mapping["Widget Next"])) {
            get()->kbFocusID = rootID;
            if (get()->pressed(Key::mapping["Widget Mod"])) {
                get()->kbFocusID = get()->lastProcessed;
            }
        }
    }

    // before any returns
    get()->lastProcessed = id;

    // check click
    if (has_kb_focus(id)) {
        if (get()->pressed(Key::mapping["Widget Press"])) {
            return true;
        }
    }
    if (!get()->lmouseDown && get()->hotID == id && get()->activeID == id) {
        return true;
    }
    return false;
}

bool slider(uuid id, glm::vec2 position, glm::vec2 size, float* value,
            float mnf, float mxf) {
    bool inside =
        isMouseInside(glm::vec4{position.x, position.y, size.x, size.y});

    float min = position.y - size.y / 2.f;
    float max = position.y + size.y / 2.f;
    float ypos = min + ((max - min) * (*value));

    if (inside) {
        get()->hotID = id;
        if (get()->activeID == rootID && get()->lmouseDown) {
            get()->activeID = id;
        }
    }

    auto col = white;
    if (get()->activeID == id || get()->hotID == id) {
        col = green;
    } else {
        col = blue;
    }

    // dont mind if i do
    try_to_grab_kb(id);

    // everything is drawn from the center so move it so its not the center that
    // way the mouse collision works
    auto pos =
        glm::vec2{position.x + (size.x / 2.f), position.y + (size.y / 2.f)};
    Renderer::drawQuad(pos + glm::vec2{0.f, ypos}, glm::vec2{0.5f}, col,
                       "white");
    Renderer::drawQuad(pos, size, red, "white");

    draw_if_kb_focus(id, [&]() {
        Renderer::drawQuad(position, size + glm::vec2{0.1f}, teal, "white");
    });

    // all drawing has to happen before this ///
    if (has_kb_focus(id)) {
        if (get()->pressed(Key::mapping["Widget Next"])) {
            get()->kbFocusID = rootID;
            if (get()->pressed(Key::mapping["Widget Mod"])) {
                get()->kbFocusID = get()->lastProcessed;
            }
        }
        if (get()->pressed(Key::mapping["Widget Press"])) {
            return true;
        }
        if (Input::isKeyPressed(Key::mapping["Value Up"])) {
            (*value) += 0.005;
            if (*value > mxf) *value = mxf;
            return true;
        }
        if (Input::isKeyPressed(Key::mapping["Value Down"])) {
            (*value) -= 0.005;
            if (*value < mnf) *value = mnf;
            return true;
        }
    }
    get()->lastProcessed = id;

    if (get()->activeID == id) {
        float v = (position.y - get()->mousePosition.y) / size.y;
        if (v < mnf) v = mnf;
        if (v > mxf) v = mxf;
        if (v != *value) {
            *value = v;
            return true;
        }
    }
    return false;
}

bool textfield(uuid id, glm::vec2 position, glm::vec2 size,
               std::string& buffer) {
    int item = 0;
    bool inside =
        isMouseInside(glm::vec4{position.x, position.y, size.x, size.y});

    // everything is drawn from the center so move it so its not the center that
    // way the mouse collision works
    position.x += size.x / 2.f;
    position.y += size.y / 2.f;

    if (inside) {
        get()->hotID = id;
        if (get()->activeID == rootID && get()->lmouseDown) {
            get()->activeID = id;
        }
    }

    // if no one else has keyboard focus
    // dont mind if i do
    try_to_grab_kb(id);

    {  // start render
        // Draw Input Cursor
        float tSize = 0.3f;
        text(uuid({id.item, item++, 0}),
             WidgetConfig({.text = buffer,
                           .color = glm::vec4{1.0, 0.8f, 0.5f, 1.0f},
                           .position = position + glm::vec2{0.f, -4.f},
                           .size = glm::vec2{tSize}

             }));
        draw_if_kb_focus(id, [&]() {
            text(uuid({id.item, item++, 0}),
                 WidgetConfig(
                     {.text = "_",
                      .color = glm::vec4{1.0, 0.8f, 0.5f, 1.0f},
                      .position =
                          position + glm::vec2{tSize * buffer.size(), -4.f},
                      .size = glm::vec2{tSize}

                     }));
        });
        Renderer::drawQuad(position, size, white, "white");
        if (get()->hotID == id) {
            if (get()->activeID == id) {
                Renderer::drawQuad(position, size, red, "white");
            } else {
                Renderer::drawQuad(position, size, green, "white");
            }
        } else {
            Renderer::drawQuad(position, size, blue, "white");
        }
        // Draw focus ring
        draw_if_kb_focus(id, [&]() {
            Renderer::drawQuad(position, size + glm::vec2{0.1f}, teal, "white");
        });
    }  // end render

    bool changed = false;

    if (has_kb_focus(id)) {
        if (get()->pressed(Key::mapping["Widget Next"])) {
            get()->kbFocusID = rootID;
            if (get()->pressed(Key::mapping["Widget Mod"])) {
                get()->kbFocusID = get()->lastProcessed;
            }
        }
        if (get()->keychar != Key::KeyCode()) {
            if (get()->keychar >= 48 && get()->keychar <= 57) {
                buffer.append(std::string(1, get()->keychar));
            } else {
                auto shiftDown =
                    Input::isKeyPressed(Key::mapping["Text Cap Mod"]);
                char c = (char)(get()->keychar + (shiftDown ? 0 : 32));
                buffer.append(std::string(1, c));
            }
            changed = true;
        }
        if (get()->modchar == Key::mapping["Text Space"]) {
            buffer.append(" ");
            changed = true;
        }
        if (get()->modchar == Key::mapping["Text Backspace"]) {
            buffer.pop_back();
            changed = true;
        }
    }

    // before any returns
    get()->lastProcessed = id;

    if (!get()->lmouseDown && get()->hotID == id && get()->activeID == id) {
        get()->kbFocusID = id;
    }

    return changed;
}

void begin() {
    get()->hotID = IUI::rootID;
    get()->lmouseDown =
        Input::isMouseButtonPressed(Mouse::MouseCode::ButtonLeft);
    get()->mousePosition =
        screenToWorld(glm::vec3{Input::getMousePosition(), 0.f},
                      menuCameraController->camera.view,        //
                      menuCameraController->camera.projection,  //
                      glm::vec4{0, 0, WIN_W, WIN_H});
}
void end() {
    if (get()->lmouseDown) {
        if (get()->activeID == IUI::rootID) {
            get()->activeID = fakeID;
        }
    } else {
        get()->activeID = IUI::rootID;
    }
    get()->key = Key::KeyCode();
    get()->mod = Key::KeyCode();

    get()->keychar = Key::KeyCode();
    get()->modchar = Key::KeyCode();
}

/*
enum UILayoutType {
    Row,
    Column,
    Grid,
};

struct Style {
    glm::vec4 margin;
    glm::vec4 padding;
    glm::vec4 bgColor;
    glm::vec4 borderColor;
    glm::vec4 borderWidth;
    glm::vec4 cornerRadius;

    Style(const Style& s) {
        this->margin = s.margin;
        this->padding = s.padding;
        this->bgColor = s.bgColor;
        this->borderColor = s.borderColor;
        this->borderWidth = s.borderWidth;
        this->cornerRadius = s.cornerRadius;
    }
};
*/

}  // namespace IUI
