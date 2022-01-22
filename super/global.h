
#pragma once

#include <any>
#include <sstream>

#include "../vendor/supermarket-engine/engine/camera.h"
#include "../vendor/supermarket-engine/engine/commands.h"
#include "../vendor/supermarket-engine/engine/globals.h"
#include "../vendor/supermarket-engine/engine/edit.h"

#include "menu.h"

constexpr int WIN_W = 1920;
constexpr int WIN_H = 1080;
constexpr float WIN_RATIO = (WIN_W * 1.f) / WIN_H;
constexpr bool IS_DEBUG = true;

// TODO probably need to make a camera settings stack
// so we can just save the location of the camera
// and reload it instead of having to have multiple
static std::shared_ptr<OrthoCameraController> menuCameraController;
static std::shared_ptr<OrthoCameraController> uiTestCameraController;

struct SetMenuCommand : Command<Menu> {
    std::vector<std::any> convert(const std::vector<std::string>& tokens) {
        // Need to convert to T* and T
        std::vector<std::any> out;
        if (tokens.size() != 1) {
            this->msg = fmt::format("Invalid number of parameters {} wanted {}",
                                    tokens.size(), 1);
            return out;
        }
        out.push_back(GLOBALS.get_ptr<Menu>("menu_state"));
        int val = Deserializer<int>(tokens[0]);
        Menu::State state = static_cast<Menu::State>(val);
        out.push_back(state);
        return out;
    }

    std::string operator()(const std::vector<std::string>& params) {
        auto values = this->convert(params);
        if (!values.empty()) {
            Menu* m = std::any_cast<Menu*>(values[0]);
            Menu::State val = std::any_cast<Menu::State>(values[1]);
            m->state = val;
            this->msg = fmt::format("{} is now {}", params[0], val);
        }
        return this->msg;
    }
};


inline void add_globals() {
    //
    Menu::get();  // need this to create the menu state in the first place

    GLOBALS.set("menu_state", menu.get());
    EDITOR_COMMANDS.registerCommand("set_menu_state", SetMenuCommand(),
                                    "Change Menu:: state; set_menu_state <id>");
}


