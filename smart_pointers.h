#include <cstddef>
#include <iostream>
#include <memory>


template<typename T, typename Alloc = std::allocator<T>>
class List {
private:
    struct BaseNode {
        BaseNode *next;
        BaseNode *prev;

        BaseNode() {
            next = this;
            prev = this;
        }

        BaseNode(const BaseNode *next, const BaseNode *prev) : next(next), prev(prev) {}
    };

    struct Node : BaseNode {
        T value;
    };
    using NodeAlloc = typename std::allocator_traits<Alloc>::template rebind_alloc<Node>;
    using NodeTraits = std::allocator_traits<NodeAlloc>;

    BaseNode fakeNode;
    size_t sz;
    [[no_unique_address]] NodeAlloc alloc;


    template<bool is_const>

    class BaseIterator {
    private:
        BaseNode *node;

    public:
        BaseNode *get_node() const noexcept { return node; }

        using iterator_category = std::bidirectional_iterator_tag;
        using value_type = std::conditional_t<is_const, const T, T>;
        using difference_type = long long;
        using pointer = value_type *;
        using reference = value_type &;

        BaseIterator() : node(nullptr) {}

        BaseIterator(const BaseIterator<is_const> &iter) noexcept: node(iter.node) {}

        operator BaseIterator<true>() const noexcept {
            BaseIterator<true> const_iter(node);
            return const_iter;
        }

        explicit BaseIterator(BaseNode *new_node) noexcept: node(new_node) {}

        explicit BaseIterator(const BaseNode *new_node) noexcept: node(const_cast<BaseNode *>(new_node)) {}

        BaseIterator<is_const> &operator++() noexcept {
            node = node->next;
            return *this;
        }

        BaseIterator<is_const> &operator--() noexcept {
            node = node->prev;
            return *this;
        }

        BaseIterator<is_const> operator++(int) noexcept {
            auto tmp(*this);
            ++(*this);
            return tmp;
        }

        BaseIterator<is_const> operator--(int) noexcept {
            auto tmp(*this);
            --(*this);
            return tmp;
        }

        BaseIterator<is_const> &operator=(const BaseIterator<is_const> &iter) noexcept {
            node = iter.node;
            return *this;
        }

        value_type &operator*() const noexcept {
            return reinterpret_cast<Node *>(node)->value;
        }

        value_type *operator->() const noexcept {
            return &(reinterpret_cast<Node *>(node)->value);
        }

        bool operator==(const BaseIterator<is_const> &iter) const noexcept {
            return node == iter.node;
        }

        bool operator!=(const BaseIterator<is_const> &iter) const noexcept {
            return node != iter.node;
        }

        ~BaseIterator() = default;
    };

public:
    using iterator = BaseIterator<false>;
    using const_iterator = BaseIterator<true>;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    iterator begin() noexcept { return iterator((&fakeNode)->next); }

    iterator end() noexcept { return iterator(&fakeNode); }

    const_iterator begin() const noexcept { return cbegin(); }

    const_iterator end() const noexcept { return cend(); }

    const_iterator cbegin() const noexcept { return const_iterator((&fakeNode)->next); }

    const_iterator cend() const noexcept { return const_iterator(&fakeNode); }

    reverse_iterator rbegin() { return std::make_reverse_iterator(end()); }

    reverse_iterator rend() { return std::make_reverse_iterator(begin()); }

    const_reverse_iterator crbegin() const { return std::make_reverse_iterator(cend()); }

    const_reverse_iterator crend() const { return std::make_reverse_iterator(cbegin()); }

    const_reverse_iterator rbegin() const noexcept { return crbegin(); }

    const_reverse_iterator rend() const noexcept { return crend(); }

    void insert(const_iterator pos, const T &value) {
        Node *newNode = NodeTraits::allocate(alloc, 1);
        try {
            NodeTraits::construct(alloc, &newNode->value, value);
        } catch (...) {
            NodeTraits::deallocate(alloc, newNode, 1);
            throw;
        }
        ++sz;
        newNode->next = pos.get_node();
        pos.get_node()->prev->next = newNode;
        newNode->prev = pos.get_node()->prev;
        pos.get_node()->prev = newNode;
    }

    List(const List &other) : List(std::allocator_traits<Alloc>::select_on_container_copy_construction(
        other.get_allocator())) {
        try {
            for (auto x = other.begin(); x != other.end(); ++x) {
                insert(end(), *x);
            }
        } catch (...) {
            for (int i = 0; i < sz; ++i) {
                erase(begin());
            }
        }
    }

    List(const size_t siz, const T &val, const Alloc &allocc = Alloc()) : fakeNode(), sz(0), alloc(allocc) {
        try {
            for (size_t i = 0; i < siz; ++i) {
                insert(end(), val);
            }
        } catch (...) {
            for (int i = 0; i < sz; ++i) {
                erase(begin());
            }
            throw;
        }
    }

    explicit List(const Alloc &alloc = Alloc()) : fakeNode(), sz(0), alloc(alloc) {}

    List(const size_t siz, const Alloc &allocc = Alloc()) : fakeNode(), sz(0), alloc(allocc) {
        try {
            for (size_t i = 0; i < siz; ++i) {
                const_iterator pos = end();
                Node *newNode = NodeTraits::allocate(alloc, 1);
                try {
                    NodeTraits::construct(alloc, &newNode->value);
                } catch (...) {
                    NodeTraits::deallocate(alloc, newNode, 1);
                    throw;
                }
                ++sz;
                newNode->next = pos.get_node();
                pos.get_node()->prev->next = newNode;
                newNode->prev = pos.get_node()->prev;
                pos.get_node()->prev = newNode;//
            }
        } catch (...) {
            for (int i = 0; i < sz; ++i) {
                erase(begin());
            }
        }
    }


    void erase(const_iterator pos) {
        --sz;
        pos.get_node()->next->prev = pos.get_node()->prev;
        pos.get_node()->prev->next = pos.get_node()->next;
        NodeTraits::destroy(alloc, static_cast<Node *>(pos.get_node()));
        NodeTraits::deallocate(alloc, static_cast<Node *>(pos.get_node()), 1);

    }

    ~List() {
        while (sz) {
            erase(begin());
        }
    }

    List &operator=(const List &other) {
        List copy_other(other);
        if constexpr (NodeTraits::propagate_on_container_copy_assignment::value) {
            alloc = other.get_allocator();
        }
        std::swap(copy_other.sz, sz);
        std::swap(copy_other.fakeNode, fakeNode);
        std::swap((&copy_other.fakeNode)->prev->next, (&fakeNode)->prev->next);
        std::swap((&copy_other.fakeNode)->next->prev, (&fakeNode)->next->prev);
        return *this;
    }

    void push_back(const T &val) {
        insert(end(), val);
    }

    void push_front(const T &val) {
        insert(begin(), val);
    }

    void pop_back() {
        erase(--end());
    }

    void pop_front() {
        erase(begin());
    }

    size_t size() const noexcept {
        return sz;
    }

    Alloc get_allocator() const noexcept {
        return alloc;
    }

};

template <size_t N>
class StackStorage {
private:
    char secret_data[N];
    void *data;
    size_t beginn;

public:
    StackStorage() {
        data = static_cast<void*>(secret_data);
        beginn = 0;
    }

    void *allocate(size_t n, size_t align) {
        if (beginn % align) {
            beginn += align - (beginn % align);
        }
        beginn += n;
        return reinterpret_cast<char *>(data) + beginn - n;
    }
};

template <typename T, size_t N>
class StackAllocator {
public:
    StackStorage<N> *stack_stor;

    using value_type = T;

    template <typename A>
    struct rebind {
        using other = StackAllocator<A, N>;
    };


    StackAllocator() = default;

    StackAllocator(StackStorage<N> &data) : stack_stor(&data) {}

    template <typename A>
    StackAllocator(const StackAllocator<A, N> &other) : stack_stor(other.stack_stor) {}

    template <typename A>
    StackAllocator &operator=(const StackAllocator<A, N> &other) {
        stack_stor = other.stack_stor;
        return *this;
    }

    value_type *allocate(size_t n) {
        return reinterpret_cast<value_type *>(
            stack_stor->allocate(n * sizeof(value_type), sizeof(value_type)));
    }

    void deallocate(value_type *, size_t) {}
};
