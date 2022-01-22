

#define BACKWARD_SUPERMARKET
#include "../vendor/backward.hpp"
//
// #define SUPER_ENGINE_PROFILING_DISABLED

#include "../vendor/supermarket-engine/engine/app.h"
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

    // NOTE: Cant live in tests because app inits keyboard...
    M_ASSERT(Key::getMapping("Up") == Key::KeyCode::W, "up should be W");
    M_ASSERT(Key::getMapping("Down") == Key::KeyCode::S, "down should be s");
    M_ASSERT(Key::getMapping("Left") == Key::KeyCode::A, "left should be a");
    M_ASSERT(Key::getMapping("Right") == Key::KeyCode::D, "right should be d")

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

    // Menu::get().state = Menu::State::Game;
    // Menu::get().state = Menu::State::Root;
    Menu::get().state = Menu::State::UITest;

    App::get().run();

    return 0;
}
