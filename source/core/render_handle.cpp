#include "texgui/render_handle.hpp"
#include "render.h"
#include "core/document_state.hpp"
#include "render/graphics2d_adapter.hpp"

using namespace TexGui;

RenderHandle::RenderHandle(tex::TeXRender* render, std::shared_ptr<detail::DocumentState> state)
    : state_{std::move(state)}
    , render_{render}
{}

RenderHandle::~RenderHandle() = default;

RenderHandle::RenderHandle(RenderHandle&&) noexcept = default;

RenderHandle& RenderHandle::operator=(RenderHandle&& other) noexcept
{
    if (this != &other)
    {
        render_ = std::move(other.render_);
        state_ = std::move(other.state_);
    }

    return *this;
}

int RenderHandle::width() const
{
    if (!render_)
        return 0;

    return render_->getWidth();
}

int RenderHandle::height() const
{
    if (!render_)
        return 0;

    return render_->getHeight();
}

int RenderHandle::depth() const
{
    if (!render_)
        return 0;

    return render_->getDepth();
}

float RenderHandle::baseline() const
{
    if (!render_)
        return 0.f;

    return render_->getBaseline();
}

void RenderHandle::set_foreground(const uint32_t argb) const
{
    if (!render_)
        return;

    render_->setForeground(argb);
}

void RenderHandle::draw(const int x, const int y) const
{
    if (!render_ || !state_)
        return;

    auto* const surface = state_->surface();
    if (!surface)
        return;

    auto adapter = Graphics2DAdapter{*surface};
    render_->draw(adapter, x, y);
}
