
#include "openglwindow.h"

#include "pch.hpp"
using namespace Mouse;

void OpenGLWindow::init(const WindowConfig& config) {
    info.title = config.title;
    info.width = config.width;
    info.height = config.height;

    log(fmt::format("Creating a new window {} ({}, {})", info.title, info.width,
                    info.height));

    // TODO Have some way of only doing this once
    int success = glfwInit();
    M_ASSERT(success, "Failed to init glfw");

#ifdef __APPLE__
    /* We need to explicitly ask for a 3.2 context on OS X */
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#endif
    window = glfwCreateWindow(info.width, info.height, info.title.c_str(), NULL,
                              NULL);
    M_ASSERT(window, "Failed to create wwindow");

    context = new OpenGLContext(window);
    context->init();

    glfwSetWindowUserPointer(window, &info);
    setVSync(true);
    auto err = glewInit();
    M_ASSERT(err == GLEW_OK, "Failed to init glew");

    glfwSetWindowSizeCallback(window, [](GLFWwindow* w, int width, int height) {
        WindowInfo& info = *(WindowInfo*)glfwGetWindowUserPointer(w);
        info.width = width;
        info.height = height;

        WindowResizeEvent event(width, height);
        info.callback(event);
    });

    glfwSetWindowCloseCallback(window, [](GLFWwindow* w) {
        WindowInfo& info = *(WindowInfo*)glfwGetWindowUserPointer(w);
        WindowCloseEvent event;
        info.callback(event);
    });

    glfwSetKeyCallback(
        window, [](GLFWwindow* w, int key, int scancode, int action, int mods) {
            (void)scancode;
            (void)mods;
            WindowInfo& info = *(WindowInfo*)glfwGetWindowUserPointer(w);
            switch (action) {
                case GLFW_PRESS: {
                    KeyPressedEvent event(key, 0);
                    info.callback(event);
                } break;
                case GLFW_RELEASE: {
                    KeyReleasedEvent event(key);
                    info.callback(event);
                } break;
                case GLFW_REPEAT: {
                    KeyPressedEvent event(key, 1);
                    info.callback(event);
                } break;
            }
        });

    glfwSetMouseButtonCallback(
        window, [](GLFWwindow* w, int button, int action, int mods) {
            (void)mods;
            WindowInfo& info = *(WindowInfo*)glfwGetWindowUserPointer(w);

            switch (action) {
                case GLFW_PRESS: {
                    MouseButtonPressedEvent event(button);
                    info.callback(event);
                    break;
                }
                case GLFW_RELEASE: {
                    MouseButtonReleasedEvent event(button);
                    info.callback(event);
                    break;
                }
            }
        });

    glfwSetScrollCallback(
        window, [](GLFWwindow* w, double xOffset, double yOffset) {
            WindowInfo& info = *(WindowInfo*)glfwGetWindowUserPointer(w);

            MouseScrolledEvent event((float)xOffset, (float)yOffset);
            info.callback(event);
        });

    glfwSetCursorPosCallback(
        window, [](GLFWwindow* w, double xPos, double yPos) {
            WindowInfo& info = *(WindowInfo*)glfwGetWindowUserPointer(w);

            MouseMovedEvent event((float)xPos, (float)yPos);
            info.callback(event);
        });
}

Window* Window::create(const WindowConfig& config) {
    return new OpenGLWindow(config);
}

bool Input::IsKeyPressed(const Key::KeyCode key) {
    auto* window =
        static_cast<GLFWwindow*>(App::get().getWindow().getNativeWindow());
    auto state = glfwGetKey(window, static_cast<int32_t>(key));
    return state == GLFW_PRESS || state == GLFW_REPEAT;
}

bool Input::IsMouseButtonPressed(const MouseCode button) {
    auto* window =
        static_cast<GLFWwindow*>(App::get().getWindow().getNativeWindow());
    auto state = glfwGetMouseButton(window, static_cast<int32_t>(button));
    return state == GLFW_PRESS;
}

glm::vec2 Input::GetMousePosition() {
    auto* window =
        static_cast<GLFWwindow*>(App::get().getWindow().getNativeWindow());
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);

    return {(float)xpos, (float)ypos};
}

float Input::GetMouseX() { return GetMousePosition().x; }

float Input::GetMouseY() { return GetMousePosition().y; }