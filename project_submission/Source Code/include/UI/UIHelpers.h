#pragma once
#include <SFML/Graphics.hpp>
#include <algorithm>
#include <numbers>
#include <cmath>

namespace UI::Helper {
    inline sf::Color brighten(const sf::Color& c, int delta) {
        return sf::Color(
            static_cast<std::uint8_t>(std::min(255, static_cast<int>(c.r) + delta)),
            static_cast<std::uint8_t>(std::min(255, static_cast<int>(c.g) + delta)),
            static_cast<std::uint8_t>(std::min(255, static_cast<int>(c.b) + delta)),
            c.a
        );
    }

    inline sf::ConvexShape makePill(const sf::Vector2f& pos, float w, float h, const sf::Color& fill, const sf::Color& outline, float thick, int ppc = 8) {
        sf::ConvexShape s;
        s.setPointCount(ppc * 4);
        float r = h * 0.5f;
        const float pi = std::numbers::pi_v<float>;
        int idx = 0;
        auto addCorner = [&](float cx, float cy, float start) {
            for (int i = 0; i < ppc; ++i) {
                float a = start + (static_cast<float>(i) * (pi * 0.5f) / static_cast<float>(ppc - 1));
                s.setPoint(idx++, sf::Vector2f(cx + r * std::cos(a), cy + r * std::sin(a)));
            }
        };
        addCorner(w - r, h - r, 0.f);
        addCorner(r, h - r, pi * 0.5f);
        addCorner(r, r, pi);
        addCorner(w - r, r, pi * 1.5f);
        s.setPosition(pos);
        s.setFillColor(fill);
        s.setOutlineThickness(thick);
        s.setOutlineColor(outline);
        return s;
    }

    inline sf::ConvexShape makeRoundedRect(const sf::Vector2f& pos, const sf::Vector2f& size, float radius, int ppc = 10) {
        sf::ConvexShape s;
        s.setPointCount(ppc * 4);
        float r = std::min({radius, size.x * 0.5f, size.y * 0.5f});
        const float pi = std::numbers::pi_v<float>;
        int idx = 0;
        auto addCorner = [&](float cx, float cy, float start) {
            for (int i = 0; i < ppc; ++i) {
                float a = start + (static_cast<float>(i) * (pi * 0.5f) / static_cast<float>(ppc - 1));
                s.setPoint(idx++, sf::Vector2f(cx + r * std::cos(a), cy + r * std::sin(a)));
            }
        };
        addCorner(size.x - r, size.y - r, 0.f);
        addCorner(r, size.y - r, pi * 0.5f);
        addCorner(r, r, pi);
        addCorner(size.x - r, r, pi * 1.5f);
        s.setPosition(pos);
        return s;
    }

    struct TrackGeom {
        float x, y, w, h, r;
        static TrackGeom fromBar(const sf::Vector2f& drawPos, const sf::Vector2f& baseSize, float h = 12.f, float pad = 16.f) {
            return {drawPos.x + pad, drawPos.y + baseSize.y - 22.f, baseSize.x - pad*2.f, h, h*0.5f};
        }
        static TrackGeom fromPureBar(const sf::Vector2f& drawPos, const sf::Vector2f& baseSize) {
            return fromBar(drawPos, baseSize, 12.f, 16.f);
        }
    };
}
