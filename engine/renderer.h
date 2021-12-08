

#pragma once

#include <cstdint>
//
#include "pch.hpp"
//
#include "buffer.h"
#include "camera.h"
#include "shader.h"

// TODO if there are ever any other renderers (directx vulcan metal)
// then have to subclass this for each one
struct Renderer3D {
    struct SceneData {
        glm::mat4 viewProjection;
    };

    static SceneData* sceneData;

    static void init() {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }

    static void begin(OrthoCamera& cam) {
        sceneData->viewProjection = cam.viewProjection;
    }
    static void end() {}

    static void clear(const glm::vec4& color) {
        glClearColor(color.r, color.g, color.b, color.a);
        glClear(GL_COLOR_BUFFER_BIT |
                GL_DEPTH_BUFFER_BIT);  // Clear the buffers
    }

    static void submit(const std::shared_ptr<VertexArray>& vertexArray,
                       const std::shared_ptr<Shader>& shader,
                       const glm::mat4& transform = glm::mat4(1.f)) {
        shader->bind();
        shader->uploadUniformMat4("viewProjection", sceneData->viewProjection);
        shader->uploadUniformMat4("transformMatrix", transform);

        vertexArray->bind();
        Renderer3D::draw(vertexArray);
    }

    static void draw(const std::shared_ptr<VertexArray>& vertexArray) {
        glDrawElements(GL_TRIANGLES, vertexArray->indexBuffer->getCount(),
                       GL_UNSIGNED_INT, nullptr);
    }
};

struct Renderer {
    struct QuadVert {
        glm::vec3 position;
        glm::vec2 texCoord;
        glm::vec4 color;
        float textureIndex;
        float tilingFactor;
    };

    struct SceneData {
        // per draw call, before forced flush 🚽
        const int MAX_QUADS = 100;
        const int MAX_VERTS = MAX_QUADS * 4;
        const int MAX_IND = MAX_QUADS * 6;

        int quadCount = 0;

        QuadVert* qvBufferStart = nullptr;
        QuadVert* qvBufferPtr = nullptr;

        glm::mat4 viewProjection;

        std::shared_ptr<VertexArray> vertexArray;
        std::shared_ptr<VertexBuffer> vertexBuffer;
        ShaderLibrary shaderLibrary;
        TextureLibrary textureLibrary;
    };

    static SceneData* sceneData;

    static void init() {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glEnable(GL_DEPTH_TEST);

        sceneData->vertexArray.reset(VertexArray::create());
        sceneData->vertexBuffer.reset(
            VertexBuffer::create(sceneData->MAX_VERTS * sizeof(QuadVert)));

        sceneData->vertexBuffer->setLayout(BufferLayout{
            {"i_pos", BufferType::Float3},
            {"i_texcoord", BufferType::Float2},
            {"i_color", BufferType::Float4},
            {"i_texindex", BufferType::Float},
            {"i_tilingfactor", BufferType::Float},
        });
        sceneData->vertexArray->addVertexBuffer(sceneData->vertexBuffer);

        sceneData->qvBufferStart = new QuadVert[sceneData->MAX_QUADS];

        uint32_t* squareIs = new uint32_t[sceneData->MAX_IND];
        int offset = 0;
        for (int i = 0; i < sceneData->MAX_IND; i += 6) {
            squareIs[i + 0] = offset + 0;
            squareIs[i + 1] = offset + 1;
            squareIs[i + 2] = offset + 2;

            squareIs[i + 3] = offset + 2;
            squareIs[i + 4] = offset + 3;
            squareIs[i + 5] = offset + 0;

            offset += 4;
        }

        std::shared_ptr<IndexBuffer> squareIB;
        squareIB.reset(IndexBuffer::create(squareIs, sceneData->MAX_IND));
        sceneData->vertexArray->setIndexBuffer(squareIB);
        delete[] squareIs;

        sceneData->shaderLibrary.load("./engine/shaders/flat.glsl");
        sceneData->shaderLibrary.load("./engine/shaders/texture.glsl");

        std::shared_ptr<Texture> whiteTexture =
            std::make_shared<Texture2D>("white", 1, 1, 0);
        unsigned int data = 0xffffffff;
        whiteTexture->setData(&data);
        sceneData->textureLibrary.add(whiteTexture);

        sceneData->textureLibrary.load("./resources/face.png", 1);
        sceneData->textureLibrary.load("./resources/screen.png", 2);
    }

    static void resize(int width, int height) {
        glViewport(0, 0, width, height);
    }

    static void begin(OrthoCamera& cam) {
        prof(__PROFILE_FUNC__);

        sceneData->viewProjection = cam.viewProjection;
        start_batch();
    }

    static void start_batch() {
        sceneData->quadCount = 0;
        sceneData->qvBufferPtr = sceneData->qvBufferStart;
    }

    static void next_batch() {
        flush();
        start_batch();
    }

    static void end() {
        prof(__PROFILE_FUNC__);
        flush();
    }

    static void flush() {
        // Nothing to draw
        if (sceneData->quadCount == 0) return;

        uint32_t dataSize = (uint32_t)((uint8_t*)sceneData->qvBufferPtr -
                                       (uint8_t*)sceneData->qvBufferStart);
        sceneData->vertexBuffer->setData(sceneData->qvBufferStart, dataSize);

        // for (auto& tex : sceneData->textureLibrary) {
        // tex.second->bind();
        // }

        auto textureShader = sceneData->shaderLibrary.get("texture");
        textureShader->bind();
        textureShader->uploadUniformMat4("viewProjection",
                                         sceneData->viewProjection);
        Renderer::draw(sceneData->vertexArray, sceneData->quadCount);
    }

    static void shutdown() {
        delete[] sceneData->qvBufferStart;
        delete sceneData;
    }

    static void clear(const glm::vec4& color) {
        prof(__PROFILE_FUNC__);
        glClearColor(color.r, color.g, color.b, color.a);
        glClear(GL_COLOR_BUFFER_BIT |
                GL_DEPTH_BUFFER_BIT);  // Clear the buffers
    }

    static void draw(const std::shared_ptr<VertexArray>& vertexArray,
                     unsigned int count = 0) {
        prof(__PROFILE_FUNC__);
        auto c = count ? count : vertexArray->indexBuffer->getCount();
        glDrawElements(GL_TRIANGLES, c, GL_UNSIGNED_INT, nullptr);
    }

    static void drawQuad(const glm::mat4& transform,  //
                         const glm::vec4& color,      //
                         const std::string& texturename) {
        prof(__PROFILE_FUNC__);

        if (sceneData->quadCount >= sceneData->MAX_IND) {
            next_batch();
        }

        const glm::vec2 textureCoords[] = {
            {0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f}};

        const glm::vec4 vertexPos[] = {{-0.5f, -0.5f, 0.0f, 1.0f},
                                       {0.5f, -0.5f, 0.0f, 1.0f},
                                       {0.5f, 0.5f, 0.0f, 1.0f},
                                       {-0.5f, 0.5f, 0.0f, 1.0f}};

        // use the default white texture if none supplied
        auto texture = sceneData->textureLibrary.get(texturename);
        int texIndex = texture == nullptr ? 0 : texture->textureIndex;
        if (texture == nullptr) {
            log_warn(fmt::format("Texture : {} is missing", texturename));
        }

        for (int i = 0; i < 4; i++) {
            sceneData->qvBufferPtr->position = transform * vertexPos[i];
            sceneData->qvBufferPtr->color = color;
            sceneData->qvBufferPtr->texCoord = textureCoords[i];
            sceneData->qvBufferPtr->textureIndex = texIndex;
            sceneData->qvBufferPtr->tilingFactor = 1.0f;
            sceneData->qvBufferPtr++;
        }
        sceneData->quadCount += 6;
    }

    static void drawQuadRotated(const glm::vec3& position,
                                const glm::vec2& size, float angleInRad,
                                const glm::vec4& color,
                                const std::string& texturename) {
        prof(__PROFILE_FUNC__);

        auto transform =
            glm::translate(glm::mat4(1.f), position) *
            glm::rotate(glm::mat4(1.f), angleInRad, {0.0f, 0.0f, 1.f}) *
            glm::scale(glm::mat4(1.0f), {size.x, size.y, 1.0f});

        auto texture = sceneData->textureLibrary.get(texturename);

        if (texture == nullptr) {
            auto flatShader = sceneData->shaderLibrary.get("flat");
            flatShader->bind();
            flatShader->uploadUniformMat4("transformMatrix", transform);
            flatShader->uploadUniformFloat4("u_color", color);
        } else {
            auto textureShader = sceneData->shaderLibrary.get("texture");
            textureShader->bind();
            textureShader->uploadUniformMat4("transformMatrix", transform);
            textureShader->uploadUniformInt("u_texture", texture->textureIndex);
            textureShader->uploadUniformFloat4("u_color", color);
            // TODO if we end up wanting this then obv have to expose it
            // through function param
            textureShader->uploadUniformFloat("f_tiling",
                                              texture->tilingFactor);
            texture->bind();
        }

        sceneData->vertexArray->bind();
        Renderer::draw(sceneData->vertexArray);
    }

    ////// ////// ////// ////// ////// ////// ////// //////
    //      the draw quads below here, just call one of the ones above
    ////// ////// ////// ////// ////// ////// ////// //////

    static void drawQuad(const glm::vec3& position,                 //
                         const glm::vec2& size,                     //
                         const glm::vec4& color,                    //
                         const std::string& texture_name = "white"  //

    ) {
        auto transform = glm::translate(glm::mat4(1.f), position) *
                         glm::scale(glm::mat4(1.0f), {size.x, size.y, 1.0f});
        drawQuad(transform, color, texture_name);
    }

    static void drawQuad(const glm::vec2& position, const glm::vec2& size,
                         const glm::vec4& color,
                         const std::string& texturename = "white") {
        Renderer::drawQuad(glm::vec3{position.x, position.y, 0.f}, size, color,
                           texturename);
    }

    static void drawQuadRotated(const glm::vec2& position,
                                const glm::vec2& size,  //
                                float angleInRad,       //
                                const glm::vec4& color,
                                const std::string& texturename = "white") {
        Renderer::drawQuadRotated(                   //
            glm::vec3{position.x, position.y, 0.f},  //
            size,                                    //
            angleInRad,                              //
            color,                                   //
            texturename                              //
        );
    }
};
