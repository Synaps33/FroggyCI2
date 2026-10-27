#pragma once

#include "drawable.h"
#include "event.h"
#include "vector2.h"
#include "video_mode.h"
#include <cstdint>
#include <string>

namespace oci {

class Window {
public:
    static Window& Instance();

    void create(VideoMode mode, const std::string& title, unsigned int style = 0);
    void close();
    void display();
    // Push the last frame again without redrawing (frame repeat).
    void present();
    void draw(const Drawable& obj);
    void clear();

    void setMouseCursorVisible(bool visible);
    bool pollEvent(Event& event_received);
    bool isOpen() const noexcept { return mIsOpen; }

    Vector2u getSize() const noexcept { return mSize; }

    void setFramerateLimit(unsigned int limit);

    using VideoCallback = void (*)(const void* data, unsigned width, unsigned height, size_t pitch);
    static void SetVideoCallback(VideoCallback cb);

private:
    Window();
    bool mIsOpen;
    Vector2u mSize;
    unsigned int mFramerateLimit;
    uint16_t mOutput320[320 * 240];
};

} // namespace oci
