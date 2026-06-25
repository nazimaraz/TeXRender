#pragma once

#include <string>
#include "graphic/graphic.h"

namespace TeXRender
{
    class FtFont final : public microtex::Font
    {
    public:
        explicit FtFont(std::string path);

        [[nodiscard]] bool operator==(const Font& f) const override;
        [[nodiscard]] bool operator!=(const Font& f) const;
        [[nodiscard]] const std::string& path() const;

    private:
        std::string path_;
    };
} // namespace TeXRender
