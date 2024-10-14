#pragma once

#include <condition_variable>
#include <exception>
#include <mutex>
#include <optional>
#include <queue>

namespace channel {

class ChannelClosed : public std::exception {
public:
    const char * what() const noexcept override {
        return "Channel closed";
    }
};

template <class T>
class iterator;

template <class T>
class Channel {
    std::condition_variable message_ready_flag;
    std::mutex queue_lock;
    std::mutex flag_lock;
    std::queue<T> message_queue;
    bool is_closed = false;
public:
    void send(const T message);
    T receive();
    std::optional<T> try_receive();
    void close();
    iterator<T> begin() {
        return iterator(this);
    }
    iterator<T> end() {
        return iterator<T>();
    }
};

template <class T>
class iterator {
    bool end_flag = false;
    Channel<T>* ch;
    T value;

    void get_next_value() {
        try {
            value = ch->receive();
        } catch (ChannelClosed&) {
            end_flag = true;
        }
    }
public:
    iterator(Channel<T>*ch): ch(ch) {
        get_next_value();
    }
    iterator(): ch(nullptr) {}
    bool operator!=(iterator& other) {
        return other.end_flag == end_flag;
    }
    T& operator*() {
        return value;
    }
    iterator& operator++() {
        get_next_value();
        return *this;
    }
};

}

template <class T>
void channel::Channel<T>::close() {
    const std::lock_guard<std::mutex> lock(flag_lock);
    is_closed = true;
    message_ready_flag.notify_all();
}

template <class T>
void channel::Channel<T>::send(const T message) {
    if (is_closed) {
        throw ChannelClosed();
    }
    const std::lock_guard<std::mutex> lock(queue_lock);
    message_queue.push(message);
    message_ready_flag.notify_all();
}

template <class T>
T channel::Channel<T>::receive() {
    while (true) {
        std::unique_lock<std::mutex> flag_unique_lock(flag_lock);
        message_ready_flag.wait(flag_unique_lock, [this]{return !message_queue.empty() || is_closed ;});
        const std::lock_guard<std::mutex> lock(queue_lock);
        if (!message_queue.empty()) {
            T message = message_queue.front();
            message_queue.pop();
            return message;
        }
        if (is_closed) {
            throw ChannelClosed();
        }
    }
}

template <class T>
std::optional<T> channel::Channel<T>::try_receive() {
    std::unique_lock<std::mutex> lock(queue_lock);
    bool lock_result = lock.try_lock();

    if (!lock_result) {
        return std::nullopt;
    }

    if (message_queue.empty()) {
        if (is_closed) {
            throw ChannelClosed();
        }
        return std::nullopt;
    }

    T message = message_queue.front();
    message_queue.pop();
    return message;
}

