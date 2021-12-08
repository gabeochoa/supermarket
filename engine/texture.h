
#pragma once

#include "pch.hpp"

struct Texture {
    std::string name;
    int width;
    int height;
    int textureIndex;
    float tilingFactor;

    Texture()
        : name("TEXTURE_HAS_NO_NAME"),
          width(0),
          height(0),
          textureIndex(0),
          tilingFactor(1.f) {}
    Texture(const std::string &n, int w, int h, int texIndex)
        : name(n),
          width(w),
          height(h),
          textureIndex(texIndex),
          tilingFactor(1.f) {}
    virtual void setData(void *data) { (void)data; }

    virtual ~Texture() {}
    // its const from a C++ pov but
    // its not really const if you know what i mean
    virtual void bind() const = 0;
};

struct Texture2D : public Texture {
    unsigned int rendererID;

    Texture2D(const std::string &name, int w, int h, int texIndex = 0)
        : Texture(name, w, h, texIndex) {
        glGenTextures(1, &rendererID);
        glActiveTexture(GL_TEXTURE0 + textureIndex);
        glBindTexture(GL_TEXTURE_2D, rendererID);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    }
    virtual void setData(void *data) override {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA,
                     GL_UNSIGNED_BYTE, data);
    }

    Texture2D(const std::string &path, int texIndex = 0) : Texture() {
        log_trace(fmt::format("Loading texture: {}", path));

        name = nameFromFilePath(path);
        textureIndex = texIndex;

        int w, h, channels;
        stbi_set_flip_vertically_on_load(1);
        stbi_uc *data = stbi_load(path.c_str(), &w, &h, &channels, 0);
        M_ASSERT(data, fmt::format("Failed to load texture2d: {}", path));

        GLenum internalFormat = 0, dataFormat = 0;
        if (channels == 4) {
            internalFormat = GL_RGBA8;
            dataFormat = GL_RGBA;
        } else if (channels == 3) {
            internalFormat = GL_RGB8;
            dataFormat = GL_RGB;
        }
        M_ASSERT(internalFormat,
                 fmt::format("image format not supported: {}", channels));
        log_trace(fmt::format("texture {} has {} channels", path, channels));

        width = w;
        height = h;

        glGenTextures(1, &rendererID);
        glActiveTexture(GL_TEXTURE0 + textureIndex);
        glBindTexture(GL_TEXTURE_2D, rendererID);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0,
                     dataFormat, GL_UNSIGNED_BYTE, data);

        stbi_image_free(data);
    }

    virtual ~Texture2D() { glDeleteTextures(1, &rendererID); }
    virtual void bind() const override {
        glBindTexture(textureIndex, rendererID);
    }
};

struct TextureLibrary {
    std::unordered_map<std::string, std::shared_ptr<Texture>> textures;

    void add(const std::shared_ptr<Texture> &texture) {
        if (textures.find(texture->name) != textures.end()) {
            log_warn(fmt::format(
                "Failed to add texture to library, texture with name "
                "{} already exists",
                texture->name));
            return;
        }
        log_trace(
            fmt::format("Adding Texture \"{}\" to our library", texture->name));
        textures[texture->name] = texture;
    }
    std::shared_ptr<Texture> load(const std::string &path,
                                  int activeTextureIndex = 0) {
        auto texture = std::make_shared<Texture2D>(path, activeTextureIndex);
        add(texture);
        return texture;
    }

    std::unordered_map<std::string, std::shared_ptr<Texture>>::iterator
    begin() {
        return textures.begin();
    }

    std::unordered_map<std::string, std::shared_ptr<Texture>>::iterator end() {
        return textures.end();
    }

    std::unordered_map<std::string, std::shared_ptr<Texture>>::const_iterator
    begin() const {
        return textures.begin();
    }

    std::unordered_map<std::string, std::shared_ptr<Texture>>::const_iterator
    end() const {
        return textures.end();
    }

    std::shared_ptr<Texture> get(const std::string &name) {
        return textures[name];
    }
};

