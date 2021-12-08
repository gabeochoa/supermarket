

#include "../engine/app.h"
#include "../engine/input.h"
#include "../engine/pch.hpp"

// TODO at some point we should have some way to send this to app
constexpr int WIN_W = 1920;
constexpr int WIN_H = 1080;
constexpr float WIN_RATIO = (WIN_W * 1.f) / WIN_H;

struct SuperLayer : public Layer {
    TextureLibrary textureLibrary;

    OrthoCameraController cameraController;

    SuperLayer() : Layer("Supermarket"), cameraController(WIN_RATIO, true) {}

    virtual ~SuperLayer() {}
    virtual void onAttach() override {}
    virtual void onDetach() override {}

    virtual void onUpdate(Time dt) override {
        prof(__PROFILE_FUNC__);

        cameraController.onUpdate(dt);

        log_trace(fmt::format("{:.2}s ({:.2} ms) ", dt.s(), dt.ms()));

        Renderer::clear(/* color */ {0.1f, 0.1f, 0.1f, 1.0f});
        Renderer::begin(cameraController.camera);

        Renderer::drawQuad(glm::vec2{0.f, 0.f},                // position
                           glm::vec2{1.f, 1.f},                // scale
                           glm::vec4{1.0f, 1.0f, 1.0f, 1.0f},  // color
                           "face"                              // texture
        );

        Renderer::drawQuad(glm::vec2{0.5f, 1.f},              // position
                           glm::vec2{0.8f, 0.8f},             // scale
                           glm::vec4{0.5f, 0.4f, 0.2f, 1.0f}  // color
        );

        // Renderer::drawQuadRotated(
        // glm::vec2{0.f, 0.f}, glm::vec2{1.f, 1.f}, glm::radians(45.f),
        // glm::vec4{1.0f, 1.0f, 1.0f, 1.0f}, textureLibrary.get("face"));
        //

        Renderer::drawQuad(glm::vec2{-0.5f, -1.f},            // position
                           glm::vec2{1.1f, 1.1f},             // scale
                           glm::vec4{0.2f, 0.7f, 0.0f, 1.0f}  // color
        );

        Renderer::end();
    }

    virtual void onEvent(Event& event) override {
        log_trace(event.toString());
        cameraController.onEvent(event);
    }
};

struct ProfileLayer : public Layer {
    bool showFilenames;
    ProfileLayer() : Layer("Profiling"), showFilenames(false) {}

    virtual ~ProfileLayer() {}
    virtual void onAttach() override {}
    virtual void onDetach() override {}

    GLTtext* drawText(const std::string& content, int x, int y, float scale) {
        GLTtext* text = gltCreateText();
        gltSetText(text, content.c_str());
        gltColor(1.0f, 1.0f, 1.0f, 1.0f);
        gltDrawText2D(text, x, y, scale);
        return text;
    }

    virtual void onUpdate(Time dt) override {
        (void)dt;
        if (isMinimized) {
            return;
        }

        prof(__PROFILE_FUNC__);

        gltInit();
        int y = 10;
        float scale = 1.f;
        std::vector<GLTtext*> texts;
        gltBeginDraw();

        std::vector<SamplePair> pairs;
        for (const auto& x : _acc) {
            pairs.push_back(x);
        }

        sort(pairs.begin(), pairs.end(),
             [](const SamplePair& a, const SamplePair& b) {
                 return a.second.average() > b.second.average();
             });

        for (const auto& x : pairs) {
            auto name = x.first;
            auto stats = x.second;
            auto filename = showFilenames ? stats.filename : "";
            std::string t = fmt::format("{}{}: avg: {:.2f}ns", filename, name,
                                        stats.average());
            texts.push_back(drawText(t, 10, y, scale));
            y += 30;
        }

        texts.push_back(
            drawText(fmt::format("Press delete to toggle filenames {}",
                                 showFilenames ? "off" : "on"),
                     0, y, scale));
        y += 30;

        gltEndDraw();
        for (auto text : texts) gltDeleteText(text);
        gltTerminate();
    }

    bool onKeyPressed(KeyPressedEvent event) {
        if (event.keycode == Key::mapping["Open Profiler"]) {
            isMinimized = !isMinimized;
        }

        if (event.keycode == Key::mapping["Profiler Hide Filenames"]) {
            showFilenames = !showFilenames;
        }

        return false;
    }

    virtual void onEvent(Event& event) override {
        EventDispatcher dispatcher(event);
        dispatcher.dispatch<KeyPressedEvent>(std::bind(
            &ProfileLayer::onKeyPressed, this, std::placeholders::_1));
    }
};

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;

    App::get();

    Layer* profile = new ProfileLayer();
    App::get().pushLayer(profile);

    Layer* super = new SuperLayer();
    App::get().pushLayer(super);

    App::get().run();
    return 0;
}
