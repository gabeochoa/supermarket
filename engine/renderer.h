

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
static const glm::mat4 im(1.f);
struct Renderer {
    struct QuadVert {
        glm::vec3 position;
        glm::vec4 color;
        glm::vec2 texcoord;
        float texindex;
        // float tilingfactor;
    };

    struct LineVert {
        glm::vec4 clipcrd;
        glm::vec4 color;
        glm::vec2 texcoord;
        float width;
        float length;
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

        const int MAX_LINES = 20000;
        const int MAX_LINE_VERTS = MAX_LINES * 4;
        const int MAX_LINE_INDICES = MAX_LINES * 6;

        std::shared_ptr<VertexArray> quadVA;
        std::shared_ptr<VertexBuffer> quadVB;

        int quadIndexCount = 0;
        QuadVert* qvbufferstart = nullptr;
        QuadVert* qvbufferptr = nullptr;

        std::shared_ptr<VertexArray> lineVA;
        std::shared_ptr<VertexBuffer> lineVB;

        int lineIndexCount = 0;
        LineVert* lvbufferstart = nullptr;
        LineVert* lvbufferptr = nullptr;

        glm::mat4 viewProjection;

        ShaderLibrary shaderLibrary;
        std::array<std::shared_ptr<Texture2D>, MAX_TEX> textureSlots;
        int nextTexSlot = 1;  // 0 will be white

        int width, height;
    };

    static SceneData* sceneData;

    // TODO lets add something similar for shaders
    static void addTexture(const std::string& filepath) {
        auto texName = textureLibrary.load(filepath);
        textureLibrary.get(texName)->tilingFactor = 1.f;
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
        sceneData->shaderLibrary.load("./engine/shaders/line.glsl");
    }

    static void init_default_textures() {
        std::shared_ptr<Texture> whiteTexture =
            std::make_shared<Texture2D>("white", 1, 1);
        unsigned int data = 0xffffffff;
        whiteTexture->setData(&data);
        textureLibrary.add(whiteTexture);
    }

    static void init_line_buffers() {
        std::shared_ptr<VertexBuffer> lineVB;
        std::shared_ptr<IndexBuffer> lineIB;

        sceneData->lineVA.reset(VertexArray::create());
        sceneData->lineVB.reset(
            VertexBuffer::create(sceneData->MAX_LINE_VERTS * sizeof(LineVert)));
        sceneData->lineVB->setLayout(BufferLayout{
            {"i_clip_coord", BufferType::Float4},
            {"i_color", BufferType::Float4},
            {"i_texcoord", BufferType::Float2},
            {"i_width", BufferType::Float},
            {"i_length", BufferType::Float},
        });
        sceneData->lineVA->addVertexBuffer(sceneData->lineVB);
        sceneData->lvbufferstart = new LineVert[sceneData->MAX_LINE_VERTS];

        uint32_t* lineIndices = new uint32_t[sceneData->MAX_LINE_INDICES];
        uint32_t offset = 0;

        for (int i = 0; i < sceneData->MAX_LINE_INDICES; i += 6) {
            lineIndices[i + 0] = offset + 0;
            lineIndices[i + 1] = offset + 1;
            lineIndices[i + 2] = offset + 2;

            lineIndices[i + 3] = offset + 2;
            lineIndices[i + 4] = offset + 3;
            lineIndices[i + 5] = offset + 0;

            offset += 4;
        }

        lineIB.reset(
            IndexBuffer::create(lineIndices, sceneData->MAX_LINE_INDICES));
        sceneData->lineVA->setIndexBuffer(lineIB);
        delete[] lineIndices;
    }

    static void init_quad_buffers() {
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
            // {"i_tilingfactor", BufferType::Float},
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
    }

    static void init(int width, int height) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        // glEnable(GL_DEPTH_TEST);
        glEnable(GL_LINE_SMOOTH);
        glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
        Renderer::setLineThickness(1.f);

        sceneData->width = width;
        sceneData->height = height;

        init_default_shaders();
        init_default_textures();

        init_quad_buffers();
        init_line_buffers();

        std::array<int, MAX_TEX> samples = {0};
        for (size_t i = 0; i < MAX_TEX; i++) {
            samples[(int)i] = (int)i;
        }
        auto textureShader = sceneData->shaderLibrary.get("texture");
        textureShader->bind();
        textureShader->uploadUniformIntArray("u_textures", samples.data(),
                                             MAX_TEX);

        auto line = sceneData->shaderLibrary.get("line");
        line->bind();
        // 0 no cap // 1 square // 2 round // 3 triangle
        line->uploadUniformInt("u_cap", 3);
    }

    static void resize(int width, int height) {
        glViewport(0, 0, width, height);
    }

    static void shutdown() {
        delete[] sceneData->qvbufferstart;
        delete sceneData;
    }

    static void clear(const glm::vec4& color) {
        prof give_me_a_name(__PROFILE_FUNC__);
        glClearColor(color.r, color.g, color.b, color.a);
        glClear(GL_COLOR_BUFFER_BIT |
                GL_DEPTH_BUFFER_BIT);  // Clear the buffers
    }

    static void draw(const std::shared_ptr<VertexArray>& vertexArray,
                     int indexCount = 0) {
        prof give_me_a_name(__PROFILE_FUNC__);
        int count =
            indexCount ? indexCount : vertexArray->indexBuffer->getCount();
        vertexArray->bind();
        glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_INT, nullptr);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    static void begin(OrthoCamera& cam) {
        prof give_me_a_name(__PROFILE_FUNC__);
        sceneData->viewProjection = cam.viewProjection;

        auto textureShader = sceneData->shaderLibrary.get("texture");
        textureShader->bind();
        textureShader->uploadUniformMat4("viewProjection",
                                         sceneData->viewProjection);

        auto lineShader = sceneData->shaderLibrary.get("line");
        lineShader->bind();

        start_batch();
    }

    static void end() {
        prof give_me_a_name(__PROFILE_FUNC__);
        flush();
    }

    static void start_batch() {
        sceneData->textureSlots[0] =
            dynamic_pointer_cast<Texture2D>(textureLibrary.get("white"));
        sceneData->quadIndexCount = 0;
        sceneData->qvbufferptr = sceneData->qvbufferstart;
        sceneData->nextTexSlot = 1;

        sceneData->lineIndexCount = 0;
        sceneData->lvbufferptr = sceneData->lvbufferstart;
    }

    static void next_batch() {
        flush();
        start_batch();
    }

    static void flush() {
        if (sceneData->quadIndexCount) {
            uint32_t dataSize = (uint32_t)((uint8_t*)sceneData->qvbufferptr -
                                           (uint8_t*)sceneData->qvbufferstart);
            sceneData->quadVB->setData(sceneData->qvbufferstart, dataSize);

            for (int i = 0; i < sceneData->nextTexSlot; i++)
                sceneData->textureSlots[i]->bind(i);

            sceneData->shaderLibrary.get("texture")->bind();

            draw(sceneData->quadVA, sceneData->quadIndexCount);
            stats.drawCalls++;
        }

        if (sceneData->lineIndexCount) {
            uint32_t dataSize = (uint32_t)((uint8_t*)sceneData->lvbufferptr -
                                           (uint8_t*)sceneData->lvbufferstart);
            sceneData->lineVB->setData(sceneData->lvbufferstart, dataSize);
            sceneData->shaderLibrary.get("line")->bind();
            draw(sceneData->lineVA, sceneData->lineIndexCount);
            stats.drawCalls++;
        }
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
        float textureIndex = 0.f;
        if (texture                        // tex is valid (ie not nullptr)
            && textureName != DEFAULT_TEX  // default tex is always loaded
            && textureStatus != -1  // set to default tex so already loaded
        ) {
            for (int i = 1; i < sceneData->nextTexSlot; i++) {
                if (*(sceneData->textureSlots[i]) == *texture) {
                    textureIndex = (float)i;
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
            sceneData->qvbufferptr->texindex = textureIndex;
            sceneData->qvbufferptr++;
        }
        sceneData->quadIndexCount += 6;

        stats.quadCount++;
    }

    static void drawLine(const glm::vec3& start, const glm::vec3& end,
                         float thickness = 1.f,
                         const glm::vec4& color = glm::vec4{1}) {
        //
        if (sceneData->lineIndexCount >= sceneData->MAX_LINE_INDICES)
            next_batch();

        // world to clip
        glm::vec4 clipI = glm::vec4(start.x, start.y, start.z, 1.0f);
        glm::vec4 clipJ = glm::vec4(end.x, end.y, end.z, 1.0f);

        // clip to pixel
        glm::vec2 pixeli, pixelj;
        pixeli.x = 0.5f * (float)sceneData->width * (clipI.x / clipI.w + 1.0f);
        pixeli.y = 0.5f * (float)sceneData->height * (1.0f - clipI.y / clipI.w);
        pixelj.x = 0.5f * (float)sceneData->width * (clipJ.x / clipJ.w + 1.0f);
        pixelj.y = 0.5f * (float)sceneData->height * (1.0f - clipJ.y / clipJ.w);

        glm::vec2 dir = {pixelj.x - pixeli.x, pixelj.y - pixeli.y};
        float linelength = glm::length(dir);

        if (linelength < 1e-10) return;

        dir /= linelength;
        glm::vec2 normal = {-dir.y, +dir.x};

        float d = 0.5f * thickness;

        float dOverwidth = d / (float)sceneData->width;
        float dOverHeight = d / (float)sceneData->height;

        glm::vec4 offset(0.0f);

        offset.x = (-dir.x + normal.x) * dOverwidth;
        offset.y = (+dir.y - normal.y) * dOverHeight;
        sceneData->lvbufferptr->clipcrd = clipI + offset;
        sceneData->lvbufferptr->color = color;
        sceneData->lvbufferptr->texcoord = {-d, +d};
        sceneData->lvbufferptr->width = 2 * d;
        sceneData->lvbufferptr->length = linelength;
        sceneData->lvbufferptr++;

        offset.x = (+dir.x + normal.x) * dOverwidth;
        offset.y = (-dir.y - normal.y) * dOverHeight;
        sceneData->lvbufferptr->clipcrd = clipJ + offset;
        sceneData->lvbufferptr->color = color;
        sceneData->lvbufferptr->texcoord = {linelength + d, +d};
        sceneData->lvbufferptr->width = 2 * d;
        sceneData->lvbufferptr->length = linelength;
        sceneData->lvbufferptr++;

        offset.x = (+dir.x - normal.x) * dOverwidth;
        offset.y = (-dir.y + normal.y) * dOverHeight;
        sceneData->lvbufferptr->clipcrd = clipJ + offset;
        sceneData->lvbufferptr->color = color;
        sceneData->lvbufferptr->texcoord = {linelength + d, -d};
        sceneData->lvbufferptr->width = 2 * d;
        sceneData->lvbufferptr->length = linelength;
        sceneData->lvbufferptr++;

        offset.x = (-dir.x - normal.x) * dOverwidth;
        offset.y = (+dir.y + normal.y) * dOverHeight;
        sceneData->lvbufferptr->clipcrd = clipI + offset;
        sceneData->lvbufferptr->color = color;
        sceneData->lvbufferptr->texcoord = {-d, -d};
        sceneData->lvbufferptr->width = 2 * d;
        sceneData->lvbufferptr->length = linelength;
        sceneData->lvbufferptr++;

        sceneData->lineIndexCount += 6;

        stats.quadCount++;
    }

    static void setLineThickness(float thickness) { glLineWidth(thickness); };

    ////// ////// ////// ////// ////// ////// ////// //////
    //      the draw calls below here, just call one of the ones above
    ////// ////// ////// ////// ////// ////// ////// //////

    static void drawLine(const glm::vec2& start, const glm::vec2& end,
                         float thickness, const glm::vec4& color) {
        Renderer::drawLine(glm::vec3{start, 0.f}, glm::vec3{end, 0.f},
                           thickness, color);
    }

    static void drawQuad(const glm::vec2& position, const glm::vec2& size,
                         const glm::vec4& color,
                         const std::string& textureName = DEFAULT_TEX) {
        Renderer::drawQuad(glm::vec3{position.x, position.y, 0.f}, size, color,
                           textureName);
    }

    static void drawQuad(const glm::vec3& position, const glm::vec2& size,
                         const glm::vec4& color,
                         const std::string& textureName = DEFAULT_TEX) {
        auto transform = glm::translate(im, position) *
                         glm::scale(im, {size.x, size.y, 1.0f});
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
        prof p(__PROFILE_FUNC__);

        auto transform = glm::translate(im, position) *
                         glm::rotate(im, angleInRad, {0.0f, 0.0f, 1.f}) *
                         // TODO We need this in order for the rotation to
                         // happen kinda close to the center of the object
                         glm::translate(im, {size.x / 4, size.y / 4, 0.f}) *
                         glm::scale(im, {size.x, size.y, 1.0f});

        drawQuad(transform, color, textureName);
    }
};
