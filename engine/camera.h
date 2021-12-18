#pragma once

#include <glm/gtc/matrix_transform.hpp>

#include "event.h"
#include "input.h"
#include "pch.hpp"

struct Camera {};

struct OrthoCamera : public Camera {
    glm::mat4 projection;
    glm::mat4 view;
    glm::mat4 viewProjection;
    glm::vec3 position;
    // TODO maybe switch to quaternion later?
    float rotation;

    OrthoCamera(float left, float right, float bottom, float top)
        : projection(glm::ortho(left, right, bottom, top, -1.f, 1.f)),

          view(1.0f),
          position(0.0f),
          rotation(0.0f) {
        updateViewMat();
    }

    void setProjection(float left, float right, float bottom, float top) {
        prof(__PROFILE_FUNC__);
        projection = glm::ortho(left, right, bottom, top, -1.f, 1.f);
        updateViewMat();
    }

    void setPosition(glm::vec3 newpos) {
        position = newpos;
        updateViewMat();
    }

    void updateViewMat() {
        glm::mat4 transform =
            glm::translate(glm::mat4(1.0f), position) *
            glm::rotate(glm::mat4(1.0f), glm::radians(rotation),
                        glm::vec3(0, 0, 1));

        view = glm::inverse(transform);
        viewProjection = projection * view;
    }
};

struct OrthoCameraController {
    float aspectRatio;
    float zoomLevel;
    float camSpeed;
    float rotSpeed;
    bool movementEnabled;
    bool rotationEnabled;
    OrthoCamera camera;

    OrthoCameraController(float ratio, float defaultZoom = 1.f,
                          float cameraSpeed = 5.f, float rotationSpeed = 180.f)
        : aspectRatio(ratio),
          zoomLevel(defaultZoom),
          camSpeed(cameraSpeed),
          rotSpeed(rotationSpeed),
          movementEnabled(cameraSpeed != 0.f),
          rotationEnabled(rotationSpeed != 0.f),
          camera(-ratio * zoomLevel, ratio * zoomLevel, -zoomLevel, zoomLevel) {
    }

    void onUpdate(Time dt) {
        prof(__PROFILE_FUNC__);
        if (movementEnabled) {
            if (Input::isKeyPressed(Key::mapping["Left"])) {
                camera.position.x -= camSpeed * dt;
            }
            if (Input::isKeyPressed(Key::mapping["Right"])) {
                camera.position.x += camSpeed * dt;
            }
            if (Input::isKeyPressed(Key::mapping["Down"])) {
                camera.position.y -= camSpeed * dt;
            }
            if (Input::isKeyPressed(Key::mapping["Up"])) {
                camera.position.y += camSpeed * dt;
            }
        }
        if (rotationEnabled) {
            if (Input::isKeyPressed(Key::mapping["Rotate Clockwise"])) {
                camera.rotation += rotSpeed * dt;
            }
            if (Input::isKeyPressed(Key::mapping["Rotate Counterclockwise"])) {
                camera.rotation -= rotSpeed * dt;
            }
        }
        camera.updateViewMat();
    }

    bool onMouseScrolled(Mouse::MouseScrolledEvent& event) {
        zoomLevel =
            fmin(fmax(zoomLevel - (event.GetYOffset() * 0.25), 0.5f), 20.f);
        camera.setProjection(-aspectRatio * zoomLevel, aspectRatio * zoomLevel,
                             -zoomLevel, zoomLevel);
        camSpeed = zoomLevel;
        return false;
    }

    bool onWindowResized(WindowResizeEvent& event) {
        aspectRatio = event.width() / event.height();
        camera.setProjection(-aspectRatio * zoomLevel, aspectRatio * zoomLevel,
                             -zoomLevel, zoomLevel);
        return false;
    }

    void onEvent(Event& event) {
        EventDispatcher dispatcher(event);
        dispatcher.dispatch<Mouse::MouseScrolledEvent>(
            std::bind(&OrthoCameraController::onMouseScrolled, this,
                      std::placeholders::_1));
        dispatcher.dispatch<WindowResizeEvent>(
            std::bind(&OrthoCameraController::onWindowResized, this,
                      std::placeholders::_1));
    }
};
