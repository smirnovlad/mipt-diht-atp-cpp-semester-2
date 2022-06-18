#include <iostream>
#include <cstddef>
#include <chrono>

template<size_t N>
class StackStorage {
 private:
  alignas(std::max_align_t) uint8_t storage_[N];
  uint8_t* top_ = storage_;
 public:
  StackStorage() noexcept;
  StackStorage(const StackStorage&) = delete;
  ~StackStorage() noexcept;
  StackStorage& operator=(const StackStorage&) = delete;

  template<typename T>
  T* allocate(size_t);

  void deallocate(uint8_t*, size_t) noexcept;
};

template<size_t N>
StackStorage<N>::StackStorage() noexcept = default;

template<size_t N>
StackStorage<N>::~StackStorage() noexcept = default;

template<size_t N>
template<typename T>
T* StackStorage<N>::allocate(size_t count) {
  // also we can use reinterpret_cast<uintptr_t> for ptr
  size_t bytes = count * sizeof(T);
  uint8_t* begin = top_ + alignof(T) - (top_ - storage_) % alignof(T);
  if (begin + bytes > storage_ + N) {
    throw std::bad_alloc();
  } else {
    top_ = begin + bytes;
  }
  return reinterpret_cast<T*>(begin);
}

template<size_t N>
void StackStorage<N>::deallocate([[maybe_unused]] uint8_t* pos, [[maybe_unused]] size_t count) noexcept {
  // empty because we are working with a StackAllocator
}

template<typename T, size_t N>
class StackAllocator {
 private:
  StackStorage<N>* storage_;
 public:
  using value_type = T;

  template<typename U>
  struct rebind {
    using other = StackAllocator<U, N>;
  };

  StackAllocator() = delete;
  StackAllocator(StackStorage<N>&) noexcept;
  template<typename U>
  StackAllocator(const StackAllocator<U, N>&) noexcept;
  ~StackAllocator();
  template<typename U>
  StackAllocator& operator=(const StackAllocator<U, N>&) noexcept;

  bool operator==(const StackAllocator<T, N>&) const;
  bool operator!=(const StackAllocator<T, N>&) const;

  T* allocate(size_t);
  void deallocate(T*, size_t) noexcept;

  template<typename U, size_t M>
  friend
  class StackAllocator;
};

template<typename T, size_t N>
StackAllocator<T, N>::StackAllocator(StackStorage<N>& storage) noexcept
    : storage_(&storage) {}

template<typename T, size_t N>
template<typename U>
StackAllocator<T, N>::StackAllocator(const StackAllocator<U, N>& other_alloc) noexcept
    : storage_(other_alloc.storage_) {}

template<typename T, size_t N>
StackAllocator<T, N>::~StackAllocator() = default;

template<typename T, size_t N>
template<typename U>
StackAllocator<T, N>& StackAllocator<T, N>::operator=(
    const StackAllocator<U, N>& other_alloc) noexcept {
  storage_ = other_alloc.storage_;
  return *this;
}

template<typename T, size_t N>
bool StackAllocator<T, N>::operator==(const StackAllocator<T, N>& other_alloc) const {
  return storage_ == other_alloc.storage_;
}

template<typename T, size_t N>
bool StackAllocator<T, N>::operator!=(const StackAllocator<T, N>& other_alloc) const {
  return storage_ != other_alloc.storage_;
}

template<typename T, size_t N>
T* StackAllocator<T, N>::allocate(size_t count) {
  return storage_->template allocate<T>(count);
}

template<typename T, size_t N>
void StackAllocator<T, N>::deallocate(T* pos, size_t count) noexcept {
  storage_->deallocate(reinterpret_cast<uint8_t*>(pos), count * sizeof(T));
}

template<typename T, typename Allocator = std::allocator<T>>
class List {
 private:
  struct BaseNode {
    BaseNode* prev;
    BaseNode* next;

    BaseNode() : prev(this), next(this) {}
  };

  BaseNode fake_node_;

  struct Node : BaseNode {
    T value;
    Node() = default;

    Node(const T& value) : value(value) {}
  };

  using inner_allocator_type = typename Allocator::template rebind<Node>::other;
  inner_allocator_type allocator_;

  size_t size_;

  template<bool is_const>
  class CommonIterator;

  using AllocTraits = std::allocator_traits<inner_allocator_type>;

  void add_elements(const List&);
  void swap(List<T, Allocator>&) noexcept;

 public:

  List() noexcept;
  List(size_t);
  List(size_t, const T&);
  List(const Allocator&) noexcept;
  List(size_t, const Allocator&);
  List(size_t, const T&, const Allocator&);

  List(const List&);
  ~List() noexcept;
  List& operator=(const List&);

  const inner_allocator_type& get_allocator() const;

  size_t size() const noexcept;

  using iterator = CommonIterator<false>;
  using const_iterator = CommonIterator<true>;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  template <typename... Args>
  iterator emplace(const_iterator, const Args&...);

  template <typename... Args>
  iterator insert(const_iterator, const Args&...);
  iterator erase(const_iterator) noexcept;

  void push_back(const T&);
  void push_back();
  void push_front(const T&);
  void pop_back() noexcept;
  void pop_front() noexcept;

  iterator begin() noexcept;
  const_iterator begin() const noexcept;
  const_iterator cbegin() const noexcept;
  iterator end() noexcept;
  const_iterator end() const noexcept;
  const_iterator cend() const noexcept;

  reverse_iterator rbegin() noexcept;
  const_reverse_iterator rbegin() const noexcept;
  const_reverse_iterator crbegin() const noexcept;
  reverse_iterator rend() noexcept;
  const_reverse_iterator rend() const noexcept;
  const_reverse_iterator crend() const noexcept;

 private:
  iterator begin_, end_;
};

template<typename T, typename Allocator>
List<T, Allocator>::List() noexcept: size_(0), begin_(&fake_node_), end_(&fake_node_) {}

template<typename T, typename Allocator>
List<T, Allocator>::List(size_t size) : List<T, Allocator>() {
  try {
    while (size-- > 0) {
      push_back();
    }
  } catch (...) {
    while (size_ > 0) {
      pop_back();
    }
    throw;
  }
}

template<typename T, typename Allocator>
List<T, Allocator>::List(size_t size, const T& value) : List<T, Allocator>() {
  try {
    while (size-- > 0) {
      push_back(value);
    }
  } catch (...) {
    while (size_ > 0) {
      pop_back();
    }
    throw;
  }
}

template<typename T, typename Allocator>
List<T, Allocator>::List(const Allocator& allocator) noexcept: allocator_(allocator),
                                                               size_(0), begin_(&fake_node_),
                                                               end_(&fake_node_) {}

template<typename T, typename Allocator>
List<T, Allocator>::List(size_t size, const Allocator& allocator)
    : List<T, Allocator>(allocator) {
  try {
    while (size-- > 0) {
      push_back();
    }
  } catch (...) {
    while (size_ > 0) {
      pop_back();
    }
    throw;
  }
}

template<typename T, typename Allocator>
List<T, Allocator>::List(size_t size, const T& value, const Allocator& allocator)
    : List<T, Allocator>(size, allocator) {
  for (auto it = begin_; it != end_; ++it) {
    try {
      *it = value;
    } catch (...) {
      while (size_ > 0) {
        pop_back();
      }
      throw;
    }
  }
}

template<typename T, typename Allocator>
void List<T, Allocator>::add_elements(const List<T, Allocator>& arg_list) {
  for (auto it = arg_list.begin(); it != arg_list.end(); ++it) {
    try {
      push_back(*it);
    } catch (...) {
      while (size_ > 0) {
        pop_back();
      }
      throw;
    }
  }
}

template<typename T, typename Allocator>
List<T, Allocator>::List(const List& other_list) :
    List(AllocTraits::select_on_container_copy_construction(other_list.allocator_)) {
  add_elements(other_list);
}

template<typename T, typename Allocator>
List<T, Allocator>::~List() noexcept {
  while (size_ > 0) {
    pop_back();
  }
}

template<typename T, typename Allocator>
void List<T, Allocator>::swap(List<T, Allocator>& arg_list) noexcept {
  std::swap(fake_node_, arg_list.fake_node_);
  std::swap(fake_node_.prev->next, arg_list.fake_node_.prev->next);
  std::swap(fake_node_.next->prev, arg_list.fake_node_.next->prev);

  std::swap(size_, arg_list.size_);
  std::swap(begin_, arg_list.begin_);
  std::swap(allocator_, arg_list.allocator_);
}

template<typename T, typename Allocator>
List<T, Allocator>& List<T, Allocator>::operator=(const List& arg_list) {
  List<T, Allocator> tmp_list(AllocTraits::propagate_on_container_copy_assignment::value ? arg_list.allocator_ : allocator_);
  tmp_list.add_elements(arg_list);
  swap(tmp_list);
  return *this;
}

template<typename T, typename Allocator>
const typename List<T, Allocator>::inner_allocator_type& List<T, Allocator>::get_allocator() const {
  return allocator_;
}

template<typename T, typename Allocator>
size_t List<T, Allocator>::size() const noexcept {
  return size_;
}

template<typename T, typename Allocator>
template<typename... Args>
typename List<T, Allocator>::iterator List<T, Allocator>::emplace(List::const_iterator iter, const Args&... args) {
  auto ptr = reinterpret_cast<BaseNode*>(AllocTraits::allocate(allocator_, 1));
  try {
    AllocTraits::construct(allocator_, reinterpret_cast<Node*>(ptr), args...);
  } catch (...) {
    AllocTraits::deallocate(allocator_, reinterpret_cast<Node*>(ptr), 1);
    throw;
  }
  ++size_;
  auto cur = ptr;
  auto prev = (iter.get_node())->prev, next = reinterpret_cast<BaseNode*>(iter.get_node());
  cur->prev = prev;
  cur->next = next;
  prev->next = cur;
  next->prev = cur;
  if (iter == begin_) {
    begin_ = iterator(cur);
  }
  return iterator(cur);
}

template<typename T, typename Allocator>
void List<T, Allocator>::push_back(const T& value) {
  emplace(end_, value);
}

template<typename T, typename Allocator>
void List<T, Allocator>::push_back() {
  emplace(end_);
}

template<typename T, typename Allocator>
void List<T, Allocator>::push_front(const T& value) {
  emplace(begin_, value);
}


template<typename T, typename Allocator>
void List<T, Allocator>::pop_back() noexcept {
  erase(std::prev(end_));
}


template<typename T, typename Allocator>
void List<T, Allocator>::pop_front() noexcept {
  erase(begin_);
}

template<typename T, typename Allocator>
template <typename... Args>
typename List<T, Allocator>::iterator List<T, Allocator>::insert(
    List::const_iterator iter, const Args&... args) {
  return emplace(iter, args...);
}

template<typename T, typename Allocator>
typename List<T, Allocator>::iterator List<T, Allocator>::erase(
    List::const_iterator iter) noexcept {
  BaseNode* node_to_delete = iter.get_node();
  BaseNode* prev = node_to_delete->prev;
  BaseNode* next = node_to_delete->next;
  prev->next = next;
  next->prev = prev;
  AllocTraits::destroy(allocator_, reinterpret_cast<Node*>(node_to_delete));
  AllocTraits::deallocate(allocator_, reinterpret_cast<Node*>(node_to_delete), 1);
  --size_;
  if (iter == begin_) {
    begin_ = iterator(next);
  }
  return iterator(next);
}

template<typename T, typename Allocator>
typename List<T, Allocator>::iterator List<T, Allocator>::begin() noexcept {
  return begin_;
}

template<typename T, typename Allocator>
typename List<T, Allocator>::const_iterator List<T, Allocator>::begin() const noexcept {
  return cbegin();
}

template<typename T, typename Allocator>
typename List<T, Allocator>::const_iterator List<T, Allocator>::cbegin() const noexcept {
  return static_cast<const_iterator>(begin_);
}

template<typename T, typename Allocator>
typename List<T, Allocator>::iterator List<T, Allocator>::end() noexcept {
  return end_;
}

template<typename T, typename Allocator>
typename List<T, Allocator>::const_iterator List<T, Allocator>::end() const noexcept {
  return cend();
}

template<typename T, typename Allocator>
typename List<T, Allocator>::const_iterator List<T, Allocator>::cend() const noexcept {
  return static_cast<const_iterator>(end_);
}

template<typename T, typename Allocator>
typename List<T, Allocator>::reverse_iterator List<T, Allocator>::rbegin() noexcept {
  return reverse_iterator(end_);
}

template<typename T, typename Allocator>
typename List<T, Allocator>::const_reverse_iterator List<T, Allocator>::rbegin() const noexcept {
  return crbegin();
}

template<typename T, typename Allocator>
typename List<T, Allocator>::const_reverse_iterator List<T, Allocator>::crbegin() const noexcept {
  return const_reverse_iterator(end_);
}

template<typename T, typename Allocator>
typename List<T, Allocator>::reverse_iterator List<T, Allocator>::rend() noexcept {
  return reverse_iterator(begin_);
}

template<typename T, typename Allocator>
typename List<T, Allocator>::const_reverse_iterator List<T, Allocator>::rend() const noexcept {
  return crend();
}

template<typename T, typename Allocator>
typename List<T, Allocator>::const_reverse_iterator List<T, Allocator>::crend() const noexcept {
  return const_reverse_iterator(begin_);
}

template<typename T, typename Allocator>
template<bool is_const>
class List<T, Allocator>::CommonIterator {
 private:
  BaseNode* node_;

 public:
  CommonIterator() noexcept;
  CommonIterator(BaseNode*) noexcept;

  using value_type = T;
  using iterator_category = std::bidirectional_iterator_tag;
  using difference_type = ssize_t;
  using reference = typename std::conditional<is_const, const T&, T&>::type;
  using pointer = typename std::conditional<is_const, const T*, T*>::type;

  operator CommonIterator<true>() const noexcept;

  CommonIterator<is_const> operator--(int) noexcept;
  CommonIterator<is_const> operator++(int) noexcept;
  CommonIterator<is_const>& operator--() noexcept;
  CommonIterator<is_const>& operator++() noexcept;

  reference operator*() const noexcept;
  pointer operator->() const noexcept;

  bool operator==(const CommonIterator<is_const>&) const noexcept;
  bool operator!=(const CommonIterator<is_const>&) const noexcept;

  BaseNode* get_node() const noexcept;
};

template<typename T, typename Allocator>
template<bool is_const>
List<T, Allocator>::CommonIterator<is_const>::CommonIterator() noexcept = default;

template<typename T, typename Allocator>
template<bool is_const>
List<T, Allocator>::CommonIterator<is_const>::CommonIterator(BaseNode* node) noexcept
    : node_(node) {}

template<typename T, typename Allocator>
template<bool is_const>
List<T, Allocator>::CommonIterator<is_const>::operator CommonIterator<true>() const noexcept {
  return CommonIterator<true>(node_);
}

template<typename T, typename Allocator>
template<bool is_const>
typename List<T, Allocator>::template CommonIterator<is_const>&
List<T, Allocator>::CommonIterator<is_const>::operator--() noexcept {
  node_ = node_->prev;
  return *this;
}

template<typename T, typename Allocator>
template<bool is_const>
typename List<T, Allocator>::template CommonIterator<is_const>&
List<T, Allocator>::CommonIterator<is_const>::operator++() noexcept {
  node_ = node_->next;
  return *this;
}

template<typename T, typename Allocator>
template<bool is_const>
typename List<T, Allocator>::template CommonIterator<is_const>
List<T, Allocator>::CommonIterator<is_const>::operator--(int) noexcept {
  CommonIterator<is_const> tmp(*this);
  --(*this);
  return tmp;
}

template<typename T, typename Allocator>
template<bool is_const>
typename List<T, Allocator>::template CommonIterator<is_const>
List<T, Allocator>::CommonIterator<is_const>::operator++(int) noexcept {
  CommonIterator<is_const> tmp(*this);
  ++(*this);
  return tmp;
}

template<typename T, typename Allocator>
template<bool is_const>
typename List<T, Allocator>::template CommonIterator<is_const>::reference
List<T, Allocator>::CommonIterator<is_const>::operator*() const noexcept {
  return reinterpret_cast<Node*>(node_)->value;
}

template<typename T, typename Allocator>
template<bool is_const>
typename List<T, Allocator>::template CommonIterator<is_const>::pointer
List<T, Allocator>::CommonIterator<is_const>::operator->() const noexcept {
  return &(operator*());
};

template<typename T, typename Allocator>
template<bool is_const>
bool List<T, Allocator>::CommonIterator<is_const>::operator==(
    const List<T, Allocator>::CommonIterator<is_const>& other_iter) const noexcept {
  return (node_ == other_iter.node_);
}

template<typename T, typename Allocator>
template<bool is_const>
bool List<T, Allocator>::CommonIterator<is_const>::operator!=(
    const List<T, Allocator>::CommonIterator<is_const>& other_iter) const noexcept {
  return !(*this == other_iter);
}

template<typename T, typename Allocator>
template<bool is_const>
typename List<T, Allocator>::BaseNode*
List<T, Allocator>::CommonIterator<is_const>::get_node() const noexcept {
  return reinterpret_cast<BaseNode*>(node_);
}