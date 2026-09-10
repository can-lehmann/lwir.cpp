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

#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <cstring>

namespace lwir {
  class ArenaAllocator {
  private:
    struct Chunk {
      Chunk* next = nullptr;
      uint8_t data[0];
    };

    static constexpr size_t CHUNK_SIZE = 1024 * 1024; // 1 MiB
    static constexpr size_t USABLE_SIZE = CHUNK_SIZE - sizeof(Chunk);

    Chunk* _first = nullptr;
    Chunk* _current = nullptr;

    size_t _left = 0;
    uint8_t* _ptr = nullptr;

    inline size_t align_pad(void* ptr, size_t align) {
      size_t delta = (uintptr_t) ptr % align;
      return delta ? align - delta : 0;
    }
  public:
    ArenaAllocator() {
      _first = (Chunk*) malloc(CHUNK_SIZE);
      new (_first) Chunk();
      _current = _first;
    }

    ~ArenaAllocator() {
      Chunk* chunk = _first;
      while (chunk) {
        Chunk* next = chunk->next;
        free(chunk);
        chunk = next;
      }
    }

    void* alloc(size_t size, size_t align) {
      assert(size <= USABLE_SIZE);

      size_t align_padding = align_pad(_ptr, align);

      if (__builtin_expect(_left - align_padding < size, 0)) {
        if (_current->next) {
          _current = _current->next;
        } else {
          Chunk* chunk = (Chunk*) malloc(CHUNK_SIZE);
          new (chunk) Chunk();
          _current->next = chunk;
          _current = chunk;
        }
        _left = USABLE_SIZE;
        _ptr = _current->data;
        align_padding = align_pad(_ptr, align);
      }

      _ptr += align_padding;
      _left -= align_padding;
      void* ptr = (void*) _ptr;
      _ptr += size;
      _left -= size;
      return ptr;
    }

    template <class T>
    T* alloc() {
      return (T*) alloc(sizeof(T), alignof(T));
    }

    void dealloc_all() {
      _current = _first;
      _ptr = _first->data;
      _left = USABLE_SIZE;
    }

    void zero_all() {
      Chunk* chunk = _first;
      while (chunk) {
        std::memset(chunk->data, 0, USABLE_SIZE);
        chunk = chunk->next;
      }
      dealloc_all();
    }
  };

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
    bool empty() const { return _size == 0; }

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

    inline T front() const { return at(0); }
    inline T back() const { return at(_size - 1); }

    Span zeroed() const {
      memset(_data, 0, sizeof(T) * _size);
      return *this;
    }

    Span with(size_t index, const T& value) const {
      assert(index < _size);
      _data[index] = value;
      return *this;
    }

    template <class Iterator>
    Span copy_from(Iterator begin, Iterator end) const {
      size_t index = 0;
      for (Iterator iter = begin; iter != end; iter++) {
        assert(index < _size);
        _data[index++] = *iter;
      }
      assert(index == _size);
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

    void remove(T* first, T* last) {
      if (first->prev()) {
        first->prev()->set_next(last->next());
      } else {
        _first = last->next();
      }
      if (last->next()) {
        last->next()->set_prev(first->prev());
      } else {
        _last = first->prev();
      }
      first->set_prev(nullptr);
      last->set_next(nullptr);
    }

    void add(T* first, T* last) {
      assert(!first->prev() && !last->next());
      first->set_prev(_last);
      if (_last) {
        _last->set_next(first);
      } else {
        _first = first;
      }
      _last = last;
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

  template <class Block>
  class DefaultBlockMap {
  public:
    template <class T>
    class Map {
    private:
      std::unordered_map<Block*, T> _map;
    public:
      Map() {}
      Map(size_t) {}

      T& at(Block* block) { return _map[block]; }
      T& operator[](Block* block) { return _map[block]; }
    };
  };

  template <class Self, class Block, template <class> class BlockMap = DefaultBlockMap<Block>::template Map>
  class DominatorTreeBase {
  private:
    size_t _block_count;
    BlockMap<Block*> _idom;

    std::vector<Block*> successors(Block* block) {
      return static_cast<Self*>(this)->successors(block);
    }

    void traverse(Block* block,
                  BlockMap<std::vector<Block*>>& incoming,
                  std::vector<Block*>& post_order,
                  BlockMap<size_t>& nums) {
      assert(block);
      for (Block* succ : successors(block)) {
        if (!_idom[succ]) {
          _idom[succ] = block;
          traverse(succ, incoming, post_order, nums);
        }
        incoming[succ].push_back(block);
      }
      nums[block] = post_order.size();
      post_order.push_back(block);
    }

  protected:
    DominatorTreeBase(size_t block_count): _block_count(block_count), _idom(block_count) {}

    // Loosely based on ideas from Keith D. Cooper, Timothy J. Harvey,
    // and Ken Kennedy "A Simple, Fast Dominance Algorithm"
    void build(Block* entry) {
      BlockMap<std::vector<Block*>> incoming(_block_count);
      std::vector<Block*> post_order;
      BlockMap<size_t> nums(_block_count);

      traverse(entry, incoming, post_order, nums);

      post_order.pop_back();

      bool changed = true;
      while (changed) {
        changed = false;

        for (size_t it = post_order.size(); it-- > 0; ) {
          Block* block = post_order[it];

          Block* idom = _idom[block];
          assert(idom);
          for (Block* pred : incoming[block]) {
            while (pred != idom) {
              if (nums[pred] < nums[idom]) {
                pred = _idom[pred];
              } else {
                idom = _idom[idom];
              }
            }
          }

          if (idom != _idom[block]) {
            _idom[block] = idom;
            changed = true;
          }
        }
      }
    }

  public:
    Block* idom(Block* block) {
      return _idom[block];
    }

    bool dominates(Block* dominator, Block* dominated) {
      while (dominated && dominated != dominator) {
        dominated = idom(dominated);
      }
      return dominated == dominator;
    }
  };

  template <class Self, class T>
  class BaseFlags {
  protected:
    T _flags = 0;

    T mask() const {
      if (Self::COUNT == sizeof(T) * 8) {
        return (T) -1;
      } else {
        assert(Self::COUNT < sizeof(T) * 8);
        return (T(1) << Self::COUNT) - 1;
      }
    }
  public:
    BaseFlags(T flags = 0): _flags(flags) {}

    explicit operator T() const {
      return _flags;
    }

    explicit operator uint64_t() const {
      return _flags;
    }

    bool has(Self flag) const {
      return (_flags & flag._flags) != 0;
    }

    bool operator==(const Self& other) const {
      return _flags == other._flags;
    }

    bool operator!=(const Self& other) const {
      return !(*this == other);
    }

    Self operator|(const Self& other) const {
      return Self(_flags | other._flags);
    }

    Self& operator|=(const Self& other) {
      _flags |= other._flags;
      return (Self&) *this;
    }

    Self operator&(const Self& other) const {
      return Self(_flags & other._flags);
    }

    Self& operator&=(const Self& other) {
      _flags &= other._flags;
      return (Self&) *this;
    }

    Self operator~() const {
      return Self(~_flags & mask());
    }

    class name_iterator {
    private:
      size_t _index = 0;
      Self _flags;

      void advance() {
        while (_index < Self::COUNT && !_flags.has(Self(1 << _index))) {
          _index++;
        }
      }
    public:
      name_iterator(Self flags): _index(0), _flags(flags) {
        advance();
      }
      name_iterator(Self flags, size_t index): _index(index), _flags(flags) {}

      const char* operator*() const {
        assert(_index < Self::COUNT);
        return Self::NAMES[_index];
      }

      name_iterator& operator++() {
        _index++;
        advance();
        return *this;
      }

      bool operator==(const name_iterator& other) const {
        return _index == other._index && _flags == other._flags;
      }

      bool operator!=(const name_iterator& other) const {
        return !(*this == other);
      }
    };

    Range<name_iterator> names() const {
      return Range<name_iterator>(
        name_iterator(*((Self*) this)),
        name_iterator(*((Self*) this), Self::COUNT)
      );
    }

    void write(std::ostream& stream) const {
      stream << "{";
      bool is_first = true;
      for (const char* name : names()) {
        if (!is_first) { stream << ", "; }
        is_first = false;
        stream << name;
      }
      stream << "}";
    }

    void write_json(std::ostream& stream) const {
      stream << "[";
      bool is_first = true;
      for (const char* name : names()) {
        if (!is_first) { stream << ", "; }
        is_first = false;
        stream << "\"" << name << "\"";
      }
      stream << "]";
    }
  };
}
