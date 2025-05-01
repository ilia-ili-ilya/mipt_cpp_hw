#include <iostream>

template<typename T>

class Deque {
private:
    static const int sz_of_bucket = 32;
    T** buckets;
    size_t count_of_buckets;
    std::pair<size_t, size_t> start, finish;

    void reallocation() {
        T** new_buckets = new T* [3 * count_of_buckets];
        start.first += count_of_buckets;
        finish.first += count_of_buckets;
        for (size_t i = 0; i < count_of_buckets; ++i) {
            new_buckets[i] = reinterpret_cast<T*> (new char[sz_of_bucket * sizeof(T)]);
            new_buckets[count_of_buckets + i] = buckets[i];
            new_buckets[2 * count_of_buckets + i] = reinterpret_cast<T*> (new char[sz_of_bucket * sizeof(T)]);
        }
        delete[] buckets;
        buckets = new_buckets;
        count_of_buckets *= 3;
    }

    void copy_bucket(T* new_bucket, T* old_bucket, size_t bucket_size) {
        size_t i = 0;
        try {
            for (; i < bucket_size; ++i) {
                new(new_bucket + i) T(*(old_bucket + i));
            }
        } catch (...) {
            for (size_t j = 0; j < i; ++j) {
                (new_bucket + j)->~T();
            }
            delete[] reinterpret_cast<char *>(new_bucket);
            throw;
        }
    }

    void clear_bucket(T* bucket, size_t bucket_size) {
        for (size_t i = 0; i < bucket_size; ++i) {
            (bucket + i)->~T();
        }
    }

    int get_correct_size(int count) {
        return (count / sz_of_bucket + 1) * 3;
    }


    template<bool is_const>
    struct base_iterator {
    protected:
        T** to_buck;
        T* _buck;
        int to_el;

    public:
        using value_type = typename std::conditional<is_const, const T, T>::type;
        using pointer = typename std::conditional<is_const, const T*, T* >::type;
        using reference = typename std::conditional<is_const, const T&, T&>::type;
        using const_reference = typename std::conditional<is_const, const T&, T&>::type;
        using iterator_category = std::random_access_iterator_tag;

        operator base_iterator<true>() const {
            return base_iterator<true>(to_buck, to_el);
        }

        base_iterator(T** to_buck, int to_el) : to_buck(to_buck), _buck(*to_buck), to_el(to_el) {}

        base_iterator &operator++() {
            if (to_el < static_cast<int> (Deque::sz_of_bucket) - 1) {
                to_el++;
            } else {
                to_buck++;
                _buck = *to_buck;
                to_el = 0;
            }
            return *this;
        }

        base_iterator &operator--() {
            if (to_el != 0) {
                to_el--;
            } else {
                to_buck--;
                to_el = Deque::sz_of_bucket - 1;
                _buck = *to_buck;
            }
            return *this;
        }

        base_iterator operator++(int) {
            base_iterator copy = *this;
            ++*this;
            return copy;
        }

        base_iterator operator--(int) {
            base_iterator copy = *this;
            --*this;
            return copy;
        }

        base_iterator &operator+=(int x) {
            if (x > 0) {
                to_buck += (to_el + x) / Deque::sz_of_bucket;
                _buck = *to_buck;
                to_el = (to_el + x) % Deque::sz_of_bucket;
            } else {
                to_buck -= (-x - to_el + Deque::sz_of_bucket - 1) / Deque::sz_of_bucket;
                _buck = *to_buck;
                to_el = (to_el + Deque::sz_of_bucket - (-x) % Deque::sz_of_bucket) % Deque::sz_of_bucket;
            }
            return *this;
        }

        base_iterator &operator-=(int x) {
            *this += -x;
            return *this;
        }

        auto operator<=>(const base_iterator &other) const {
            if (auto cmp = to_buck <=> other.to_buck; cmp != 0) {
                return cmp;
            } else {
                return to_el <=> other.to_el;
            }
        }

        bool operator==(const base_iterator &other) const = default;

        bool operator!=(const base_iterator &other) const = default;

        base_iterator operator+(int x) const {
            base_iterator copy = *this;
            copy += x;
            return copy;
        }

        base_iterator operator-(int x) const {
            base_iterator copy = *this;
            copy -= x;
            return copy;
        }

        int operator-(const base_iterator& other) const {
            return (to_buck - other.to_buck) * Deque::sz_of_bucket + to_el - other.to_el;
        }

        reference operator*() const {
            return *((_buck) + to_el);
        }

        pointer operator->() const {
            return (_buck) + to_el;
        }
    };

public:
    using iterator = base_iterator<false>;
    using const_iterator = base_iterator<true>;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    Deque() : buckets(new T *[3]), count_of_buckets(3), start({1, 0}), finish({1, 0}) {
        for (int i = 0; i < 3; ++i) {
            buckets[i] = reinterpret_cast<T *> (new char[sz_of_bucket * sizeof(T)]);
        }
    }

    Deque(const Deque& other) : buckets(new T*[other.count_of_buckets]), count_of_buckets(other.count_of_buckets),
                                start(other.start), finish(other.finish) {
        for (size_t j = 0; j < count_of_buckets; ++j) {
            buckets[j] = reinterpret_cast<T*> (new char[sz_of_bucket * sizeof(T)]);
        }
        size_t i = start.first;
        try {
            if (start.first == finish.first) {
                copy_bucket(buckets[i] + start.second, other.buckets[i], finish.second - start.second);
            } else {
                for (; i <= finish.first; ++i) {
                    if (i == start.first) {
                        copy_bucket(buckets[i] + start.second, other.buckets[i], sz_of_bucket - start.second);
                    } else if (i == finish.first) {
                        copy_bucket(buckets[i], other.buckets[i], finish.second);
                    } else {
                        copy_bucket(buckets[i], other.buckets[i], sz_of_bucket);
                    }
                }
            }
        } catch (...) {
            for (size_t j = start.first; j < i; ++j) {
                if (j == start.first) {
                    clear_bucket(buckets[j] + start.second, sz_of_bucket - start.second);
                } else {
                    clear_bucket(buckets[j], sz_of_bucket);
                }
            }
            throw;
        }
    }

    explicit Deque(int count) : buckets(new T*[get_correct_size(count)]),
                                count_of_buckets(get_correct_size(count)), start({count / sz_of_bucket + 1, 0}),
                                finish({(count / sz_of_bucket) * 2 + 1, count % sz_of_bucket}) {
        for (size_t j = 0; j < count_of_buckets; ++j) {
            buckets[j] = reinterpret_cast<T*> (new char[sz_of_bucket * sizeof(T)]);
        }
        size_t i = start.first;
        try {
            for (; i <= finish.first; ++i) {
                for (size_t j = 0; j < ((i == finish.first) ? finish.second : sz_of_bucket); ++j) {
                    try {
                        new(buckets[i] + j) T();
                    } catch (...) {
                        clear_bucket(buckets[i], j);
                        throw;
                    }
                }

            }
        } catch (...) {
            for (size_t j = start.first; j < i; ++j) {
                clear_bucket(buckets[j], sz_of_bucket);
            }
            finish = start;
        }
    }

    Deque(int count, const T &value) : buckets(new T*[get_correct_size(count)]),
                                       count_of_buckets(get_correct_size(count)),
                                       start({count / sz_of_bucket + 1, 0}),
                                       finish({(count / sz_of_bucket) * 2 + 1, count % sz_of_bucket}) {
        for (size_t j = 0; j < count_of_buckets; ++j) {
            buckets[j] = reinterpret_cast<T*> (new char[sz_of_bucket * sizeof(T)]);
        }
        size_t i = start.first;
        try {
            for (; i <= finish.first; ++i) {
                for (size_t j = 0; j < ((i == finish.first) ? finish.second : sz_of_bucket); ++j) {
                    try {
                        new(buckets[i] + j) T(value);
                    } catch (...) {
                        clear_bucket(buckets[i], j);
                    }
                }

            }
        } catch (...) {
            for (int j = start.first; j < i; ++j) {
                clear_bucket(buckets[j], sz_of_bucket);
            }
        }
    }

    ~Deque() {
        for (size_t i = start.first; i <= finish.first; ++i) {
            for (size_t j = ((i == start.first) ? start.second : 0);
                 j < ((i == finish.first) ? finish.second : sz_of_bucket); ++j) {
                (buckets[i] + j)->~T();
            }
        }
        for (size_t i = 0; i < count_of_buckets; ++i) {
            delete[] reinterpret_cast<char*>(buckets[i]);
        }
        delete[] buckets;
    }

    iterator begin() {
        return iterator(buckets + start.first, start.second);
    }

    iterator end() {
        return iterator(buckets + finish.first, finish.second);
    }

    const_iterator cbegin() const {
        return const_iterator(buckets + start.first, start.second);
    }

    const_iterator cend() const {
        return const_iterator(buckets + finish.first, finish.second);
    }

    reverse_iterator rbegin() {
        return reverse_iterator(end());
    }

    reverse_iterator rend() {
        return reverse_iterator(begin());
    }

    const_iterator begin() const {
        return cbegin();
    }

    const_iterator end() const {
        return cend();
    }

    const_reverse_iterator rbegin() const {
        return const_reverse_iterator(end());
    }

    const_reverse_iterator rend() const {
        return const_reverse_iterator(begin());
    }

    const_reverse_iterator crbegin() const {
        return const_reverse_iterator(cend());
    }

    const_reverse_iterator crend() const {
        return const_reverse_iterator(cbegin());
    }

    size_t size() const {
        return (finish.first - start.first) * sz_of_bucket + finish.second - start.second;
    }

    T& operator[](const size_t index) {
        return buckets[start.first + (start.second + index) / sz_of_bucket][(start.second + index) % sz_of_bucket];
    }

    const T& operator[](const size_t index) const {
        return buckets[start.first + (start.second + index) / sz_of_bucket][(start.second + index) % sz_of_bucket];
    }

    T& at(const size_t index) {
        if (index >= size()) {
            throw std::out_of_range("Out of range");
        }
        return Deque<T>::operator[](index);
    }

    const T& at(const size_t index) const {
        if (index >= size()) {
            throw std::out_of_range("Out of range");
        }
        return Deque<T>::operator[](index);
    }

    Deque& operator=(const Deque &other) {
        if (this == &other) {
            return *this;
        }
        Deque copy(other);
        swap(copy);
        return *this;

    }

    void swap(Deque<T>& other) {
        std::swap(buckets, other.buckets);
        std::swap(count_of_buckets, other.count_of_buckets);
        std::swap(start, other.start);
        std::swap(finish, other.finish);
    }

    void push_back(const T& value) {
        if ((finish.first + 1 == count_of_buckets) && (finish.second + 1 == static_cast<size_t> (sz_of_bucket))) {
            reallocation();
        }
        new(buckets[finish.first] + finish.second) T(value);
        if (finish.second + 1 == static_cast<size_t> (sz_of_bucket)) {
            finish.second = 0;
            finish.first++;
        } else {
            finish.second++;
        }
    }

    void pop_back() {
        if (finish.second == 0) {
            finish.second = sz_of_bucket - 1;
            finish.first--;
        } else {
            finish.second--;
        }
    }

    void push_front(const T& value) {
        if ((start.first == 0) && (start.second == 0)) {
            reallocation();
        }
        if (start.second == 0) {
            start.second = sz_of_bucket - 1;
            start.first--;
        } else {
            start.second--;
        }
        new(buckets[start.first] + start.second) T(value);
    }

    void pop_front() {
        if (start.second + 1 == static_cast<size_t> (sz_of_bucket)) {
            start.second = 0;
            start.first++;
        } else {
            start.second++;
        }
    }

    void insert(iterator it, const T& x) {
        auto xxx = it.operator->();
        push_back(x);
        for (iterator jt = end() - 1; jt.operator->() != xxx; --jt) {
            std::swap(*jt, *(jt - 1));
        }
    }

    void erase(iterator it) {
        for (iterator jt = it + 1; jt != end(); ++jt) {
            std::swap(*jt, *(jt - 1));
        }
        pop_back();
    }
};


