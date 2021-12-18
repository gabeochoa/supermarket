

#pragma once
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

static const char* DEFAULT_TEX = "white";
static const int MAX_TEX = 16;
struct Renderer {
    struct QuadVert {
        glm::vec3 position;
        glm::vec4 color;
        glm::vec2 texcoord;
        float texindex;
        float tilingfactor;
    };

    struct Statistics {
        std::array<float, 100> renderTimes;
        int drawCalls = 0;
        int quadCount = 0;
        int textureCount = 0;
        size_t frameCount = 0;
        float frameBeginTime = 0.f;
        float totalFrameTime = 0.f;

        void reset() {
            drawCalls = 0;
            quadCount = 0;
            textureCount = 0;
        }

        void begin() { frameBeginTime = (float)glfwGetTime(); }

        void end() {
            auto endt = (float)glfwGetTime();
            renderTimes[frameCount] = endt - frameBeginTime;
            totalFrameTime +=
                renderTimes[frameCount] -
                renderTimes[(frameCount + 1) % renderTimes.size()];
            frameCount += 1;
            if (frameCount >= renderTimes.size()) {
                frameCount = 0;
            }
        }
    };

    static Statistics stats;

    struct SceneData {
        // Max per draw call
        const int MAX_QUADS = 1000;
        const int MAX_VERTS = MAX_QUADS * 4;
        const int MAX_IND = MAX_QUADS * 6;

        std::shared_ptr<VertexArray> quadVA;
        std::shared_ptr<VertexBuffer> quadVB;

        int quadIndexCount = 0;
        QuadVert* qvbufferstart = nullptr;
        QuadVert* qvbufferptr = nullptr;

        glm::mat4 viewProjection;

        ShaderLibrary shaderLibrary;
        std::array<std::shared_ptr<Texture2D>, MAX_TEX> textureSlots;
        // 0 will be white
        // 1 is full transparent
        int MIN_TEX_SLOT = 2;
        int nextTexSlot = MIN_TEX_SLOT;
    };

    static SceneData* sceneData;

    // TODO lets add something similar for shaders
    static void addTexture(const std::string& filepath, float tiling = 1.f) {
        if ((int)textureLibrary.size() >= MAX_TEX) {
            log_error("Cant have more than MAX TEX textures (16)");
            return;
        }
        auto texName = textureLibrary.load(filepath);
        textureLibrary.get(texName)->tilingFactor = 2.f;
    }

    static void addSubtexture(const std::string& textureName,
                              const std::string& name, float x, float y,
                              float spriteWidth, float spriteHeight) {
        textureLibrary.addSubtexture(textureName, name, x, y, spriteWidth,
                                     spriteHeight);
    }

    static void init_default_shaders() {
        sceneData->shaderLibrary.load("./engine/shaders/flat.glsl");
        sceneData->shaderLibrary.load("./engine/shaders/texture.glsl");
    }

    static void init_default_textures() {
        {
            std::shared_ptr<Texture> whiteTexture =
                std::make_shared<Texture2D>("white", 1, 1);
            unsigned int data = 0xffffffff;
            whiteTexture->setData(&data);
            textureLibrary.add(whiteTexture);
        }

        // TODO have to make sure this resource
        // exists as part of the engine
        // and not the game textures
        addTexture("./resources/transparent.png");
    }

    static void init() {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glEnable(GL_DEPTH_TEST);

        init_default_shaders();
        init_default_textures();

        std::shared_ptr<VertexBuffer> squareVB;
        std::shared_ptr<IndexBuffer> squareIB;
        std::shared_ptr<IndexBuffer> quadIB;
        sceneData->quadVA.reset(VertexArray::create());

        sceneData->quadVB.reset(
            VertexBuffer::create(sceneData->MAX_VERTS * sizeof(QuadVert)));
        sceneData->quadVB->setLayout(BufferLayout{
            {"i_pos", BufferType::Float3},
            {"i_color", BufferType::Float4},
            {"i_texcoord", BufferType::Float2},
            {"i_texindex", BufferType::Float},
            {"i_tilingfactor", BufferType::Float},
        });
        sceneData->quadVA->addVertexBuffer(sceneData->quadVB);

        sceneData->qvbufferstart = new QuadVert[sceneData->MAX_VERTS];

        uint32_t* quadIndices = new uint32_t[sceneData->MAX_IND];
        uint32_t offset = 0;

        for (int i = 0; i < sceneData->MAX_IND; i += 6) {
            quadIndices[i + 0] = offset + 0;
            quadIndices[i + 1] = offset + 1;
            quadIndices[i + 2] = offset + 2;

            quadIndices[i + 3] = offset + 2;
            quadIndices[i + 4] = offset + 3;
            quadIndices[i + 5] = offset + 0;

            offset += 4;
        }

        quadIB.reset(IndexBuffer::create(quadIndices, sceneData->MAX_IND));
        sceneData->quadVA->setIndexBuffer(quadIB);
        delete[] quadIndices;

        std::array<int, MAX_TEX> samples = {0};
        for (size_t i = 0; i < MAX_TEX; i++) {
            samples[(int)i] = (int)i;
        }
        auto textureShader = sceneData->shaderLibrary.get("texture");
        textureShader->bind();
        textureShader->uploadUniformIntArray("u_textures", samples.data(),
                                             MAX_TEX);
    }

    static void resize(int width, int height) {
        glViewport(0, 0, width, height);
    }

    static void shutdown() {
        delete[] sceneData->qvbufferstart;
        delete sceneData;
    }

    static void clear(const glm::vec4& color) {
        prof(__PROFILE_FUNC__);
        glClearColor(color.r, color.g, color.b, color.a);
        glClear(GL_COLOR_BUFFER_BIT |
                GL_DEPTH_BUFFER_BIT);  // Clear the buffers
    }

    static void draw(const std::shared_ptr<VertexArray>& vertexArray,
                     int indexCount = 0) {
        prof(__PROFILE_FUNC__);
        int count =
            indexCount ? indexCount : vertexArray->indexBuffer->getCount();
        vertexArray->bind();
        glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_INT, nullptr);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    static void begin(OrthoCamera& cam) {
        prof(__PROFILE_FUNC__);
        sceneData->viewProjection = cam.viewProjection;

        auto textureShader = sceneData->shaderLibrary.get("texture");
        textureShader->bind();
        textureShader->uploadUniformMat4("viewProjection",
                                         sceneData->viewProjection);

        start_batch();
    }

    static void end() {
        prof(__PROFILE_FUNC__);
        flush();
    }

    static void start_batch() {
        sceneData->textureSlots[0] =
            dynamic_pointer_cast<Texture2D>(textureLibrary.get("white"));
        sceneData->quadIndexCount = 0;
        sceneData->qvbufferptr = sceneData->qvbufferstart;
        sceneData->nextTexSlot = sceneData->MIN_TEX_SLOT;
    }

    static void next_batch() {
        flush();
        start_batch();
    }

    static void flush() {
        if (sceneData->quadIndexCount == 0) return;  // Nothing to draw
        uint32_t dataSize = (uint32_t)((uint8_t*)sceneData->qvbufferptr -
                                       (uint8_t*)sceneData->qvbufferstart);
        sceneData->quadVB->setData(sceneData->qvbufferstart, dataSize);

        for (int i = 0; i < sceneData->nextTexSlot; i++)
            sceneData->textureSlots[i]->bind(i);

        sceneData->shaderLibrary.get("texture")->bind();

        draw(sceneData->quadVA, sceneData->quadIndexCount);

        //
        stats.drawCalls++;
    }

    static void drawQuad(const glm::mat4& transform, const glm::vec4& color,
                         const std::string& textureName = DEFAULT_TEX) {
        const std::array<glm::vec4, 4> vertexCoords = {{
            {-0.5f, -0.5f, 0.0f, 1.0f},
            {0.5f, -0.5f, 0.0f, 1.0f},
            {0.5f, 0.5f, 0.0f, 1.0f},
            {-0.5f, 0.5f, 0.0f, 1.0f},
        }};

        if (sceneData->quadIndexCount >= sceneData->MAX_IND) {
            next_batch();
        }

        auto textureStatus = textureLibrary.isTextureOrSubtexture(textureName);

        std::shared_ptr<Texture> texture;
        std::shared_ptr<Subtexture> subtexture;

        if (textureStatus == -1 || textureStatus == 0) {
            texture = textureLibrary.get(textureName);
        } else {
            subtexture = textureLibrary.getSubtexture(textureName);
            // only do this -> when valid, to avoid segfault
            // we check for validity later so this is fine
            if (subtexture) texture = subtexture->texture;
        }

        // Load the corresponding Texture into the texture slots
        int textureIndex = 0;
        if (texture                        // tex is valid (ie not nullptr)
            && textureName != DEFAULT_TEX  // default tex is always loaded
            && textureStatus != -1  // set to default tex so already loaded
        ) {
            for (int i = 1; i < sceneData->nextTexSlot; i++) {
                if (*(sceneData->textureSlots[i]) == *texture) {
                    textureIndex = i;
                    break;
                }
            }
            if (textureIndex == 0) {
                if (sceneData->nextTexSlot >= MAX_TEX) {
                    next_batch();
                }
                textureIndex = sceneData->nextTexSlot;
                sceneData->textureSlots[textureIndex] =
                    dynamic_pointer_cast<Texture2D>(texture);
                sceneData->nextTexSlot++;

                stats.textureCount++;
            }
        } else {
            texture = textureLibrary.get(DEFAULT_TEX);
            // if we fall into this case,
            // either textureName didnt exist at all
            // or textureName was a texture and is invalid
            // or textureName was an invalid subtexture or texture is
            textureStatus = -1;
        }
        // else use 0 which is white texture

        for (size_t i = 0; i < 4; i++) {
            sceneData->qvbufferptr->position = transform * vertexCoords[i];
            sceneData->qvbufferptr->color = color;
            sceneData->qvbufferptr->texcoord =
                textureStatus != 1 ? texture->textureCoords[i]
                                   : subtexture->textureCoords[i];
            sceneData->qvbufferptr->texindex = (float)textureIndex;
            // TODO FIX - for some reason the "->tilingFactor" controls the
            // texture index in the shader
            sceneData->qvbufferptr->tilingfactor = (float)textureIndex;
            // sceneData->qvbufferptr->entityID = entityID;
            sceneData->qvbufferptr++;
        }
        sceneData->quadIndexCount += 6;

        stats.quadCount++;
    }

    ////// ////// ////// ////// ////// ////// ////// //////
    //      the draw quads below here, just call one of the ones above
    ////// ////// ////// ////// ////// ////// ////// //////

    static void drawQuad(const glm::vec2& position, const glm::vec2& size,
                         const glm::vec4& color,
                         const std::string& textureName = DEFAULT_TEX) {
        Renderer::drawQuad(glm::vec3{position.x, position.y, 0.f}, size, color,
                           textureName);
    }

    static void drawQuad(const glm::vec3& position, const glm::vec2& size,
                         const glm::vec4& color,
                         const std::string& textureName = DEFAULT_TEX) {
        auto transform = glm::translate(glm::mat4(1.f), position) *
                         glm::scale(glm::mat4(1.0f), {size.x, size.y, 1.0f});
        Renderer::drawQuad(transform, color, textureName);
    }

    static void drawQuadRotated(const glm::vec2& position,
                                const glm::vec2& size, float angleInRad,
                                const glm::vec4& color,
                                const std::string& textureName = DEFAULT_TEX) {
        Renderer::drawQuadRotated({position.x, position.y, 0.f}, size,
                                  angleInRad, color, textureName);
    }

    static void drawQuadRotated(const glm::vec3& position,
                                const glm::vec2& size, float angleInRad,
                                const glm::vec4& color,
                                const std::string& textureName = DEFAULT_TEX) {
        prof(__PROFILE_FUNC__);

        auto transform =
            glm::translate(glm::mat4(1.f), position) *
            glm::rotate(glm::mat4(1.f), angleInRad, {0.0f, 0.0f, 1.f}) *
            glm::scale(glm::mat4(1.0f), {size.x, size.y, 1.0f});

        drawQuad(transform, color, textureName);
    }
};
