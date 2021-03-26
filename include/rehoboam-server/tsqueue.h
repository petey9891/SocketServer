template<typename T>
class tsqueue {
public:
    tsqueue() = default;
    tsqueue(const tsqueue<T>&) = delete;
    virtual ~tsqueue() {
        this->clear();
    }

    void clear() {
        
    }
};