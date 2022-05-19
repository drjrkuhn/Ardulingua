#pragma once

#ifndef __BUFFER_H__
    #define __BUFFER_H__

    #include <cstddef>

namespace rdl {

    /**
     * Simple array buffer.
     *
     * Data array memory can either be passed in the constructor, or the data
     * will be allocated from the heap.
     *
     * @tparam T    type of data
     */
    template <typename T>
    class Buffer {
     public:
        Buffer(size_t capacity) : capacity_(capacity), owns_data_(true) {
            data_ = (T*)malloc(sizeof(T) * capacity_);
        }
        Buffer(T* data, size_t capacity) : data_(data), capacity_(capacity), owns_data_(false) {
        }
        ~Buffer() {
            if (owns_data_)
                free(data_);
        }
        T* data() { return data_; }
        size_t capacity() const { return capacity_; }

     protected:
        T* data_;
        size_t capacity_;
        bool owns_data_;
    };

    /**
     * Static array buffer.
     *
     * Can be created in global memory and converted to a Buffer.
     *
     * ## Examples
     * @code{.cpp}
     * // does not work with GCC >= 8.0
     * Buffer<long> sample_1(StaticBuffer<long,100>::type{},100);
     *
     * long buf2[100];
     * Buffer<long> sample_2(buf2,100);
     *
     * StaticBuffer<long,100> sbuf3;
     * Buffer<long> sample_3 = sbuf3;
     *
     * Buffer<long> sample_4 = StaticBuffer<long,100>();
     * @endcode
     *
     * @tparam T            Type of data
     * @tparam CAPACITY     total capacity of buffer
     */
    template <typename T, size_t CAPACITY>
    class StaticBuffer {
     public:
        using type = T[CAPACITY];
        T* data() { return data_; }
        size_t capacity() const { return CAPACITY; }

        /** Treat this as a dynamic buffer */
        operator Buffer<T>() {
            return Buffer<T>(data_, CAPACITY);
        }

     protected:
        T data_[CAPACITY];
    };

}

#endif // __BUFFER_H__