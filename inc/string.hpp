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
        if (!data || length == 0)
            return;
        size_t newSize = size_ + length;
        char* newData = new char[newSize];
        if (data_)
            memcpy(newData, data_, size_);

        memcpy(newData + size_, data, length);
        delete[] data_;
        data_ = newData;
        size_ = newSize;
    }

    void append(const char* data) {
        if (!data || strlen(data) == 0)
            return;
        size_t newSize = size_ + strlen(data);
        char* newData = new char[newSize];
        if (data_)
            memcpy(newData, data_, size_);

        memcpy(newData + size_, data, strlen(data));
        delete[] data_;
        data_ = newData;
        size_ = newSize;
    }

    void append(const Bstring& other) {
        append(other.data(), other.size());
    }

    Bstring substr(size_t f, size_t l) const {
        if (f >= size_ || f >= l)
            return Bstring();
        
        if (l > size_)
            l = size_;
        
        size_t len = l - f;
        char* newStr = new char[len + 1];
        memcpy(newStr, data_ + f, len);
        newStr[len] = '\0';
        
        Bstring result(newStr, len);
        delete[] newStr;
        return result;
    }

    void erase(size_t pos, size_t len) {
        if (pos >= size_ || len == 0)
            return;

        size_t remaining = size_ - pos;
        if (len > remaining) {
            len = remaining;
        }

        if (pos + len < size_)
            memmove(data_ + pos, data_ + pos + len, size_ - (pos + len));

        size_ -= len;

        if (size_ == 0) {
            delete[] data_;
            data_ = nullptr;
        }
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

    Bstring& operator+=(const char* data) {
        append(data);
        return *this;
    }

    Bstring& operator+=(const Bstring& other) {
        append(other);
        return *this;
    }

    Bstring& operator+=(const string& str) {
        if (!str.empty())
            append(Bstring(str));
        return *this;
    }

    bool operator<(const Bstring& other) const {
        size_t minSize = size_ < other.size_ ? size_ : other.size_;
        int cmp = memcmp(data_, other.data_, minSize);
        if (cmp != 0)
            return cmp < 0;
        return size_ < other.size_;
    }

    bool operator==(const Bstring& other) const {
        if (size_ != other.size_)
            return false;
        return memcmp(data_, other.data_, size_) == 0;
    }
};