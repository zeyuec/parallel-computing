#ifndef DATA_STACK_H

#define DATA_STACK_H

template <typename T>
class DataStack {
public:
    DataStack() {
        
    }
    void init(T *data, int size) {
        data_ = data;
        size_ = size;
        h_ = 0;
    }
    
    T pop() {
        T ret = data_[--h_];
        return ret;
    }

    void push(T item) {
        data_[h_++] = item;
    }

    bool isEmpty() {
        return h_ == 0;
    }

    T *getData() {
        return data_; 
    }

    int getHeight() {
        return h_;
    }

    T *data_;
    int h_, size_;
private:
    
};

#endif
