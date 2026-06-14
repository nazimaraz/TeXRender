#pragma once

#include <cmath>

namespace TeXRender
{
    class Transform2D
    {
    public:
        Transform2D() = default;

        void translate(const float dx, const float dy)
        {
            concat(1.f, 0.f, 0.f, 1.f, dx, dy);
        }

        void scale(const float sx, const float sy)
        {
            concat(sx, 0.f, 0.f, sy, 0.f, 0.f);
        }

        void rotate(const float theta)
        {
            const auto cs = std::cos(theta);
            const auto sn = std::sin(theta);
            concat(cs, sn, -sn, cs, 0.f, 0.f);
        }

        void rotate(const float theta, const float px, const float py)
        {
            translate(px, py);
            rotate(theta);
            translate(-px, -py);
        }

        void reset()
        {
            *this = Transform2D{};
        }

        [[nodiscard]] float apply_x(const float x, const float y) const
        {
            return a_ * x + c_ * y + e_;
        }

        [[nodiscard]] float apply_y(const float x, const float y) const
        {
            return b_ * x + d_ * y + f_;
        }

        [[nodiscard]] float scale_x() const
        {
            return std::hypot(a_, b_);
        }

        [[nodiscard]] float scale_y() const
        {
            return std::hypot(c_, d_);
        }

        [[nodiscard]] float rotation() const
        {
            return std::atan2(b_, a_);
        }

    private:
        void concat(const float a2, const float b2, const float c2, const float d2, const float e2, const float f2)
        {
            const auto na = a_ * a2 + c_ * b2;
            const auto nb = b_ * a2 + d_ * b2;
            const auto nc = a_ * c2 + c_ * d2;
            const auto nd = b_ * c2 + d_ * d2;
            const auto ne = a_ * e2 + c_ * f2 + e_;
            const auto nf = b_ * e2 + d_ * f2 + f_;
            a_ = na;
            b_ = nb;
            c_ = nc;
            d_ = nd;
            e_ = ne;
            f_ = nf;
        }

        float a_ = 1.f;
        float b_ = 0.f;
        float c_ = 0.f;
        float d_ = 1.f;
        float e_ = 0.f;
        float f_ = 0.f;
    };
} // namespace TeXRender
