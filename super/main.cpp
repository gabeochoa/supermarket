

#include "../engine/app.h"
#include "../engine/input.h"
#include "../engine/pch.hpp"

struct SuperLayer : public Layer {
    OrthoCamera camera;
    glm::vec3 camPosition;
    float camSpeed;
    float rotSpeed;

    std::shared_ptr<Shader> shader;
    std::shared_ptr<Shader> flatShader;

    std::shared_ptr<VertexArray> vertexArray;
    std::shared_ptr<VertexArray> squareVA;

    SuperLayer()
        : Layer("Supermarket"),
          camera(-1.6f, 1.6f, -0.9f, 0.9f),
          camPosition(0.0f),
          camSpeed(5.f),
          rotSpeed(180.f) {
        {
            vertexArray.reset(VertexArray::create());
            std::shared_ptr<VertexBuffer> vertexBuffer;
            std::shared_ptr<IndexBuffer> indexBuffer;

            float vertices[] = {
                -0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f,
                0.5f,  -0.5f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f,
                0.0f,  0.5f,  0.0f, 1.f,  0.f,  1.f,  1.f,
            };
            vertexBuffer.reset(
                VertexBuffer::create(vertices, sizeof(vertices)));

            vertexBuffer->setLayout({
                {"vp", BufferType::Float3},
                {"color", BufferType::Float4},
            });
            vertexArray->addVertexBuffer(vertexBuffer);

            unsigned int indices[3] = {0, 1, 2};
            indexBuffer.reset(IndexBuffer::create(indices, 3));
            vertexArray->setIndexBuffer(indexBuffer);

            std::string vertex_shader = R"(
                #version 400

                in vec3 vp;
                in vec4 color;
                uniform mat4 viewProjection;
                uniform mat4 transformMatrix;

                out vec3 op;
                out vec4 oc;

                void main(){
                    op = vp;
                    oc = color;
                    gl_Position = viewProjection * transformMatrix * vec4(vp, 1.0);
                }
            )";

            std::string fragment_shader = R"(
                #version 400
                in vec3 op;
                in vec4 oc;

                out vec4 frag_color;

                void main(){
                    frag_color = vec4(op*0.5 + 0.5, 1.0);
                    frag_color = oc;
                }
            )";

            shader.reset(new Shader(vertex_shader, fragment_shader));
        }

        {
            std::shared_ptr<VertexBuffer> squareVB;
            std::shared_ptr<IndexBuffer> squareIB;
            squareVA.reset(VertexArray::create());

            float squareVerts[] = {
                -0.5f, -0.5f, 0.0f,  // one
                0.5f,  -0.5f, 0.0f,  // two
                0.5f,  0.5f,  0.f,   //
                -0.5f, 0.0f,  0.f,   //
            };
            squareVB.reset(
                VertexBuffer::create(squareVerts, sizeof(squareVerts)));
            squareVB->setLayout(BufferLayout{
                {"i_pos", BufferType::Float3},
            });
            squareVA->addVertexBuffer(squareVB);

            unsigned int squareIs[] = {0, 1, 2, 2, 3, 0};
            squareIB.reset(IndexBuffer::create(
                squareIs, sizeof(squareIs) / sizeof(unsigned int)));
            squareVA->setIndexBuffer(squareIB);
            flatShader.reset(new Shader(flat_vert_s, flat_frag_s));
        }
    }

    virtual ~SuperLayer() {}
    virtual void onAttach() override {}
    virtual void onDetach() override {}

    void handleLiveInput(Time dt) {
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
        if (Input::isKeyPressed(Key::mapping["Rotate Clockwise"])) {
            camera.rotation += rotSpeed * dt;
        }
        if (Input::isKeyPressed(Key::mapping["Rotate Counterclockwise"])) {
            camera.rotation -= rotSpeed * dt;
        }
        camera.updateViewMat();
    }

    virtual void onUpdate(Time dt) override {
        log_trace(fmt::format("{:.2}s ({:.2} ms) ", dt.s(), dt.ms()));

        handleLiveInput(dt);

        Renderer::clear({0.1f, 0.1f, 0.1f, 1.0f});
        Renderer::begin(camera);

        flatShader->bind();
        flatShader->uploadUniformFloat4("u_color",
                                        glm::vec4(0.8f, 0.2f, 0.3f, 1.0f));
        Renderer::submit(squareVA, flatShader);
        Renderer::submit(vertexArray, shader);
        Renderer::end();
    }

    bool onKeyPressed(KeyPressedEvent& event) {
        (void)event;
        return false;
    }

    virtual void onEvent(Event& event) override {
        log_trace(event.toString());
        EventDispatcher dispatcher(event);
        dispatcher.dispatch<KeyPressedEvent>(
            std::bind(&SuperLayer::onKeyPressed, this, std::placeholders::_1));
    }
};

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;

    App::get();

    SuperLayer* super = new SuperLayer();
    App::get().pushLayer(super);

    App::get().run();
    return 0;
}
