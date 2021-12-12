#include "buffer.h"

#include "pch.hpp"

VertexArray* VertexArray::create() { return new OpenGLVertexArray(); }
VertexBuffer* VertexBuffer::create(float* verts, int size) {
    return new OpenGLVertexBuffer(verts, size);
}

VertexBuffer* VertexBuffer::create(int size) {
    return new OpenGLVertexBuffer(size);
}

IndexBuffer* IndexBuffer::create(unsigned int* i_s, unsigned int count) {
    return new OpenGLIndexBuffer(i_s, count);
}
