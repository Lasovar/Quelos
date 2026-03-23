#pragma once

#include "Log.h"
#include "Quelos/Scenes/Actor.h"

#include "Quelos/Scenes/Entity.h"

template <>
struct fmt::formatter<Quelos::Entity> {
    constexpr auto parse(fmt::format_parse_context& ctx) { return ctx.begin(); }

    template <typename FormatContext>
    auto format(const Quelos::Entity& entity, FormatContext& ctx) {
        return fmt::format_to(ctx.out(), "Entity:{}({})", entity.GetName(), entity.GetInternalID());
    }
};

template <>
struct fmt::formatter<Quelos::GUID64> {
    constexpr auto parse(fmt::format_parse_context& ctx) { return ctx.begin(); }

    template <typename FormatContext>
    auto format(const Quelos::GUID64& guid, FormatContext& ctx) {
        return fmt::format_to(ctx.out(), "{:016X}", guid);
    }
};

template <>
struct fmt::formatter<Quelos::GUID128> {
    constexpr auto parse(format_parse_context& ctx) {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(const Quelos::GUID128& g, FormatContext& ctx) {
        return fmt::format_to(ctx.out(), "{}", g.ToString());
    }
};

template <>
struct fmt::formatter<Quelos::Actor> {
    constexpr auto parse(fmt::format_parse_context& ctx) { return ctx.begin(); }

    template <typename FormatContext>
    auto format(const Quelos::Actor& actor, FormatContext& ctx) {
        return fmt::format_to(ctx.out(), "Actor:{}({},{})", actor.GetName(), actor.GetActorID(), actor.GetInternalID());
    }
};
