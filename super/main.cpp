

#include "../engine/app.h"
#include "../engine/input.h"
#include "../engine/pch.hpp"
#include "custom_fmt.h"
#include "entities.h"
#include "job.h"
#include "menu.h"
#include "util.h"

constexpr int WIN_W = 1920;
constexpr int WIN_H = 1080;
constexpr float WIN_RATIO = (WIN_W * 1.f) / WIN_H;
constexpr bool IS_DEBUG = true;

// TODO probably need to make a camera settings stack
// so we can just save the location of the camera
// and reload it instead of having to have multiple
static std::shared_ptr<OrthoCameraController> cameraController;
static std::shared_ptr<OrthoCameraController> menuCameraController;
static std::shared_ptr<OrthoCameraController> uiTestCameraController;

// Requires access to the camera and entitites
#include "debug_layers.h"
#include "menulayer.h"
#include "superlayer.h"
#include "tests.h"
#include "uitest.h"

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;

    // on start tests
    {
        theta_test();
        point_collision_test();
    }

    app.reset(App::create({
        .width = WIN_W,
        .height = WIN_H,
        .title = "SuperMarket",
        .clearEnabled = true,
        .escClosesWindow = false,
    }));

    Layer* super = new SuperLayer();
    App::get().pushLayer(super);

    Layer* menu = new MenuLayer();
    App::get().pushLayer(menu);

    Layer* uitest = new UITestLayer();
    App::get().pushLayer(uitest);

    Layer* profile = new ProfileLayer();
    App::get().pushLayer(profile);

    Layer* entityDebug = new EntityDebugLayer();
    App::get().pushLayer(entityDebug);

    Layer* jobLayer = new JobLayer();
    App::get().pushLayer(jobLayer);

    Menu::get().state = Menu::State::UITest;

    App::get().run();

    return 0;
}
