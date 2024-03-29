#include "utils/tsqueue.h"
#include "parameter.h"
#include <filesystem>

using namespace warpgate;

template <class T>
utils::tsqueue<T>::tsqueue(void): q(), m(), c(), closed(false) {}

template <class T>
utils::tsqueue<T>::~tsqueue(void) {}

template <class T>
void utils::tsqueue<T>::enqueue(T t) {
    if(closed) {
        return;
    }
    std::lock_guard<std::mutex> lock(m);
    q.push(t);
    c.notify_one();
}

template <class T>
T utils::tsqueue<T>::dequeue(void) {
    std::unique_lock<std::mutex> lock(m);
    while (q.empty())
    {
        // release lock as long as the wait and reaquire it afterwards.
        c.wait(lock);
    }
    T val = q.front();
    q.pop();
    return val;
}

template <class T>
T utils::tsqueue<T>::try_dequeue(T def) {
    std::unique_lock<std::mutex> lock(m);
    while (q.empty() && !closed)
    {
        // release lock as long as the wait and reaquire it afterwards.
        c.wait(lock);
    }
    if(q.empty() && closed) {
        return def;
    }
    T val = q.front();
    q.pop();
    return val;
}

template <class T>
std::optional<T> utils::tsqueue<T>::try_dequeue_for(const std::chrono::milliseconds ms) {
    if(is_closed()) {
        return {};
    }
    std::unique_lock<std::mutex> lock(m);
    c.wait_for(lock, ms);
    if(q.empty()) {
        return {};
    }
    T val = q.front();
    q.pop();
    return val;
}

template <class T>
bool utils::tsqueue<T>::is_closed(void) {
    return closed && q.empty();
}

template <class T>
void utils::tsqueue<T>::close(void) {
    closed = true;
    c.notify_all();
}

template class utils::tsqueue<std::pair<std::string, Semantic>>;
template class utils::tsqueue<std::tuple<std::string, std::shared_ptr<uint8_t[]>, uint32_t, std::shared_ptr<uint8_t[]>, uint32_t>>;