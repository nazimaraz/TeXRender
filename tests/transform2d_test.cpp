#include <cmath>
#include <numbers>
#include <print>
#include "render/transform2d.hpp"

using TexGui::Transform2D;

namespace
{
    int g_failures = 0;

    void check(const bool ok, const char* const expr)
    {
        if (!ok)
        {
            std::println("FAIL: {}", expr);
            ++g_failures;
        }
    }

    bool approx(const float a, const float b)
    {
        return std::fabs(a - b) < 1e-4f;
    }

    constexpr float k_pi = std::numbers::pi_v<float>;
} // namespace

#define CHECK(cond) check((cond), #cond)

int main()
{
    {
        Transform2D t;
        CHECK(approx(t.apply_x(3.f, 4.f), 3.f));
        CHECK(approx(t.apply_y(3.f, 4.f), 4.f));
        CHECK(approx(t.scale_x(), 1.f));
        CHECK(approx(t.scale_y(), 1.f));
        CHECK(approx(t.rotation(), 0.f));
    }
    {
        Transform2D t;
        t.translate(10.f, 20.f);
        CHECK(approx(t.apply_x(1.f, 1.f), 11.f));
        CHECK(approx(t.apply_y(1.f, 1.f), 21.f));
    }
    {
        Transform2D t;
        t.scale(2.f, 3.f);
        CHECK(approx(t.apply_x(4.f, 5.f), 8.f));
        CHECK(approx(t.apply_y(4.f, 5.f), 15.f));
        CHECK(approx(t.scale_x(), 2.f));
        CHECK(approx(t.scale_y(), 3.f));
    }
    {
        Transform2D t;
        t.translate(10.f, 20.f);
        t.scale(2.f, 3.f);
        CHECK(approx(t.apply_x(1.f, 1.f), 12.f));
        CHECK(approx(t.apply_y(1.f, 1.f), 23.f));
    }
    {
        Transform2D t;
        t.rotate(k_pi / 2.f);
        CHECK(approx(t.apply_x(1.f, 0.f), 0.f));
        CHECK(approx(t.apply_y(1.f, 0.f), 1.f));
        CHECK(approx(t.scale_x(), 1.f));
        CHECK(approx(t.scale_y(), 1.f));
        CHECK(approx(t.rotation(), k_pi / 2.f));
    }
    {
        Transform2D t;
        t.rotate(k_pi / 2.f, 5.f, 5.f);
        CHECK(approx(t.apply_x(6.f, 5.f), 5.f));
        CHECK(approx(t.apply_y(6.f, 5.f), 6.f));
    }
    {
        Transform2D t;
        t.scale(2.f, 2.f);
        t.reset();
        CHECK(approx(t.apply_x(7.f, 8.f), 7.f));
        CHECK(approx(t.apply_y(7.f, 8.f), 8.f));
    }

    if (g_failures == 0)
        std::println("all transform2d tests passed");

    return g_failures == 0 ? 0 : 1;
}
