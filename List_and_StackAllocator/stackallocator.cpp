#include <iostream>
#include <cstddef>
#include <chrono>

template<size_t N>
class StackStorage {
 private:
  alignas(std::max_align_t) uint8_t storage_[N];
  uint8_t* top_ = storage_;
 public:
  StackStorage();
  StackStorage(const StackStorage&) = delete;
  ~StackStorage();
  StackStorage& operator=(const StackStorage&) = delete;

  template<typename T>
  T* allocate(size_t);

  void deallocate(uint8_t*, size_t);
};

template<size_t N>
StackStorage<N>::StackStorage() = default;

template<size_t N>
StackStorage<N>::~StackStorage() = default;

template<size_t N>
template<typename T>
T* StackStorage<N>::allocate(size_t count) {
  // also we can use reinterpret_cast<uintptr_t> for ptr
  size_t bytes = count * sizeof(T);
  uint8_t* begin = top_ + alignof(T) - (top_ - storage_) % alignof(T);
  top_ = begin + bytes;
  return reinterpret_cast<T*>(begin);
}

template<size_t N>
void StackStorage<N>::deallocate([[maybe_unused]] uint8_t* pos, [[maybe_unused]] size_t count) {
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
  void deallocate(T*, size_t);

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
void StackAllocator<T, N>::deallocate(T* pos, size_t count) {
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

 public:

  List();
  List(size_t);
  List(size_t, const T&);
  List(const Allocator&);
  List(size_t, const Allocator&);
  List(size_t, const T&, const Allocator&);

  List(const List&);
  ~List();
  List& operator=(const List&);

  const inner_allocator_type& get_allocator() const;

  size_t size() const;

  void push_back(const T&);
  void push_back();
  void push_front(const T&);
  void pop_back();
  void pop_front();

  using iterator = CommonIterator<false>;
  using const_iterator = CommonIterator<true>;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  iterator begin();
  const_iterator begin() const;
  const_iterator cbegin() const;
  iterator end();
  const_iterator end() const;
  const_iterator cend() const;

  reverse_iterator rbegin();
  const_reverse_iterator rbegin() const;
  const_reverse_iterator crbegin() const;
  reverse_iterator rend();
  const_reverse_iterator rend() const;
  const_reverse_iterator crend() const;

  iterator insert(const_iterator, const T&);
  iterator insert(const_iterator);
  iterator erase(const_iterator);
 private:
  iterator begin_, end_;
};

template<typename T, typename Allocator>
List<T, Allocator>::List() : size_(0), begin_(&fake_node_), end_(&fake_node_) {}

template<typename T, typename Allocator>
List<T, Allocator>::List(size_t size) : List<T, Allocator>() {
  while (size-- > 0) {
    push_back(T());
  }
}

template<typename T, typename Allocator>
List<T, Allocator>::List(size_t size, const T& value) : List<T, Allocator>(size) {
  for (auto it = begin_; it != end_; ++it) {
    *it = value;
  }
}

template<typename T, typename Allocator>
List<T, Allocator>::List(const Allocator& allocator) : allocator_(allocator),
                                                       size_(0), begin_(&fake_node_),
                                                       end_(&fake_node_) {}

template<typename T, typename Allocator>
List<T, Allocator>::List(size_t size, const Allocator& allocator)
    : List<T, Allocator>(allocator) {
  while (size-- > 0) {
    push_back();
  }
}

template<typename T, typename Allocator>
List<T, Allocator>::List(size_t size, const T& value, const Allocator& allocator)
    : List<T, Allocator>(size, allocator) {
  for (auto it = begin_; it != end_; ++it) {
    *it = value;
  }
}

template<typename T, typename Allocator>
List<T, Allocator>::List(const List& other_list) :
    List(AllocTraits::select_on_container_copy_construction(other_list.allocator_)) {
  for (auto it = other_list.begin(); it != other_list.end(); ++it) {
    push_back(*it);
  }
}

template<typename T, typename Allocator>
List<T, Allocator>::~List() {
  //std::cout << "~List:\n";
  while (size_ > 0) {
    //std::cout << "size_ = " << size_ << '\n';
    pop_back();
  }
}

template<typename T, typename Allocator>
List<T, Allocator>& List<T, Allocator>::operator=(const List& other_list) {
  size_t tmp_size = size_;
  iterator tmp_begin = begin_, tmp_end = end_;
  BaseNode tmp_fake_node = fake_node_;
  inner_allocator_type tmp_allocator = allocator_;
  if (AllocTraits::propagate_on_container_copy_assignment::value) {
    allocator_ = other_list.allocator_;
  }
  try {
    size_ = 0;
    fake_node_ = BaseNode();
    begin_ = end_ = &fake_node_;
    for (auto it = other_list.begin(); it != other_list.end(); ++it) {
      push_back(*it);
    }
  } catch (...) {
    while (size_ > 0) {
      pop_back();
    }
    size_ = tmp_size;
    begin_ = tmp_begin;
    end_ = tmp_end;
    fake_node_ = tmp_fake_node;
    allocator_ = tmp_allocator;
    throw;
  }
  for (iterator it = tmp_begin; it != tmp_end; ++it) {
    //std::cout << "it = " << it.get_node() << '\n';
    AllocTraits::destroy(tmp_allocator, reinterpret_cast<Node*>(it.get_node()));
  }
  AllocTraits::deallocate(tmp_allocator, reinterpret_cast<Node*>(tmp_begin.get_node()), tmp_size);
  return *this;
}

template<typename T, typename Allocator>
const typename List<T, Allocator>::inner_allocator_type& List<T, Allocator>::get_allocator() const {
  return allocator_;
}

template<typename T, typename Allocator>
size_t List<T, Allocator>::size() const {
  return size_;
}

template<typename T, typename Allocator>
void List<T, Allocator>::push_back(const T& value) {
  insert(end_, value);
}

template<typename T, typename Allocator>
void List<T, Allocator>::push_back() {
  insert(end_);
}

template<typename T, typename Allocator>
void List<T, Allocator>::push_front(const T& value) {
  insert(begin_, value);
}


template<typename T, typename Allocator>
void List<T, Allocator>::pop_back() {
  erase(std::prev(end_));
}


template<typename T, typename Allocator>
void List<T, Allocator>::pop_front() {
  erase(begin_);
}

template<typename T, typename Allocator>
typename List<T, Allocator>::iterator List<T, Allocator>::insert(
    List::const_iterator iter, const T& value) {
  auto ptr = reinterpret_cast<BaseNode*>(AllocTraits::allocate(allocator_, 1));
  // AllocTraits::construct(allocator_,  reinterpret_cast<Node*>(ptr), Node(value)); - superfluous construction
  AllocTraits::construct(allocator_, reinterpret_cast<Node*>(ptr), value);
  ++size_;
  auto cur = reinterpret_cast<BaseNode*>(ptr);
  //std::cout << "ADDED: " << cur << '\n';
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
typename List<T, Allocator>::iterator List<T, Allocator>::insert(
    List::const_iterator iter) {
  auto ptr = reinterpret_cast<BaseNode*>(AllocTraits::allocate(allocator_, 1));
  AllocTraits::construct(allocator_, reinterpret_cast<Node*>(ptr));
  ++size_;
  auto cur = reinterpret_cast<BaseNode*>(ptr);
  //std::cout << "ADDED DEFAULT: " << cur << '\n';
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
typename List<T, Allocator>::iterator List<T, Allocator>::erase(
    List::const_iterator iter) {
  BaseNode* node_to_delete = iter.get_node();
  BaseNode* prev = node_to_delete->prev;
  BaseNode* next = node_to_delete->next;
  prev->next = next;
  next->prev = prev;
  //std::cout << "DELETED: " << node_to_delete << '\n';
  AllocTraits::destroy(allocator_, reinterpret_cast<Node*>(node_to_delete));
  AllocTraits::deallocate(allocator_, reinterpret_cast<Node*>(node_to_delete), 1);
  --size_;
  if (iter == begin_) {
    begin_ = iterator(next);
  }
  return iterator(next);
}

template<typename T, typename Allocator>
typename List<T, Allocator>::iterator List<T, Allocator>::begin() {
  return begin_;
}

template<typename T, typename Allocator>
typename List<T, Allocator>::const_iterator List<T, Allocator>::begin() const {
  return cbegin();
}

template<typename T, typename Allocator>
typename List<T, Allocator>::const_iterator List<T, Allocator>::cbegin() const {
  return static_cast<const_iterator>(begin_);
}

template<typename T, typename Allocator>
typename List<T, Allocator>::iterator List<T, Allocator>::end() {
  return end_;
}

template<typename T, typename Allocator>
typename List<T, Allocator>::const_iterator List<T, Allocator>::end() const {
  return cend();
}

template<typename T, typename Allocator>
typename List<T, Allocator>::const_iterator List<T, Allocator>::cend() const {
  return static_cast<const_iterator>(end_);
}

template<typename T, typename Allocator>
typename List<T, Allocator>::reverse_iterator List<T, Allocator>::rbegin() {
  return reverse_iterator(end_);
}

template<typename T, typename Allocator>
typename List<T, Allocator>::const_reverse_iterator List<T, Allocator>::rbegin() const {
  return crbegin();
}

template<typename T, typename Allocator>
typename List<T, Allocator>::const_reverse_iterator List<T, Allocator>::crbegin() const {
  return const_reverse_iterator(end_);
}

template<typename T, typename Allocator>
typename List<T, Allocator>::reverse_iterator List<T, Allocator>::rend() {
  return reverse_iterator(begin_);
}

template<typename T, typename Allocator>
typename List<T, Allocator>::const_reverse_iterator List<T, Allocator>::rend() const {
  return crend();
}

template<typename T, typename Allocator>
typename List<T, Allocator>::const_reverse_iterator List<T, Allocator>::crend() const {
  return const_reverse_iterator(begin_);
}

template<typename T, typename Allocator>
template<bool is_const>
class List<T, Allocator>::CommonIterator {
 private:
  BaseNode* node_;

 public:
  CommonIterator();
  CommonIterator(BaseNode*);

  using value_type = T;
  using iterator_category = std::bidirectional_iterator_tag;
  using difference_type = ssize_t;
  using reference = typename std::conditional<is_const, const T&, T&>::type;
  using pointer = typename std::conditional<is_const, const T*, T*>::type;

  operator CommonIterator<true>() const;

  CommonIterator<is_const> operator--(int);
  CommonIterator<is_const> operator++(int);
  CommonIterator<is_const>& operator--();
  CommonIterator<is_const>& operator++();

  reference operator*() const;
  pointer operator->() const;

  bool operator==(const CommonIterator<is_const>&) const;
  bool operator!=(const CommonIterator<is_const>&) const;

  BaseNode* get_node() const;
};

template<typename T, typename Allocator>
template<bool is_const>
List<T, Allocator>::CommonIterator<is_const>::CommonIterator() = default;

template<typename T, typename Allocator>
template<bool is_const>
List<T, Allocator>::CommonIterator<is_const>::CommonIterator(BaseNode* node)
    : node_(node) {}

template<typename T, typename Allocator>
template<bool is_const>
List<T, Allocator>::CommonIterator<is_const>::operator CommonIterator<true>() const {
  return CommonIterator<true>(node_);
}

template<typename T, typename Allocator>
template<bool is_const>
typename List<T, Allocator>::template CommonIterator<is_const>&
List<T, Allocator>::CommonIterator<is_const>::operator--() {
  node_ = node_->prev;
  return *this;
}

template<typename T, typename Allocator>
template<bool is_const>
typename List<T, Allocator>::template CommonIterator<is_const>&
List<T, Allocator>::CommonIterator<is_const>::operator++() {
  node_ = node_->next;
  return *this;
}

template<typename T, typename Allocator>
template<bool is_const>
typename List<T, Allocator>::template CommonIterator<is_const>
List<T, Allocator>::CommonIterator<is_const>::operator--(int) {
  CommonIterator<is_const> tmp(*this);
  --(*this);
  return tmp;
}

template<typename T, typename Allocator>
template<bool is_const>
typename List<T, Allocator>::template CommonIterator<is_const>
List<T, Allocator>::CommonIterator<is_const>::operator++(int) {
  CommonIterator<is_const> tmp(*this);
  ++(*this);
  return tmp;
}

template<typename T, typename Allocator>
template<bool is_const>
typename List<T, Allocator>::template CommonIterator<is_const>::reference
List<T, Allocator>::CommonIterator<is_const>::operator*() const {
  return reinterpret_cast<Node*>(node_)->value;
}

template<typename T, typename Allocator>
template<bool is_const>
typename List<T, Allocator>::template CommonIterator<is_const>::pointer
List<T, Allocator>::CommonIterator<is_const>::operator->() const {
  return &(operator*());
};

template<typename T, typename Allocator>
template<bool is_const>
bool List<T, Allocator>::CommonIterator<is_const>::operator==(
    const List<T, Allocator>::CommonIterator<is_const>& other_iter) const {
  return (node_ == other_iter.node_);
}

template<typename T, typename Allocator>
template<bool is_const>
bool List<T, Allocator>::CommonIterator<is_const>::operator!=(
    const List<T, Allocator>::CommonIterator<is_const>& other_iter) const {
  return !(*this == other_iter);
}

template<typename T, typename Allocator>
template<bool is_const>
typename List<T, Allocator>::BaseNode*
List<T, Allocator>::CommonIterator<is_const>::get_node() const {
  return reinterpret_cast<BaseNode*>(node_);
}