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
        array() : data_(nullptr), max_size_(0), ref_counts(0) {
            std::cout << "\tarray() constructor\n";
        }
        array(SizeT max_size) : max_size_(max_size), owns_data_(true) {
            std::cout << "\tarray(" << max_size << ") constructor\n";
            data_ = (T*)malloc(sizeof(T) * max_size_);
        }
        array(T* data, SizeT max_size) : data_(data), max_size_(max_size), owns_data_(false) {
            std::cout << "\tarray(" << data << "," << max_size << ") constructor\n";
        }
        array(array<T>& other) {

        }
        ~array() {
            std::cout << "\t~array() destructor: owns_data_:" << owns_data_ << ", data_:" << data_ << "\n";
            if (owns_data_ && data_ != nullptr) free(data_);
        }

        bool valid() { return data_ != nullptr; }

        /** Array access convenience operators (unsafe). */
        T& operator[](size_t idx) { return data_[idx]; }
        const T& operator[](size_t idx) const { return data_[idx]; }

        T* data() { return data_; }
        SizeT max_size() const { return max_size_; }

     protected:
        T* data_;
        SizeT max_size_;
        SizeT ref_counts_;
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
        static_array() : array<T, SizeT>(staticdata_, CAPACITY) {
            std::cout << "\tstatic_array<" << CAPACITY << ">() constructor\n";
        }
        /** Treat this as a dynamic array */
        operator array<T>() {
            return array<T>(staticdata_, CAPACITY);
        }

     protected:
        T staticdata_[CAPACITY];
    };
}

#endif // __BUFFER_H__