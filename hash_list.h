#ifndef HASH_LIST_H
#define HASH_LIST_H

#include <functional>
#include <iterator>
#include <list>
#include <unordered_map>
#include <utility>

template<typename T>
class hash_list {
    friend class iterator;
    friend class reference;
    class reference;
    class iterator;

    std::list<T> data;
    std::unordered_map<T, typename decltype(data)::iterator> positions;

    using value_type = T;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using reference = reference;
    using const_reference = const T &;
    using iterator = iterator;
    using const_iterator = typename decltype(data)::const_iterator;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

public:
    const T & back() const {
        return data.back();
    }
    reference back() {
        return reference(*this, data.back());
    }
    const_iterator begin() const {
        return std::begin(data);
    }
    iterator begin() {
        return iterator(*this, *std::begin(data));
    }
    void clear() {
        data.clear();
        positions.clear();
    }
    template<typename... Args>
    void emplace_back(Args &&... args) {
        data.emplace_back(std::forward<Args...>(args...));
        positions.emplace(std::piecewise_construct, std::forward_as_tuple(args...), std::forward_as_tuple(std::prev(std::end(data))));
    }
    bool empty() const noexcept {
        return data.empty();
    }
    decltype(std::end(data)) end() const {
        return std::end(data);
    }
    iterator end() {
        return iterator(*this, *std::end(data));
    }
    void erase(const T & expired) {
        auto it = positions.find(expired);
        if (it != std::end(positions)) {
            data.erase(it->second);
            positions.erase(it);
        }
    }
    const T & front() const {
        return data.front();
    }
    reference front() {
        return reference(*this, data.front());
    }
    void replace(const T & oldValue, const T & newValue) {
        auto it = positions.find(oldValue);
        if (it != std::end(positions)) {
            *it->second = newValue;//replace value
            //new mapping to unchanged position
            positions.emplace(std::piecewise_construct, std::forward_as_tuple(newValue), std::forward_as_tuple(it->second));
            positions.erase(oldValue);//remove old mapping
        }
    }
    std::size_t size() const {
        return data.size();
    }
};

template<typename T>
class hash_list<T>::reference {
    std::reference_wrapper<hash_list> owner;
    std::reference_wrapper<T> value;
    friend class hash_list;
    reference(hash_list & owner, T & value) : owner{owner}, value{value} {}
public:
    reference(reference &&) = default;
    reference & operator=(reference &&) = default;
    reference & operator=(const T & newValue) {
        auto it = owner.positions.find(value);
        owner.positions.emplace(std::piecewise_construct, std::forward_as_tuple(newValue), std::forward_as_tuple(it->second));
        owner.positions.erase(value);
        value = newValue;
        return *this;
    }
    operator T() const {
        return value;
    }
    friend void swap(reference lhs, reference rhs) {
        std::swap(lhs, rhs);
    }
};

template<typename T>
class hash_list<T>::iterator : public std::iterator<std::bidirectional_iterator_tag, T, std::ptrdiff_t, reference*, reference> {
    hash_list & owner;
    T & value;
    friend class hash_list;
    iterator(hash_list & owner, T & value) : owner{owner}, value{value} {}
public:
    reference operator*() {
        return reference(owner, value);
    }
    T operator*() const {
        return value;
    }
    iterator operator++() {
        //lookup list iterator, increment and return iterator of its value
        return iterator(owner, *std::next(owner.positions[value]));
    }
    iterator operator--() {
        //lookup list iterator, increment and return iterator of its value
        return iterator(owner, *std::prev(owner.positions[value]));
    }
};

#endif//HASH_LIST_H
