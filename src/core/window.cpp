#include "window.h"
#include "boot_log.h"
#include "screen.h"
#include "video_settings.h"

namespace oci {

static Window::VideoCallback s_video_callback = nullptr;

Window& Window::Instance() {
    static Window instance;
    return instance;
}

Window::Window()
    : mIsOpen(false), mSize(640, 480), mFramerateLimit(25) {
}

void Window::SetVideoCallback(VideoCallback cb) {
    s_video_callback = cb;
}

void Window::create(VideoMode mode, const std::string& /*title*/, unsigned int /*style*/) {
    mSize = Vector2u(mode.Width(), mode.Height());
    mIsOpen = true;
    core::Screen::Instance().Clear(0);
}

void Window::close() {
    mIsOpen = false;
}

void Window::display() {
    static int s_logged_mode = -1;
    bool is_640 = (VideoSettings::GetResolution() == VideoResolution::Res640x480);
    if(s_logged_mode != (is_640 ? 1 : 0)) {
        s_logged_mode = is_640 ? 1 : 0;
        BootLog("video mode: %s", is_640 ? "640x480" : "320x240");
    }

    if(!is_640) {
        core::Screen::Instance().DownscaleToOutput(mOutput320);
    }
    present();
}

void Window::present() {
    if(!s_video_callback)
        return;

    if(VideoSettings::GetResolution() == VideoResolution::Res640x480) {
        s_video_callback(
            core::Screen::Instance().GetBuffer(),
            core::Screen::WIDTH,
            core::Screen::HEIGHT,
            core::Screen::WIDTH * sizeof(uint16_t)
        );
    } else {
        s_video_callback(
            mOutput320,
            core::Screen::OUT_WIDTH,
            core::Screen::OUT_HEIGHT,
            core::Screen::OUT_WIDTH * sizeof(uint16_t)
        );
    }
}

void Window::draw(const Drawable& obj) {
    obj.DoDraw(nullptr);
}

void Window::clear() {
    core::Screen::Instance().Clear(0);
}

void Window::setMouseCursorVisible(bool /*visible*/) {
}

bool Window::pollEvent(Event& /*event_received*/) {
    return false;
}

void Window::setFramerateLimit(unsigned int limit) {
    mFramerateLimit = limit;
}

} // namespace oci
