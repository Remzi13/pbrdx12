#pragma once
#include <cstdint>
#include <deque>
#include <optional>
#include <chrono>

struct InputEvent
{
    enum class Type : uint8_t
    {
        KeyDown,
        KeyUp,
        Char,       // WM_CHAR
        MouseMove,
        MouseDown,
        MouseUp,
        MouseWheel,
        FocusLost,
        FocusGained
    } type;

    // keyboard:
    uint32_t key = 0;      // virtual-key code for KeyDown/KeyUp
    uint32_t modifiers = 0; // bitmask: 1=SHIFT,2=CTRL,4=ALT

    // char:
    wchar_t ch = 0;

    // mouse:
    int32_t x = 0;
    int32_t y = 0;
    uint8_t mouse_button = 0; // 1 = left, 2 = right, 3 = middle
    int32_t wheel_delta = 0;  // for MouseWheel
};

namespace Input
{
    // Capacity of internal buffer (oldest events dropped when full)
    constexpr size_t BUFFER_CAPACITY = 1024;

    // push helpers used from WindowProc:
    void pushEvent(const InputEvent& e);

    // Query / consume from App:
    bool empty();                     // true if no events
    size_t size();                     // current buffered events
    std::optional<InputEvent> pop();   // pop oldest event (returns nullopt if empty)
    void clear();                      // clear buffer
}
