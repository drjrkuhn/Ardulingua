#pragma once

#ifndef __BUFFER_H__
    #define __BUFFER_H__

    #include <cstdlib> // for size_t, malloc, free
    #include <iostream>

namespace rdl {

    /**
     * Simple array holder.
     *
     * Data array memory can either be passed in the constructor, or the data
     * will be allocated from the heap.
     *
     * @tparam T    type of data
     */
    template <typename T, typename SizeT = size_t>
    class array {
     public:
        array() : data_(nullptr), max_size_(0), can_free_(false) {
            std::cout << "\tarray() constructor "<<this<<"\n";
        }
    #if 0
        array(array& other) = delete;
    #else
        // since this is a copy from an lvalue, we turn off can_free_ and let other free the memory
        array(array& other)
            : data_(other.data_), max_size_(other.max_size_), can_free_(false) {
            std::cout << "\tarray(" << other.data_ << ") copy constructor "<<this<<"\n";
        }
    #endif
    #if 0
        array& operator=(const array& other) = delete;
    #else
        array& operator=(const array& other) {
            std::cout << "\tarray = " << &other << " copy assignment "<<this<<"\n";
            data_     = other.data_;
            max_size_ = other.max_size_;
            can_free_ = other.can_free_;
            return *this;
        }
    #endif

        array(array&& other) : data_(other.data_), max_size_(other.max_size_), can_free_(other.can_free_) {
            std::cout << "\tarray(" << &other << ") move constructor "<<this<<"\n";
            other.data_     = nullptr;
            other.max_size_ = 0;
            other.can_free_ = false;
        };

        ~array() {
            std::cout << "\t~array() destructor: can_free_:" << can_free_ << ", data_:" << data_ << " this:"<<this<<"\n";
            if (can_free_ && data_ != nullptr) free(data_);
        }

        bool valid() { return data_ != nullptr; }

        /** Array access convenience operators (unsafe). */
        T& operator[](size_t idx) { return data_[idx]; }
        const T& operator[](size_t idx) const { return data_[idx]; }

        T* data() { return data_; }
        SizeT max_size() const { return max_size_; }

     protected:
        array(T* data, SizeT max_size, bool can_free) : data_(data), max_size_(max_size), can_free_(can_free) {
            std::cout << "\tarray(" << data << "," << max_size << "," << can_free << ") constructor "<<this<<"\n";
        }
        T* data_;
        SizeT max_size_;
        bool can_free_;
    };

    /**
     * Static array holder.
     *
     * Can be created in global memory and converted to a array.
     *
     * ## Examples
     * @code{.cpp}
     * // does not work with GCC >= 8.0
     * array<long> sample_1(static_array<long,100>::type{},100);
     *
     * long buf2[100];
     * array<long> sample_2(buf2,100);
     *
     * static_array<long,100> sbuf3;
     * array<long> sample_3 = sbuf3;
     *
     * // vvv Primary use case vvv
     * array<long> sample_4 = static_array<long,100>();
     * @endcode
     *
     * @tparam T            Type of data
     * @tparam CAPACITY     total max_size of array
     * @tparam SizeT        type used to hold max_size and size
     */
    template <typename T, size_t CAPACITY, typename SizeT = size_t>
    class static_array : public array<T, SizeT> {
     public:
        using BaseT = array<T, SizeT>;
        static_array() : BaseT(staticdata_, CAPACITY, false) {
            std::cout << "\tstatic_array<" << CAPACITY << ">() constructor "<<this<<"\n";
        }
        /** Treat this as a dynamic array */
        operator BaseT&() {
            std::cout << "\tstatic_array<" << CAPACITY << ">::operator array<>() cast "<<this<<"\n";
            return BaseT(staticdata_, CAPACITY, false);
        }

     protected:
        T staticdata_[CAPACITY];
    };

    template <typename T, typename SizeT = size_t>
    class dynamic_array : public array<T, SizeT> {
     public:
        using BaseT = array<T, SizeT>;
        dynamic_array(SizeT max_size) : BaseT(nullptr, max_size, true) {
            std::cout << "\tdynamic_array<" << max_size << ">() constructor "<<this<<"\n";
            BaseT::data_ = (T*)malloc(max_size * sizeof(T));
        }
        /** Treat this as a dynamic array */
        operator BaseT&() {
            std::cout << "\tdynamic_array::operator array<>() cast "<<this<<"\n";
            return dynamic_cast<BaseT&>(*this);
        }

     protected:
    };
}

#endif // __BUFFER_H__