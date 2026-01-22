#pragma once

// Copyright 2025 Can Joshua Lehmann
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

namespace lwir {
  template <class T>
  class Span {
  private:
    T* _data = nullptr;
    size_t _size = 0;
  public:
    Span() {}
    Span(T* data, size_t size): _data(data), _size(size) {}
    
    template <class Ptr>
    static Span<T> offset(Ptr* base, size_t offset, size_t size) {
      return Span<T>((T*) ((uint8_t*) base + offset), size);
    }

    template <class Ptr>
    static Span<T> trailing(Ptr* base, size_t size) {
      return Span<T>((T*) ((uint8_t*) base + sizeof(Ptr)), size);
    }

    T* data() const { return _data; }

    size_t size() const { return _size; }
    
    inline T at(size_t index) const {
      assert(index < _size);
      return _data[index];
    }

    inline T& at(size_t index) {
      assert(index < _size);
      return _data[index];
    }

    inline T operator[](size_t index) const { return at(index); }
    inline T& operator[](size_t index) { return at(index); }

    T* begin() const { return _data; }
    T* end() const { return _data + _size; }

    Span zeroed() const {
      memset(_data, 0, sizeof(T) * _size);
      return *this;
    }

    Span with(size_t index, const T& value) const {
      assert(index < _size);
      _data[index] = value;
      return *this;
    }
  };


  template <class T>
  class LinkedListItem {
  private:
    T* _prev = nullptr;
    T* _next = nullptr;
  public:
    LinkedListItem() {}

    inline T* prev() const { return _prev; }
    inline void set_prev(T* prev) { _prev = prev; }

    inline T* next() const { return _next; }
    inline void set_next(T* next) { _next = next; }
  };

  template <class T>
  class Range {
  private:
    T _begin;
    T _end;
  public:
    Range(const T& begin, const T& end): _begin(begin), _end(end) {}

    T begin() const { return _begin; }
    T end() const { return _end; }
  };

  template <class T>
  class LinkedList {
  private:
    T* _first = nullptr;
    T* _last = nullptr;
  public:
    LinkedList() {}

    T* first() const { return _first; }
    T* last() const { return _last; }

    bool empty() const { return _first == nullptr; }

    void add(T* item) {
      assert(!item->prev() && !item->next());
      item->set_prev(_last);
      if (_last) {
        _last->set_next(item);
      } else {
        _first = item;
      }
      _last = item;
    }

    void insert_before(T* before, T* item) {
      assert(!item->prev() && !item->next());
      if (before == nullptr) {
        add(item);
      } else {
        item->set_next(before);
        item->set_prev(before->prev());
        if (before->prev()) {
          before->prev()->set_next(item);
        } else {
          _first = item;
        }
        before->set_prev(item);
      }
    }

    void remove(T* item) {
      if (item->prev()) {
        item->prev()->set_next(item->next());
      } else {
        _first = item->next();
      }
      if (item->next()) {
        item->next()->set_prev(item->prev());
      } else {
        _last = item->prev();
      }
      item->set_prev(nullptr);
      item->set_next(nullptr);
    }

    class iterator {
    private:
      LinkedList* _list;
      T* _item;
    public:
      iterator(LinkedList* list, T* item): _list(list), _item(item) {}
      
      T* operator*() const { return _item; }
      
      iterator& operator++() { 
        _item = _item->next(); 
        return *this;
      }

      iterator operator++(int) { 
        iterator iter = *this;
        ++(*this);
        return iter;
      }

      bool operator==(const iterator& other) const { return _item == other._item; }
      bool operator!=(const iterator& other) const { return !(*this == other); }

      iterator erase() {
        T* next = _item->next();
        _list->remove(_item);
        return iterator(_list, next);
      }

      iterator at(T* new_item) {
        return iterator(_list, new_item);
      }
    };

    iterator begin() { return iterator(this, _first); }
    iterator end() { return iterator(this, nullptr); }
    
    class reverse_iterator {
    private:
      LinkedList* _list;
      T* _item;
    public:
      reverse_iterator(LinkedList* list, T* item): _list(list), _item(item) {}

      T* operator*() const { return _item; }

      reverse_iterator& operator++() { 
        _item = _item->prev(); 
        return *this;
      }

      reverse_iterator operator++(int) { 
        reverse_iterator iter = *this;
        ++(*this);
        return iter;
      }

      bool operator==(const reverse_iterator& other) const { return _item == other._item; }
      bool operator!=(const reverse_iterator& other) const { return !(*this == other); }
      
      reverse_iterator erase() {
        T* prev = _item->prev();
        _list->remove(_item);
        return reverse_iterator(_list, prev);
      }
    };

    reverse_iterator rbegin() { return reverse_iterator(this, _last); }
    reverse_iterator rend() { return reverse_iterator(this, nullptr); }

    Range<iterator> range() { return Range<iterator>(begin(), end()); }
    Range<reverse_iterator> rev_range() { return Range<reverse_iterator>(rbegin(), rend()); }
  };
}
