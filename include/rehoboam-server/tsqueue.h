#include <deque>
#include <mutex>
#include <condition_variable>

/**
 * Thread safe double ended queue
 */
template<typename T>
class tsqueue {
protected:
    std::mutex mutexQueue;
    std::deque<T> deqQueue;
    
    std::condition_variable cvBlocking;
    std::mutex muxBlocking;

public:
    tsqueue() = default;
    tsqueue(const tsqueue<T>&) = delete;
    virtual ~tsqueue() {
        this->clear();
    }

public:


    // Returns item at front of queue
    const T& front() {
        std::scoped_lock lock(this->mutexQueue);
        return this->deqQueue.front();
    }

    // Returns item at end of queue
    const T& back() {
        std::scoped_lock lock(this->mutexQueue);
        return this->deqQueue.back();
    }

    // Removes and returns first item of queue
    T pop_front() {
        std::scoped_lock lock(this->mutexQueue);
        auto t = std::move(this->deqQueue.front());
        this->deqQueue.pop_front();
        return t;
    }

    // Removes and returns last item of queue
    T pop_back() {
        std::scoped_lock lock(this->mutexQueue);
        auto t = std::move(this->deqQueue.back());
        this->deqQueue.pop_back();
        return t;
    }

    // Adds an item to the front of the queue
    void push_front(const T& item) {
        std::scoped_lock lock(this->mutexQueue);
        this->deqQueue.emplace_front(std::move(item));

        std::unique_lock<std::mutex> ul(this->muxBlocking);
        this->cvBlocking.notify_one();
    }

    // Adds an item to the back of the queue
    void push_back(const T& item) {
        std::scoped_lock lock(this->mutexQueue);
        this->deqQueue.emplace_back(std::move(item));

        std::unique_lock<std::mutex> ul(this->muxBlocking);
        this->cvBlocking.notify_one();
    }
    
    // Returns true if the queue has no items
    bool empty() {
        std::scoped_lock lock(this->mutexQueue);
        return this->deqQueue.empty();
    }

    // Returns the number of items in the queue
    size_t count() {
        std::scoped_lock lock(this->mutexQueue);
        return this->deqQueue.size();
    }

    // Clears the queue
    void clear() {
        std::scoped_lock lock(this->mutexQueue);
        this->deqQueue.clear();
    }

    // While the queue is empty it waits until an item gets added.
    // Calling this prevents the program from consuming 100% of a CPU core 24/7
    void wait() {
        while (this->empty()) {
            std::unique_lock<std::mutex> ul(muxBlocking);
            cvBlocking.wait(ul);
        }
    }
};