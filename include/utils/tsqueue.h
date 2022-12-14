#pragma once
#include <queue>
#include <mutex>
#include <condition_variable>
#include <optional>
#include <chrono>

namespace warpgate::utils {
    // A threadsafe-queue.
    template <class T>
    class tsqueue
    {
    public:
        tsqueue(void);
        ~tsqueue();

        // Add an element to the queue.
        void enqueue(T t);

        // Get the "front"-element.
        // If the queue is empty, wait till a element is avaiable.
        T dequeue(void);
        T try_dequeue(T def);
        std::optional<T> try_dequeue_for(const std::chrono::milliseconds ms);

        bool is_closed(void);
        void close(void);

    private:
        std::queue<T> q;
        mutable std::mutex m;
        std::condition_variable c;
        bool closed;
    };
}