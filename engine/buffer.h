#pragma once

#include "pch.hpp"

enum class BufferType {
    None = 0,
    Float,
    Float2,
    Float3,
    Float4,
    Mat3,
    Mat4,
    Int,
    Int2,
    Int3,
    Int4,
    Bool
};

struct BufferElem {
    std::string name;
    BufferType type;
    uintptr_t offset;
    int size;
    bool normalized;

    BufferElem(const std::string& n, BufferType t, bool norm = false)
        : name(n),
          type(t),
          offset(0),
          size(getDataTypeSize(t)),
          normalized(norm) {}

    inline GLenum typeToOpenGLType() const {
        switch (type) {
            case BufferType::Bool:
                return GL_BOOL;
            case BufferType::Float:
            case BufferType::Float2:
            case BufferType::Float3:
            case BufferType::Float4:
            case BufferType::Mat3:
            case BufferType::Mat4:
                return GL_FLOAT;
            default:
                log_error("Missing type conversion for {}", (int)type);
                return GL_FLOAT;
        }
    }

    inline int getDataTypeSize(BufferType t) const {
        switch (t) {
            case BufferType::Float:
                return 4;
            case BufferType::Float2:
                return 2 * 4;
            case BufferType::Float3:
                return 3 * 4;
            case BufferType::Float4:
                return 4 * 4;
            case BufferType::Mat3:
                return 3 * 3 * 4;
            case BufferType::Mat4:
                return 4 * 4 * 4;
            case BufferType::Bool:
                return 1;
            default:
                log_error("Missing buffer type size for {}", (int)t);
                return -1;
        }
    }

    int getCount() const {
        switch (type) {
            case BufferType::Bool:
            case BufferType::Float:
                return 1;
            case BufferType::Float2:
                return 2;
            case BufferType::Float3:
                return 3;
            case BufferType::Float4:
                return 4;
            case BufferType::Mat3:
                return 3;
            case BufferType::Mat4:
                return 4;
            default:
                log_error("Missing count for {}", (int)type);
                return -1;
        }
    }
};
struct BufferLayout {
    std::vector<BufferElem> elements;

    std::vector<BufferElem>::iterator begin() { return elements.begin(); }
    std::vector<BufferElem>::iterator end() { return elements.end(); }

    std::vector<BufferElem>::const_iterator begin() const {
        return elements.begin();
    }
    std::vector<BufferElem>::const_iterator end() const {
        return elements.end();
    }

    int stride;

    BufferLayout() {}

    BufferLayout(const std::initializer_list<BufferElem>& elem)
        : elements(elem) {
        int offset = 0;
        stride = 0;
        for (auto& element : elements) {
            element.offset = offset;
            offset += element.size;
            stride += element.size;
        }
    }
};

struct VertexBuffer {
    BufferLayout layout;

    virtual ~VertexBuffer() {}
    virtual void bind() const = 0;
    virtual void unbind() const = 0;
    virtual void setLayout(const BufferLayout& l) = 0;
    virtual void setData(void* data, int size) = 0;
    static VertexBuffer* create(float* verts, int size);
    static VertexBuffer* create(int size);
};

struct IndexBuffer {
    unsigned int count;

    virtual ~IndexBuffer() {}
    virtual void bind() const = 0;
    virtual void unbind() const = 0;
    virtual unsigned int getCount() const { return count; }

    static IndexBuffer* create(unsigned int* i_s, unsigned int count);
};

struct VertexArray {
    unsigned int rendererID;
    std::vector<std::shared_ptr<VertexBuffer>> vertexBuffers;
    std::shared_ptr<IndexBuffer> indexBuffer;

    virtual ~VertexArray() {}
    virtual void bind() const = 0;
    virtual void unbind() const = 0;
    virtual void addVertexBuffer(const std::shared_ptr<VertexBuffer>& vb) = 0;
    virtual void setIndexBuffer(const std::shared_ptr<IndexBuffer>& ib) = 0;
    static VertexArray* create();
};

struct OpenGLVertexArray : public VertexArray {
    OpenGLVertexArray() {
        glGenVertexArrays(1, &rendererID);
        glBindVertexArray(rendererID);
    }
    virtual void addVertexBuffer(
        const std::shared_ptr<VertexBuffer>& vb) override {
        M_ASSERT(vb->layout.elements.size(), "Layout cannot be empty");

        glBindVertexArray(rendererID);
        vb->bind();

        int index = 0;
        for (const auto& elem : vb->layout) {
            glEnableVertexAttribArray(index);
            glVertexAttribPointer(index, elem.getCount(),
                                  elem.typeToOpenGLType(),
                                  elem.normalized ? GL_TRUE : GL_FALSE,
                                  vb->layout.stride, (const void*)elem.offset);
            index++;
        }
        vertexBuffers.push_back(vb);
    }

    virtual void setIndexBuffer(
        const std::shared_ptr<IndexBuffer>& ib) override {
        glBindVertexArray(rendererID);
        ib->bind();
        indexBuffer = ib;
    }
    virtual ~OpenGLVertexArray() { glDeleteVertexArrays(1, &rendererID); }
    virtual void bind() const override { glBindVertexArray(rendererID); }
    virtual void unbind() const override { glBindVertexArray(0); }
};

struct OpenGLVertexBuffer : public VertexBuffer {
    unsigned int rendererID;
    OpenGLVertexBuffer(float* verts, unsigned int size) {
        glGenBuffers(1, &rendererID);
        glBindBuffer(GL_ARRAY_BUFFER, rendererID);
        glBufferData(GL_ARRAY_BUFFER, size, verts, GL_STATIC_DRAW);
    }
    OpenGLVertexBuffer(unsigned int size) {
        glGenBuffers(1, &rendererID);
        glBindBuffer(GL_ARRAY_BUFFER, rendererID);
        glBufferData(GL_ARRAY_BUFFER, size, nullptr, GL_DYNAMIC_DRAW);
    }
    virtual ~OpenGLVertexBuffer() { glDeleteBuffers(1, &rendererID); }
    virtual void bind() const override {
        glBindBuffer(GL_ARRAY_BUFFER, rendererID);
    }
    virtual void unbind() const override { glBindBuffer(GL_ARRAY_BUFFER, 0); }
    virtual void setLayout(const BufferLayout& l) override { layout = l; }
    virtual void setData(void* data, int size) override {
        glBindBuffer(GL_ARRAY_BUFFER, rendererID);
        glBufferSubData(GL_ARRAY_BUFFER, 0, size, data);
    }
};

struct OpenGLIndexBuffer : public IndexBuffer {
    unsigned int rendererID;
    OpenGLIndexBuffer(unsigned int* i_s, unsigned int c) {
        count = c;

        glGenBuffers(1, &rendererID);
        glBindBuffer(GL_ARRAY_BUFFER, rendererID);
        glBufferData(GL_ARRAY_BUFFER, count * sizeof(unsigned int), i_s,
                     GL_STATIC_DRAW);
    }
    virtual ~OpenGLIndexBuffer() { glDeleteBuffers(1, &rendererID); }
    virtual void bind() const override {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, rendererID);
    }
    virtual void unbind() const override {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    }
};

struct FrameBufferSpec {
    int width;
    int height;
    int samples = 1;

    bool swapChainTarget = false;
};

struct FrameBuffer {
    FrameBufferSpec spec;
    FrameBuffer(const FrameBufferSpec& s) : spec(s) {}
    virtual ~FrameBuffer() {}
    virtual void bind() = 0;
    virtual void unbind() = 0;

    // these are kinda ogl things?
    unsigned int rendererID;
    unsigned int colorAttachment;
    unsigned int depthAttachment;
};

struct OpenGLFrameBuffer : public FrameBuffer {
    OpenGLFrameBuffer(const FrameBufferSpec& s) : FrameBuffer(s) {}

    virtual void bind() override {
        glBindFramebuffer(GL_FRAMEBUFFER, rendererID);
    }
    virtual void unbind() override { glBindFramebuffer(GL_FRAMEBUFFER, 0); }

    void invalidate() {
        glCreateFramebuffers(1, &rendererID);
        glBindFramebuffer(GL_FRAMEBUFFER, rendererID);

        glCreateTextures(GL_TEXTURE_2D, 1, &colorAttachment);
        glBindTexture(GL_TEXTURE_2D, colorAttachment);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, spec.width, spec.height, 0,
                     GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                               GL_TEXTURE_2D, colorAttachment, 0);

        glCreateTextures(GL_TEXTURE_2D, 1, &depthAttachment);
        glBindTexture(GL_TEXTURE_2D, depthAttachment);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, spec.width,
                     spec.height, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8,
                     nullptr);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
                               GL_TEXTURE_2D, depthAttachment, 0);

        M_ASSERT(
            glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE,
            "Framebuffer was not complete");

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    virtual ~OpenGLFrameBuffer() {
        glDeleteFramebuffers(1, &rendererID);
        glDeleteTextures(1, &colorAttachment);
        glDeleteTextures(1, &depthAttachment);
    }
};
