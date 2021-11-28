#pragma once

#include <atomic>
#include <mutex>
#include <type_traits>

namespace util {

    class spinlock
    {
        // deperecated in C++20 as ctor defaults to zero. MSVC ctor seems to do it, but just to be safe.
        std::atomic_flag _lock = ATOMIC_FLAG_INIT; 

    public:

        spinlock() {
            // Likely the old's code here _lock._My_Val have been also an long.
            static_assert (sizeof (_lock._Storage._Storage._Value) == sizeof (long), "ABI compatibility, check serialization.");
        }

        void lock() {
            while(_lock.test_and_set(std::memory_order_acquire))  // acquire lock
                ; // spin
        }

        void unlock() {
            _lock.clear(std::memory_order_release);  
        }

        typedef std::lock_guard<spinlock> guard;
    };
}
