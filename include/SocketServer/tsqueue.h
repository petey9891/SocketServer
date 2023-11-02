#pragma once

#include <deque>
#include <mutex>
#include <condition_variable>

/**
 * @class tsqueue
 * @brief A thread-safe implementation of a double-ended queue (deque).
 *
 * The class utilizes mutexes and a condition variable to ensure that all operations are thread-safe and to provide functionality for waiting until the queue is not empty.
 *
 * @tparam T The type of elements in the queue.
 */
template<typename T>
class tsqueue {
public:
    /**
     * @brief Default constructor. Initializes an empty queue.
     */
    tsqueue() = default;

    /**
     * @brief Deleted copy constructor.
     *
     * Copy construction is not allowed for this class to avoid issues with multiple threads accessing the same queue.
     */
    tsqueue(const tsqueue<T>&) = delete;

    /**
     * @brief Destructor. Clears the queue.
     */
    virtual ~tsqueue() {
        this->clear();
    }

    // Queue Operations

    /**
     * @brief Gets the item at the front of the queue.
     *
     * @return A const reference to the item at the front of the queue.
     */
    const T& front() {
        std::scoped_lock lock(this->m_mutexQueue);
        return this->m_deqQueue.front();
    }

    /**
     * @brief Gets the item at the back of the queue.
     *
     * @return A const reference to the item at the back of the queue.
     */
    const T& back() {
        std::scoped_lock lock(this->m_mutexQueue);
        return this->m_deqQueue.back();
    }

    /**
     * @brief Removes and returns the item at the front of the queue.
     *
     * @return The item that was at the front of the queue.
     */
    T pop_front() {
        std::scoped_lock lock(this->m_mutexQueue);
        auto t = std::move(this->m_deqQueue.front());
        this->m_deqQueue.pop_front();
        return t;
    }

    /**
     * @brief Removes and returns the item at the back of the queue.
     *
     * @return The item that was at the back of the queue.
     */
    T pop_back() {
        std::scoped_lock lock(this->m_mutexQueue);
        auto t = std::move(this->m_deqQueue.back());
        this->m_deqQueue.pop_back();
        return t;
    }

    /**
     * @brief Adds an item to the front of the queue.
     *
     * @param item The item to add to the front of the queue.
     */
    void push_front(const T& item) {
        std::scoped_lock lock(this->m_mutexQueue);
        this->m_deqQueue.emplace_front(std::move(item));

        std::unique_lock<std::mutex> ul(this->m_muxBlocking);
        this->m_cvBlocking.notify_one();
    }

    /**
     * @brief Adds an item to the back of the queue.
     *
     * @param item The item to add to the back of the queue.
     */
    void push_back(const T& item) {
        std::scoped_lock lock(this->m_mutexQueue);
        this->m_deqQueue.emplace_back(std::move(item));

        std::unique_lock<std::mutex> ul(this->m_muxBlocking);
        this->m_cvBlocking.notify_one();
    }

    /**
     * @brief Checks if the queue is empty.
     *
     * @return True if the queue is empty, false otherwise.
     */
    bool empty() {
        std::scoped_lock lock(this->m_mutexQueue);
        return this->m_deqQueue.empty();
    }

    /**
     * @brief Gets the number of items in the queue.
     *
     * @return The number of items in the queue.
     */
    size_t count() {
        std::scoped_lock lock(this->m_mutexQueue);
        return this->m_deqQueue.size();
    }

    /**
     * @brief Clears all items from the queue.
     */
    void clear() {
        std::scoped_lock lock(this->m_mutexQueue);
        this->m_deqQueue.clear();
    }

    // Additional Functionality

    /**
     * @brief Waits until the queue is not empty.
     *
     * This function will block the current thread until the queue has at least one item.
     */
    void wait() {
        while (this->empty()) {
            std::unique_lock<std::mutex> ul(m_muxBlocking);
            m_cvBlocking.wait(ul);
        }
    }

protected:
    // Mutex to protect access to the deque.
    std::mutex m_mutexQueue;

    // The underlying deque data structure.
    std::deque<T> m_deqQueue;

    // Condition variable used for waiting.
    std::condition_variable m_cvBlocking;

    // Mutex used with the condition variable.
    std::mutex m_muxBlocking;
};
