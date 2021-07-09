/*
 *  This file is a part of KNOSSOS.
 *
 *  (C) Copyright 2007-2018
 *  Max-Planck-Gesellschaft zur Foerderung der Wissenschaften e.V.
 *
 *  KNOSSOS is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 of
 *  the License as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *
 *  For further information, visit https://knossos.app
 *  or contact knossosteam@gmail.com
 */

#pragma once

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
//    using reference = reference;
    using const_reference = const T &;
//    using iterator = iterator;
    using const_iterator = typename decltype(data)::const_iterator;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

public:
    hash_list() = default;
    hash_list(const hash_list & rhs) {
        operator=(rhs);
    }
    hash_list & operator=(const hash_list & rhs) {
        for (auto & elem : rhs.data) {
            emplace_back(elem);
        }
        return *this;
    }
    const T & back() const {
        return data.back();
    }
    reference back() {
        return reference(*this, data.back());
    }
    const_iterator cbegin() const {
        return data.cbegin();
    }
    const_iterator begin() const {
        return data.cbegin();
    }
    iterator begin() {
        return iterator(*this, std::begin(data));
    }
    void clear() {
        data.clear();
        positions.clear();
    }
    template<typename... Args>
    void emplace_back(Args &&... args) {
        data.emplace_back(std::forward<Args...>(args...));
        auto it = std::prev(std::end(data));
        positions.emplace(std::piecewise_construct, std::forward_as_tuple(*it), std::forward_as_tuple(it));
    }
    template<typename... Args>
    void emplace_front(Args &&... args) {
        data.emplace_front(std::forward<Args...>(args...));
        auto it = std::begin(data);
        positions.emplace(std::piecewise_construct, std::forward_as_tuple(*it), std::forward_as_tuple(it));
    }
    bool empty() const noexcept {
        return data.empty();
    }
    const_iterator cend() const {
        return data.cend();
    }
    const_iterator end() const {
        return data.cend();
    }
    iterator end() {
        return iterator(*this, std::end(data));
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
    reference(const reference &) = delete;
    reference & operator=(const reference &) = delete;
    reference(reference &&) = default;
    reference & operator=(reference &&) = default;
    reference & operator=(const T & newValue) {
        owner.replace(value, newValue);
        assert(newValue == value && *owner.positions[value]->second == value);
        return *this;
    }
    operator T() const {
        return value;
    }
};

template<typename T>
class hash_list<T>::iterator : public std::iterator<std::bidirectional_iterator_tag, T, std::ptrdiff_t, reference*, reference> {
    hash_list & owner;
    typename decltype(hash_list<T>::data)::iterator it;
    friend class hash_list;
    iterator(decltype(owner) & owner, const decltype(it) & it) : owner{owner}, it{it} {}
public:
    bool operator!=(const iterator & other) {
        return it != other.it;
    }
    reference operator*() {
        return reference(owner, *it);
    }
    T operator*() const {
        return *it;
    }
    iterator & operator++() {
        it = std::next(it);
        return *this;
    }
    iterator & operator--() {
        it = std::prev(it);
        return *this;
    }
};
