#pragma once

#ifndef __BUFFER_H__
    #define __BUFFER_H__

    #include <cstdlib> // for size_t, malloc, free
    #include <iostream>

namespace rdl {

    /**
     * Simple arraybuf holder.
     *
     * Data arraybuf memory can either be passed in the constructor, or the data
     * will be allocated from the heap. Create or assign with derived
     * static_arraybuf or dynamic_arraybuf class.
     *
     * ## Examples
     * @code{.cpp}
     * // Primary use cases. Uses move constructor.
     * // Final arraybuf responsible for freeing memory.
     *
     * arraybuf<long> sample_1s = static_arraybuf<long, 100>();
     *
     * arraybuf<long> sample_1d = dynamic_arraybuf<long>(100);
     *
     * // Alternative use case. Uses copy constructor.
     * // Original xxx_arraybuf responsible for freeing memory.
     *
     * static_arraybuf<long, 100> b2s;
     * arraybuf<long> sample_2s = b2s;
     *
     * dynamic_arraybuf<long> b2d(100);
     * arraybuf<long> sample_2d = b2d;
     *
     * // Alternative use case. Uses move assignment.
     * // Final arraybuf responsible for freeing memory.
     *
     * arraybuf<long> sample_3s;
     * static_arraybuf<long, 100> b3s;
     * sample_3s = std::move(b3s);
     *
     * arraybuf<long> sample_3d;
     * dynamic_arraybuf<long> b3d(100);
     * sample_3d = std::move(b3d);
     * @endcode
     *
     * @tparam T    type of data
     */
    template <typename T, typename SizeT = size_t>
    class arraybuf {
     public:
        /** Default constructor for empty arraybuf. Use valid() to test. */
        arraybuf() : data_(nullptr), max_size_(0), can_free_(false) {}
        /** lvalue copy constructor. Turn off can_free_ and let other lvalue free the memory. */
        arraybuf(arraybuf& lval_other) : arraybuf(lval_other.data_, lval_other.max_size_, false) {}
        /** rvalue move constructor. */
        arraybuf(arraybuf&& rval_other) : arraybuf(rval_other.data_, rval_other.max_size_, rval_other.can_free_) {
            rval_other.data_     = nullptr;
            rval_other.max_size_ = 0;
            rval_other.can_free_ = false;
        };

        /** Create an arraybuf from fixed memory. Caller is responsible for freeing. */
        arraybuf(T*& data, SizeT fixed_size) : arraybuf(data, fixed_size, false) {}

        // NO COPY ASSIGNMENT - must use move assignment
        arraybuf& operator=(const arraybuf& lval_other) = delete;
        /** rvalue move assignment. */
        arraybuf& operator=(arraybuf&& rval_other) {
            data_                = rval_other.data_;
            max_size_            = rval_other.max_size_;
            can_free_            = rval_other.can_free_;
            rval_other.data_     = nullptr;
            rval_other.max_size_ = 0;
            rval_other.can_free_ = false;
            return *this;
        }
        /** See copy and move constructors and assignments for rules on who frees buffer */
        ~arraybuf() {
            if (can_free_ && data_ != nullptr) free(data_);
        }
        /** Array access convenience operators (unsafe). */
        T& operator[](size_t idx) { return data_[idx]; }
        /** Array access convenience operators (unsafe). */
        const T& operator[](size_t idx) const { return data_[idx]; }
        /** Array access convenience method (unsafe). */
        T* data() { return data_; }
        /** Total capacity of this arraybuf. */
        SizeT max_size() const { return max_size_; }
        /** Is the data pointer valid? */
        bool valid() { return data_ != nullptr; }

     protected:
        arraybuf(T* data, SizeT max_size, bool can_free) : data_(data), max_size_(max_size), can_free_(can_free) {}
        T* data_;
        SizeT max_size_;
        bool can_free_;
    };

    /**
     * Static arraybuf holder.
     * Can be created in global or stack memory and assigned to an arraybuf via slicing.
     *
     * @tparam T            Type of data
     * @tparam CAPACITY     total max_size of arraybuf
     * @tparam SizeT        type used to hold max_size
     */
    template <typename T, size_t CAPACITY, typename SizeT = size_t>
    class static_arraybuf : public arraybuf<T, SizeT> {
     public:
        using BaseT = arraybuf<T, SizeT>;
        /** buffer is allocated as a big class member. */
        static_arraybuf() : BaseT(staticdata_, CAPACITY, false) {}

     protected:
        T staticdata_[CAPACITY];
    };

    /**
     * Dynamic arraybuf holder
     * Created on the heap and designed to be assigned to an arraybuf via slicing.
     *
     * @tparam T            Type of data
     * @tparam SizeT        type used to hold max_size
     */
    template <typename T, typename SizeT = size_t>
    class dynamic_arraybuf : public arraybuf<T, SizeT> {
     public:
        using BaseT = arraybuf<T, SizeT>;
        /** Buffer is dynamicall allocated. Base method responsible for freeing. */
        dynamic_arraybuf(SizeT max_size) : BaseT(nullptr, max_size, true) {
            BaseT::data_ = (T*)malloc(max_size * sizeof(T));
        }
    };
}

#endif // __BUFFER_H__