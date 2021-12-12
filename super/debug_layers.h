
#pragma once

#include "../engine/pch.hpp"

GLTtext* drawText(const std::string& content, int x, int y, float scale) {
    GLTtext* text = gltCreateText();
    gltSetText(text, content.c_str());
    gltDrawText2D(text, x, y, scale);
    return text;
}

struct JobLayer : public Layer {
    JobLayer() : Layer("Jobs") { isMinimized = !IS_DEBUG; }

    virtual ~JobLayer() {}
    virtual void onAttach() override {}
    virtual void onDetach() override {}

    virtual void onUpdate(Time dt) override {
        prof(__PROFILE_FUNC__);
        (void)dt;

        if (isMinimized) {
            return;
        }

        int y = 10;
        float scale = 1.f;
        gltInit();
        gltBeginDraw();
        gltColor(1.0f, 1.0f, 1.0f, 1.0f);
        std::vector<GLTtext*> texts;

        // Job queue
        texts.push_back(
            drawText("Job Queue (highest pri to lowest) ", 0, y, scale));
        y += 30;

        for (auto it = jobs.rbegin(); it != jobs.rend(); it++) {
            auto [type, job_list] = *it;
            int num_assigned = 0;
            for (auto it = job_list.begin(); it != job_list.end(); it++) {
                if ((*it)->isAssigned) num_assigned++;
            }
            std::string t = fmt::format("{}: {} ({} assigned)",
                                        jobTypeToString((JobType)type),
                                        job_list.size(), num_assigned);
            texts.push_back(drawText(t, 10, y, scale));
            y += 30;
        }
        // end job queue

        gltEndDraw();
        for (auto text : texts) gltDeleteText(text);
        gltTerminate();
    }

    virtual void onEvent(Event& event) override {
        EventDispatcher dispatcher(event);
        (void)event;
        (void)dispatcher;
    }
};

struct ProfileLayer : public Layer {
    bool showFilenames;
    float seconds;

    ProfileLayer() : Layer("Profiling"), showFilenames(false) {
        isMinimized = true;  //! IS_DEBUG;
        // Reset all profiling
        _acc.clear();
        seconds = 0.f;
    }

    virtual ~ProfileLayer() {}
    virtual void onAttach() override {}
    virtual void onDetach() override {}

    virtual void onUpdate(Time dt) override {
        prof(__PROFILE_FUNC__);
        (void)dt;

        if (isMinimized) {
            return;
        }

        int y = 0;
        float scale = 1.f;
        gltInit();
        gltBeginDraw();
        gltColor(1.0f, 1.0f, 1.0f, 1.0f);
        std::vector<GLTtext*> texts;

        std::vector<SamplePair> pairs;
        pairs.insert(pairs.end(), _acc.begin(), _acc.end());
        sort(pairs.begin(), pairs.end(),
             [](const SamplePair& a, const SamplePair& b) {
                 return a.second.average() > b.second.average();
             });

        for (const auto& x : pairs) {
            auto stats = x.second;
            texts.push_back(
                drawText(fmt::format("{}{}: avg: {:.2f}ns",
                                     showFilenames ? stats.filename : "",
                                     x.first, stats.average()),
                         10, (y++) * 30, scale));
        }

        {
            auto stats = Renderer::stats();
            texts.push_back(
                drawText(fmt::format("Draw Calls {}", stats.drawCalls), 10,
                         (y++) * 30, scale));
            texts.push_back(
                drawText(fmt::format("Quad Count {}", stats.quadCount), 10,
                         (y++) * 30, scale));
            texts.push_back(
                drawText(fmt::format("Vert Count {}", stats.vertCount()), 10,
                         (y++) * 30, scale));
            texts.push_back(
                drawText(fmt::format("Index Count {}", stats.indexCount()), 10,
                         (y++) * 30, scale));
            texts.push_back(
                drawText(fmt::format("Avg Rendertime {} ({} fps) ",
                                     stats.renderTime(), stats.fps()),
                         10, (y++) * 30, scale));
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
        if (event.keycode == Key::mapping["Profiler Clear Stats"]) {
            _acc.clear();
        }
        // log_info(std::to_string(event.keycode));
        return false;
    }

    virtual void onEvent(Event& event) override {
        EventDispatcher dispatcher(event);
        dispatcher.dispatch<KeyPressedEvent>(std::bind(
            &ProfileLayer::onKeyPressed, this, std::placeholders::_1));
    }
};
struct EntityDebugLayer : public Layer {
    std::shared_ptr<Entity> node;

    EntityDebugLayer() : Layer("EntityDebug") {
        isMinimized = false;  //! IS_DEBUG;

        node = std::make_shared<Billboard>(    //
            glm::vec2{0.f, 0.f},               //
            glm::vec2{0.05f, 0.05f},           //
            0.f,                               //
            glm::vec4{0.0f, 1.0f, 1.0f, 1.0f}  //
        );
    }

    virtual ~EntityDebugLayer() {}
    virtual void onAttach() override {}
    virtual void onDetach() override {}

    virtual void onUpdate(Time dt) override {
        (void)dt;
        if (isMinimized) {
            return;
        }

        prof(__PROFILE_FUNC__);

        gltInit();
        float scale = 0.003f;
        std::vector<GLTtext*> texts;
        gltColor(1.0f, 1.0f, 1.0f, 1.0f);
        gltBeginDraw();

        std::vector<std::shared_ptr<MovableEntity>> movables;

        for (auto& e : entities) {
            auto s = fmt::format("{}", *e);
            GLTtext* text = gltCreateText();
            gltSetText(text, s.c_str());

            // V = C^-1
            auto V = cameraController->camera.view;
            auto P = cameraController->camera.projection;
            auto pos = glm::vec3{
                e->position.x,
                e->position.y + gltGetTextHeight(text, scale),  //
                0.f                                             //
            };
            // M = T * R * S
            auto M =  // model matrix
                glm::translate(glm::mat4(1.f), pos) *
                glm::scale(glm::mat4(1.0f), {scale, -scale, 1.f});

            auto mvp = P * V * M;

            gltDrawText(text, glm::value_ptr(mvp));
            texts.push_back(text);

            auto m = dynamic_pointer_cast<MovableEntity>(e);
            if (m && !m->path.empty()) {
                movables.push_back(m);
            }
        }

        gltEndDraw();
        for (auto text : texts) gltDeleteText(text);
        gltTerminate();

        Renderer::begin(cameraController->camera);

        for (auto& e : entities) {
            auto [a, b, c, d] = getBoundingBox(e->position, e->size);
            node->position = a;
            node->render();
            node->position = b;
            node->render();
            node->position = c;
            node->render();
            node->position = d;
            node->render();
        }

        for (auto& m : movables) {
            for (auto it = m->path.begin(); it != m->path.end(); it++) {
                node->position = *it;
                node->render();
            }
        }

        Renderer::end();
    }

    bool onKeyPressed(KeyPressedEvent event) {
        if (event.keycode == Key::mapping["Show Entity Overlay"]) {
            isMinimized = !isMinimized;
        }
        return false;
    }

    virtual void onEvent(Event& event) override {
        EventDispatcher dispatcher(event);
        dispatcher.dispatch<KeyPressedEvent>(std::bind(
            &EntityDebugLayer::onKeyPressed, this, std::placeholders::_1));
    }
};

