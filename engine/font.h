
#pragma once

#include <stdio.h>
#include <stdlib.h>

#include "pch.hpp"

const int START_CODEPOINT = ' ';
const int END_CODEPOINT = 200;
const int FONT_SIZE = 64;
const int ITEMS_PER_ROW = 50;

struct BMPImage {
    std::shared_ptr<int[]> pixels;
    int width;
    int height;
    float aspect;
};

struct Glyph {
    std::string textureName;
    // TODO replace with texture
    BMPImage image;

    int codepoint;
    float advance;
    float leftbearing;
    float xoff;
    float yoff;
    glm::vec2 minUV;
    glm::vec2 maxUV;
};

struct Font {
    std::string name;
    std::vector<Glyph> glyphs;
    std::vector<float> kerning;

    // how tall the tallest letter is
    float ascent;
    // how deep the deepest letter is
    float descent;
    // recommended vert distance from decent to acent of next line
    float linegap;
    // ??? actual vertical distance?
    float lineadvance;

    int size;
};
static std::map<std::string, std::shared_ptr<Font> > fonts;

// takes 2 codepoints and finds the kerning needed for it
inline float getKerning(const std::shared_ptr<Font> font, int first_cp,
                        int second_cp) {
    int glyphIndex = first_cp - ' ';
    int nextIndex = -1;
    if (second_cp != 0) {
        nextIndex = second_cp - ' ';
    }
    float kerning = 0.f;
    if (nextIndex != -1) {
        kerning = font->kerning[glyphIndex * font->glyphs.size() + nextIndex];
    }
    return kerning;
}

inline uint32_t packRGBA(glm::vec4 color) {
    return (uint32_t)((color.r * 255.0f + 0.5f)) |
           ((uint32_t)((color.g * 255.0f) + 0.5f) << 8) |
           ((uint32_t)((color.b * 255.0f) + 0.5f) << 16) |
           ((uint32_t)((color.a * 255.0f) + 0.5f) << 24);
}

void generate_font_texture(const char* filename) {
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    if (size == -1) {
        log_error("Failed to load font {}, file not found", filename);
        return;
    }

    std::string fontname = nameFromFilePath(filename);

    // TODO are the font files bigger than this?
    char* buffer = (char*)calloc(size, sizeof(unsigned char));
    if (file.read(buffer, size)) {
        auto fontBuffer = (unsigned char*)buffer;

        /* Initialize font */
        stbtt_fontinfo info;
        if (!stbtt_InitFont(&info, fontBuffer, 0)) {
            log_error("Failed to load font {}, file not found", filename);
        }

        /* Calculate font scaling */
        float pixels = 1.f * FONT_SIZE; /* Font size (font size) */
        float scale = stbtt_ScaleForPixelHeight(
            &info, pixels); /* scale = pixels / (ascent - descent) */

        /* create a bitmap */
        int bitmap_w = pixels * ITEMS_PER_ROW; /* Width of bitmap */
        int bitmap_h = pixels * ((END_CODEPOINT - START_CODEPOINT) /
                                 ITEMS_PER_ROW); /* Height of bitmap */
        unsigned char* bitmap =
            (unsigned char*)calloc(bitmap_w * bitmap_h, sizeof(unsigned char));

        /**
         * Get the measurement in the vertical direction
         * ascent: The height of the font from the baseline to the top;
         * descent: The height from baseline to bottom is usually negative;
         * lineGap: The distance between two fonts;
         * The line spacing is: ascent - descent + lineGap.
         */
        int ascent = 0;
        int descent = 0;
        int lineGap = 0;
        stbtt_GetFontVMetrics(&info, &ascent, &descent, &lineGap);

        /* Adjust word height according to zoom */
        ascent = roundf(ascent * scale);
        descent = roundf(descent * scale);

        int x = 0; /*x of bitmap*/
        int ys = 0;

        /* Cyclic loading of each character in word */
        for (int codepoint = START_CODEPOINT; codepoint <= END_CODEPOINT;
             codepoint++) {
            /**
             * Get the measurement in the horizontal direction
             * advanceWidth: Word width;
             * leftSideBearing: Left side position;
             */
            int advanceWidth = 0;
            int leftSideBearing = 0;
            stbtt_GetCodepointHMetrics(&info, codepoint, &advanceWidth,
                                       &leftSideBearing);

            /* Gets the border of a character */
            int c_x1, c_y1, c_x2, c_y2;
            stbtt_GetCodepointBitmapBox(&info, codepoint, scale, scale, &c_x1,
                                        &c_y1, &c_x2, &c_y2);

            /* Calculate the y of the bitmap (different characters have
             * different heights) */
            int y = ys + ascent + c_y1;

            /* Render character */
            int byteOffset =
                x + roundf(leftSideBearing * scale) + (y * bitmap_w);
            stbtt_MakeCodepointBitmap(&info, bitmap + byteOffset, c_x2 - c_x1,
                                      c_y2 - c_y1, bitmap_w, scale, scale,
                                      codepoint);

            /* Adjust x */
            x += FONT_SIZE;

            if (x >= bitmap_w) {
                x = 0;
                ys += FONT_SIZE;
            }
        }

        /* Save the bitmap data to the 1-channel png image */
        std::string out_filename = fmt::format("./resources/{}.png", fontname);
        stbi_write_png(out_filename.c_str(), bitmap_w, bitmap_h, 1, bitmap,
                       bitmap_w);

        free(fontBuffer);
        free(bitmap);
    }
}

void load_font_texture(const char* filename) {
    Renderer::addTexture(filename);

    std::string fontname = nameFromFilePath(filename);
    auto fonttex = textureLibrary.get(fontname);

    int i = 0;
    int j = 0;
    int max_y = 1 + (END_CODEPOINT / ITEMS_PER_ROW);

    log_info("font name {}", fontname);
    for (int codepoint = START_CODEPOINT; codepoint <= END_CODEPOINT;
         codepoint++) {
        auto texname = fmt::format("{}_{}", fontname, codepoint);
        textureLibrary.addSubtexture(fonttex->name, texname, i, max_y - j,
                                     FONT_SIZE, FONT_SIZE);
        if (i >= ITEMS_PER_ROW) {
            j++;
            i = 0;
        }
        i++;
    }
}

void load_font_file(const char* filename) {
    log_info("loading font {}", filename);

    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    if (size == -1) {
        log_error("Failed to load font {}, file not found", filename);
        return;
    }

    // TODO are the font files bigger than this?
    char buffer[1 << 25];
    if (file.read(buffer, size)) {
        // stb_tt wants unsigned char :shrug:
        unsigned char* ttf_buffer = (unsigned char*)buffer;

        // make a new font
        std::shared_ptr<Font> font = std::make_shared<Font>();
        font->name = nameFromFilePath(filename);
        font->size = 100;

        stbtt_fontinfo info;
        stbtt_InitFont(&info, ttf_buffer, 0);
        float scale = stbtt_ScaleForPixelHeight(&info, font->size);
        int s_ascent, s_decent, s_linegap;
        stbtt_GetFontVMetrics(&info, &s_ascent, &s_decent, &s_linegap);

        font->ascent = 1.f * s_ascent * scale;
        font->descent = 1.f * s_decent * scale;
        font->linegap = 1.f * s_linegap * scale;
        font->lineadvance = font->ascent - font->descent + font->linegap;

        unsigned char* bitmap;

        for (int codepoint = ' '; codepoint <= '~'; codepoint++) {
            int s_advance;
            int s_leftbearing;
            stbtt_GetCodepointHMetrics(&info, codepoint, &s_advance,
                                       &s_leftbearing);
            float scale = stbtt_ScaleForPixelHeight(&info, font->size);

            int s_w, s_h;
            int s_xoff, s_yoff;
            bitmap = stbtt_GetCodepointBitmap(&info, 0, scale, codepoint, &s_w,
                                              &s_h, &s_xoff, &s_yoff);

            int border = 3;
            int glyph_width = s_w + 2 * border;
            int glyph_height = s_h + 2 * border;
            int img_size = glyph_width * glyph_height;

            std::shared_ptr<int[]> image(new int[img_size]);

            int j, i;
            for (j = 0; j < s_h; ++j) {
                for (i = 0; i < s_w; ++i) {
                    int y = s_h - j;
                    int px = (j + border) * glyph_width + (i + border);
                    float val = bitmap[y * s_w + i] / 255.f;
                    glm::vec4 col = glm::vec4{val};
                    int packed = packRGBA(col);
                    image[px] = packed;
                }
            }

            for (size_t i = 0; i < font->glyphs.size(); i++) {
                for (size_t j = 0; j < font->glyphs.size(); j++) {
                    // TODO actually put kern in the list,
                    // not doing it now since need to make the index correctly
                    //
                    // int idx = i * font->glyphs.size() + j;
                    // int a = font->glyphs[i].codepoint;
                    // int b = font->glyphs[j].codepoint;
                    // int kern = stbtt_GetCodepointKernAdvance(&info, a, b);
                    font->kerning.push_back(10.f);
                }
            }

            std::string texname =
                fmt::format("{}_{}", font->name, std::to_string(codepoint));
            std::shared_ptr<Texture> letterTexture =
                std::make_shared<Texture2D>(texname, glyph_width, glyph_height);
            letterTexture->setBitmapData((void*)image.get());
            letterTexture->tilingFactor = 1.f;
            textureLibrary.add(letterTexture);

            Glyph g;
            g.textureName = texname;
            g.codepoint = codepoint;
            g.advance = 1.f * s_advance * scale;
            g.leftbearing = 1.f * s_leftbearing * scale;
            g.xoff = s_xoff - border;
            g.yoff = s_yoff - border;
            g.image = BMPImage({
                .width = glyph_width,                         //
                .height = glyph_height,                       //
                .pixels = image,                              //
                .aspect = (1.f * glyph_width) / glyph_height  //
            });
            font->glyphs.push_back(g);
            stbtt_FreeBitmap(bitmap, 0);
        }

        fonts[font->name] = font;
    }
}

