#pragma once

#include "header.hpp"

class Bstring {
private:
    char* data_;
    size_t size_;

public:
    Bstring() : data_(nullptr), size_(0) {}

    Bstring(const char* data) : data_(nullptr), size_(0) {
        if (data) {
            size_t length = strlen(data);
            if (length > 0) {
                data_ = new char[length];
                memcpy(data_, data, length);
                size_ = length;
            }
        }
    }

    Bstring(const char* data, size_t length) : data_(nullptr), size_(0) {
        if (data && length > 0) {
            data_ = new char[length];
            memcpy(data_, data, length);
            size_ = length;
        }
    }

    Bstring(const Bstring& other) : data_(nullptr), size_(0) {
        if (other.size_ > 0) {
            data_ = new char[other.size_];
            memcpy(data_, other.data_, other.size_);
            size_ = other.size_;
        }
    }

    Bstring(const string& str) : data_(nullptr), size_(0) {
        if (!str.empty()) {
            data_ = new char[str.length()];
            memcpy(data_, str.data(), str.length());
            size_ = str.length();
        }
    }

    ~Bstring() {
        delete[] data_;
    }

    Bstring& operator=(const Bstring& other) {
        if (this != &other) {
            delete[] data_;
            data_ = nullptr;
            size_ = 0;
            if (other.size_ > 0) {
                data_ = new char[other.size_];
                memcpy(data_, other.data_, other.size_);
                size_ = other.size_;
            }
        }
        return *this;
    }

    void append(const char* data, size_t length) {
        if (!data || length == 0) return;
        size_t newSize = size_ + length;
        char* newData = new char[newSize];
        if (data_) {
            memcpy(newData, data_, size_);
        }
        memcpy(newData + size_, data, length);
        delete[] data_;
        data_ = newData;
        size_ = newSize;
    }

    void append(const Bstring& other) {
        append(other.data(), other.size());
    }

    const char* data() const {
        return data_;
    }

    size_t size() const {
        return size_;
    }

    bool empty() const {
        return size_ == 0;
    }

    void clear() {
        delete[] data_;
        data_ = nullptr;
        size_ = 0;
    }

    char& operator[](size_t index) {
        if (index >= size_)
            throw out_of_range("Index out of range");
        return data_[index];
    }

    const char& operator[](size_t index) const {
        if (index >= size_)
            throw out_of_range("Index out of range");
        return data_[index];
    }

    bool operator<(const Bstring& other) const {
        size_t minSize = size_ < other.size_ ? size_ : other.size_;
        int cmp = memcmp(data_, other.data_, minSize);
        if (cmp != 0) return cmp < 0;
        return size_ < other.size_;
    }

    bool operator==(const Bstring& other) const {
        if (size_ != other.size_) return false;
        return memcmp(data_, other.data_, size_) == 0;
    }
};