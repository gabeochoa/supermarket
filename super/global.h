
#pragma once

#include "../vendor/supermarket-engine/engine/camera.h"

constexpr int WIN_W = 1920;
constexpr int WIN_H = 1080;
constexpr float WIN_RATIO = (WIN_W * 1.f) / WIN_H;
constexpr bool IS_DEBUG = true;

// TODO probably need to make a camera settings stack
// so we can just save the location of the camera
// and reload it instead of having to have multiple
static std::shared_ptr<OrthoCameraController> menuCameraController;
static std::shared_ptr<OrthoCameraController> uiTestCameraController;
static std::shared_ptr<OrthoCameraController> terminalCameraController;

