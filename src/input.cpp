#include "input.h"
#include <mutex>

namespace {
    std::deque<InputEvent> g_queue;
    std::mutex g_mutex; // not strictly required for single-threaded apps, but cheap protection
}

namespace Input
{
    void pushEvent(const InputEvent& e)
    {
        std::lock_guard<std::mutex> lg(g_mutex);
        if (g_queue.size() >= BUFFER_CAPACITY) {
            // drop oldest
            g_queue.pop_front();
        }
        g_queue.push_back(e);
    }

    bool empty()
    {
        std::lock_guard<std::mutex> lg(g_mutex);
        return g_queue.empty();
    }

    size_t size()
    {
        std::lock_guard<std::mutex> lg(g_mutex);
        return g_queue.size();
    }

    std::optional<InputEvent> pop()
    {
        std::lock_guard<std::mutex> lg(g_mutex);
        if (g_queue.empty()) return std::nullopt;
        InputEvent e = std::move(g_queue.front());
        g_queue.pop_front();
        return e;
    }

    void clear()
    {
        std::lock_guard<std::mutex> lg(g_mutex);
        g_queue.clear();
    }
}
