#pragma once

#include "renderer/renderer.h"
#include <memory>

namespace gui {

class Widget {
public:
    Widget(std::shared_ptr<renderer::Renderer> renderer);

    virtual void draw(int width, int height) = 0;

protected:
    std::shared_ptr<renderer::Renderer> renderer;
};

}
