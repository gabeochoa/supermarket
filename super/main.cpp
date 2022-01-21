

#define BACKWARD_SUPERMARKET
#include "../vendor/backward.hpp"
//
// #define SUPER_ENGINE_PROFILING_DISABLED

#include "../vendor/supermarket-engine/engine/app.h"
#include "../vendor/supermarket-engine/engine/edit.h"
#include "../vendor/supermarket-engine/engine/file.h"
#include "../vendor/supermarket-engine/engine/input.h"
#include "../vendor/supermarket-engine/engine/pch.hpp"
#include "../vendor/supermarket-engine/engine/strutil.h"
#include "custom_fmt.h"
#include "entities.h"
#include "job.h"
#include "menu.h"
#include "util.h"

// defines cameras
#include "global.h"

// Requires access to the camera and entitites
#include "debug_layers.h"
#include "menulayer.h"
#include "superlayer.h"
#include "terminal_layer.h"
#include "tests.h"
#include "uitest.h"

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

void add_globals() {
    //
    Menu::get();  // need this to create the menu state in the first place

    GLOBALS.set("menu_state", menu.get());
    EDITOR_COMMANDS.registerCommand("set_menu_state", SetMenuCommand(),
                                    "Change Menu:: state; set_menu_state <id>");
}

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;

    // TODO until i fix the formatting,
    // we are only using this for SIGINT
    // lldb printer is so so much nicer id rather take the extra step
    // to open and run it
    backward::SignalHandling sh;

    all_tests();

    add_globals();

    App::create({
        .width = WIN_W,
        .height = WIN_H,
        .title = "SuperMarket",
        .clearEnabled = true,
        .escClosesWindow = false,
    });

    Layer* terminal = new TerminalLayer();
    App::get().pushLayer(terminal);

    Layer* fps = new FPSLayer();
    App::get().pushLayer(fps);

    Layer* profile = new ProfileLayer();
    App::get().pushLayer(profile);
    Layer* entityDebug = new EntityDebugLayer();
    App::get().pushLayer(entityDebug);

    Layer* menuLayer = new MenuLayer();
    App::get().pushLayer(menuLayer);

    Layer* game_ui = new GameUILayer();
    App::get().pushLayer(game_ui);

    // TODO integrate into gameUI layer
    Layer* jobLayer = new JobLayer();
    App::get().pushLayer(jobLayer);
    //
    Layer* super = new SuperLayer();
    App::get().pushLayer(super);

    // test code underneath game so it never shows
    Layer* uitest = new UITestLayer();
    App::get().pushLayer(uitest);

    Menu::get().state = Menu::State::Game;
    // Menu::get().state = Menu::State::Root;
    // Menu::get().state = Menu::State::UITest;

    App::get().run();

    return 0;
}
