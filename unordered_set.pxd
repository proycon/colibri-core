from libcpp.utility cimport pair

cdef extern from "<unordered_set>" namespace "std":
    cdef cppclass unordered_set[T]:
        cppclass iterator:
            T& operator*() nogil
            iterator operator++() nogil
            iterator operator--() nogil
            bint operator==(iterator) nogil
            bint operator!=(iterator) nogil
        unordered_set()
        unordered_set(unordered_set&)
        iterator begin() nogil
        void clear() nogil
        size_t count(T&) nogil
        bint empty() nogil
        iterator end() nogil
        void erase(iterator) nogil
        void erase(iterator, iterator) nogil
        size_t erase(T&) nogil
        iterator find(T&) nogil
        iterator insert(iterator, T) nogil
        void insert(input_iterator, input_iterator)
        size_t max_size() nogil
        void rehash(size_t)
        size_t size() nogil
