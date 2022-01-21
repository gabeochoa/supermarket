#pragma once

#include "../vendor/supermarket-engine/engine/pch.hpp"

///////////// ///////////// ///////////// /////////////
//
//
//
//
//
//  This is only for types that arent created by the Super project
//
//  for types that we are defining, just keep the formatter local to the struct
//  defn
//
//
//
//
//
//
///////////// ///////////// ///////////// /////////////

template <>
struct fmt::formatter<glm::vec2> {
    template <typename ParseContext>
    constexpr auto parse(ParseContext& ctx) {
        return ctx.begin();
    }

    template <typename FormatContext>
    constexpr auto format(glm::vec2 const& v, FormatContext& ctx) {
        return fmt::format_to(ctx.out(), "({:.2f},{:.2f})", v.x, v.y);
    }
};

template <>
struct fmt::formatter<glm::vec3> {
    template <typename ParseContext>
    constexpr auto parse(ParseContext& ctx) {
        return ctx.begin();
    }

    template <typename FormatContext>
    constexpr auto format(glm::vec3 const& v, FormatContext& ctx) {
        return fmt::format_to(ctx.out(), "({:.2f},{:.2f},{:.2f})", v.x, v.y,
                              v.z);
    }
};

template <>
struct fmt::formatter<glm::vec4> {
    template <typename ParseContext>
    constexpr auto parse(ParseContext& ctx) {
        return ctx.begin();
    }

    template <typename FormatContext>
    constexpr auto format(glm::vec4 const& v, FormatContext& ctx) {
        return fmt::format_to(ctx.out(), "({:.2f},{:.2f},{:.2f},{:.2f})", v.x,
                              v.y, v.z, v.w);
    }
};

