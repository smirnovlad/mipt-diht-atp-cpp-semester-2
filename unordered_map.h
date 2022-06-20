#include <iostream>
#include <vector>

template<typename T, typename Allocator = std::allocator<T>>
class List {
 private:
  template<bool is_const>
  class CommonIterator;

 public:
  using iterator = CommonIterator<false>;
  using const_iterator = CommonIterator<true>;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

 private:
  struct BaseNode {
    BaseNode* prev;
    BaseNode* next;

    BaseNode() : prev(this), next(this) {}

    BaseNode(BaseNode* prev, BaseNode* next) : prev(prev), next(next) {}
  };

  BaseNode* ptr_on_fake_node_ = nullptr;

  struct Node : BaseNode {
    T* ptr_on_value;
    Node() = default;

    Node(T* ptr_on_value) : ptr_on_value(ptr_on_value) {}
  };

  using BaseNodeAlloc = typename Allocator::template rebind<BaseNode>::other;
  BaseNodeAlloc base_node_allocator_;
  using NodeAlloc = typename Allocator::template rebind<Node>::other;
  NodeAlloc node_allocator_;

  using AllocTraits = std::allocator_traits<Allocator>;
  using NodeTraits = std::allocator_traits<NodeAlloc>;
  using BaseNodeTraits = std::allocator_traits<BaseNodeAlloc>;

  size_t size_ = 0;

  void add_elements(const List&);

  template<typename... Args>
  iterator emplace(const_iterator, Args&& ...);

  void swap_alloc(List<T, Allocator>&) noexcept;

  void move_alloc(List<T, Allocator>&) noexcept;
  void move_data(List<T, Allocator>&) noexcept;

 public:
  void swap_data(List<T, Allocator>&) noexcept;

  void delete_elements() noexcept;
  void delete_node(const iterator&) noexcept;
  void clear() noexcept;

  List(const Allocator& alloc = Allocator()) noexcept;
  List(size_t, const Allocator& alloc = Allocator());
  List(size_t, const T&, const Allocator& alloc = Allocator());
  List(const List&);
  List(List&&) noexcept;
  ~List() noexcept;
  List& operator=(const List&);
  List& operator=(List&&) noexcept;

  Allocator get_allocator() const;

  size_t size() const noexcept;

  iterator emplace(const_iterator, T*);

  template<typename... Args>
  iterator insert(const_iterator, const T&);
  template<typename... Args>
  iterator insert(const_iterator, T&&);
  iterator erase(const_iterator) noexcept;

  void push_back(const T&);
  void push_back(T&&);
  void push_back(T*);
  void push_front(const T&);
  void push_front(T&&);
  void push_front(T*);
  void pop_back() noexcept;
  void pop_front() noexcept;

  iterator begin() noexcept;
  const_iterator begin() const noexcept;
  const_iterator cbegin() const noexcept;
  iterator end() noexcept;
  const_iterator end() const noexcept;
  const_iterator cend() const noexcept;
};

template<typename T, typename Allocator>
List<T, Allocator>::List(const Allocator& alloc) noexcept:
    base_node_allocator_(alloc), node_allocator_(alloc) {
  ptr_on_fake_node_ = BaseNodeTraits::allocate(base_node_allocator_, 1);
  try {
    BaseNodeTraits::construct(base_node_allocator_,
                              ptr_on_fake_node_, ptr_on_fake_node_, ptr_on_fake_node_);
  } catch (...) {
    BaseNodeTraits::deallocate(base_node_allocator_, ptr_on_fake_node_, 1);
  }
}

template<typename T, typename Allocator>
List<T, Allocator>::List(size_t size, const Allocator& alloc) : List<T, Allocator>(alloc) {
  try {
    while (size-- > 0) {
      emplace(end());
    }
  } catch (...) {
    clear();
    throw;
  }
}

template<typename T, typename Allocator>
List<T, Allocator>::List(size_t size, const T& value, const Allocator& alloc)
    : List<T, Allocator>(alloc) {
  while (size-- > 0) {
    try {
      push_back(value);
    } catch (...) {
      clear();
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
      delete_elements();
      throw;
    }
  }
}

template<typename T, typename Allocator>
void List<T, Allocator>::delete_elements() noexcept {
  while (size_ > 0) {
    pop_back();
  }
}

template<typename T, typename Allocator>
void List<T, Allocator>::delete_node(const iterator& iter) noexcept {
  BaseNode* node_to_delete = iter.get_node();
  BaseNode* prev = node_to_delete->prev;
  BaseNode* next = node_to_delete->next;
  prev->next = next;
  next->prev = prev;

  NodeTraits::destroy(node_allocator_, reinterpret_cast<Node*>(node_to_delete));
  NodeTraits::deallocate(node_allocator_, reinterpret_cast<Node*>(node_to_delete), 1);
  --size_;
}

template<typename T, typename Allocator>
List<T, Allocator>::List(const List& other_list) :
    List(NodeTraits::select_on_container_copy_construction(other_list.get_allocator())) {
  add_elements(other_list);
}

template<typename T, typename Allocator>
List<T, Allocator>::List(List&& other_list) noexcept:
    List(std::move(NodeTraits::select_on_container_copy_construction(other_list.get_allocator()))) {
  swap_data(other_list);
}

template<typename T, typename Allocator>
void List<T, Allocator>::clear() noexcept {
  delete_elements();
  BaseNodeTraits::destroy(base_node_allocator_, ptr_on_fake_node_);
  BaseNodeTraits::deallocate(base_node_allocator_, ptr_on_fake_node_, 1);
}

template<typename T, typename Allocator>
List<T, Allocator>::~List() noexcept {
  clear();
}

template<typename T, typename Allocator>
void List<T, Allocator>::move_alloc(List<T, Allocator>& other_list) noexcept {
  node_allocator_ = std::move(NodeTraits::propagate_on_container_copy_assignment::value ?
                              other_list.node_allocator_ : node_allocator_);
  base_node_allocator_ = std::move(BaseNodeTraits::propagate_on_container_copy_assignment::value ?
                                   other_list.base_node_allocator_ : base_node_allocator_);
}

template<typename T, typename Allocator>
void List<T, Allocator>::move_data(List<T, Allocator>& other_list) noexcept {
  ptr_on_fake_node_ = std::move(other_list.ptr_on_fake_node_);
  size_ = other_list.size_;
}

template<typename T, typename Allocator>
void List<T, Allocator>::swap_alloc(List<T, Allocator>& other_list) noexcept {
  std::swap(base_node_allocator_, other_list.base_node_allocator_);
  std::swap(node_allocator_, other_list.node_allocator_);
}

template<typename T, typename Allocator>
void List<T, Allocator>::swap_data(List<T, Allocator>& other_list) noexcept {
  std::swap(ptr_on_fake_node_, other_list.ptr_on_fake_node_);
  std::swap(size_, other_list.size_);
}

template<typename T, typename Allocator>
List<T, Allocator>& List<T, Allocator>::operator=(const List& other_list) {
  List<T, Allocator> tmp_list(
      NodeTraits::propagate_on_container_copy_assignment::value ? other_list.node_allocator_ : node_allocator_);
  tmp_list.add_elements(other_list);
  swap_alloc(tmp_list);
  swap_data(tmp_list);
  return *this;
}

template<typename T, typename Allocator>
List<T, Allocator>& List<T, Allocator>::operator=(List&& other_list) noexcept {
  if (this != &other_list) {
    move_alloc(other_list);
    swap_data(other_list);
  }
  return *this;
}

template<typename T, typename Allocator>
Allocator List<T, Allocator>::get_allocator() const {
  return Allocator(node_allocator_);
}

template<typename T, typename Allocator>
size_t List<T, Allocator>::size() const noexcept {
  return size_;
}

template<typename T, typename Allocator>
template<typename... Args>
typename List<T, Allocator>::iterator List<T, Allocator>::emplace(List::const_iterator iter, Args&& ... args) {
  auto alloc = Allocator(node_allocator_);
  T* ptr_on_value = AllocTraits::allocate(alloc, 1);
  try {
    AllocTraits::construct(alloc, ptr_on_value, std::forward<Args>(args)...);
  } catch (...) {
    AllocTraits::deallocate(alloc, ptr_on_value, 1);
    throw;
  }

  Node* ptr = NodeTraits::allocate(node_allocator_, 1);
  try {
    NodeTraits::construct(node_allocator_, ptr, ptr_on_value);
  } catch (...) {
    NodeTraits::deallocate(node_allocator_, ptr, 1);
    throw;
  }
  ++size_;
  auto cur = reinterpret_cast<BaseNode*>(ptr);
  auto prev = (iter.get_node())->prev, next = reinterpret_cast<BaseNode*>(iter.get_node());
  cur->prev = prev;
  cur->next = next;
  prev->next = cur;
  next->prev = cur;

  return iterator(cur);
}

template<typename T, typename Allocator>
typename List<T, Allocator>::iterator List<T, Allocator>::emplace(
    List::const_iterator iter, T* ptr_on_value) {
  Node* ptr = NodeTraits::allocate(node_allocator_, 1);
  try {
    NodeTraits::construct(node_allocator_, ptr, ptr_on_value);
  } catch (...) {
    NodeTraits::deallocate(node_allocator_, ptr, 1);
    throw;
  }
  ++size_;
  auto cur = reinterpret_cast<BaseNode*>(ptr);
  auto prev = (iter.get_node())->prev, next = reinterpret_cast<BaseNode*>(iter.get_node());
  cur->prev = prev;
  cur->next = next;
  prev->next = cur;
  next->prev = cur;
  return iterator(cur);
}

template<typename T, typename Allocator>
void List<T, Allocator>::push_back(const T& value) {
  emplace(end(), value);
}

template<typename T, typename Allocator>
void List<T, Allocator>::push_back(T&& value) {
  emplace(end(), std::move(value));
}

template<typename T, typename Allocator>
void List<T, Allocator>::push_back(T* ptr_on_value) {
  emplace(end(), ptr_on_value);
}

template<typename T, typename Allocator>
void List<T, Allocator>::push_front(const T& value) {
  emplace(begin(), value);
}

template<typename T, typename Allocator>
void List<T, Allocator>::push_front(T&& value) {
  emplace(begin(), std::move(value));
}

template<typename T, typename Allocator>
void List<T, Allocator>::push_front(T* ptr_on_value) {
  emplace(begin(), ptr_on_value);
}

template<typename T, typename Allocator>
void List<T, Allocator>::pop_back() noexcept {
  erase(std::prev(end()));
}

template<typename T, typename Allocator>
void List<T, Allocator>::pop_front() noexcept {
  erase(begin());
}

template<typename T, typename Allocator>
template<typename... Args>
typename List<T, Allocator>::iterator List<T, Allocator>::insert(
    List::const_iterator iter, const T& value) {
  return emplace(iter, value);
}

template<typename T, typename Allocator>
template<typename... Args>
typename List<T, Allocator>::iterator List<T, Allocator>::insert(
    List::const_iterator iter, T&& value) {
  return emplace(iter, std::move(value));
}

template<typename T, typename Allocator>
typename List<T, Allocator>::iterator List<T, Allocator>::erase(
    List::const_iterator iter) noexcept {
  BaseNode* node_to_delete = iter.get_node();
  BaseNode* prev = node_to_delete->prev;
  BaseNode* next = node_to_delete->next;
  prev->next = next;
  next->prev = prev;

  auto alloc = Allocator(node_allocator_);
  AllocTraits::destroy(alloc, iterator(iter.get_node()).operator->());
  AllocTraits::deallocate(alloc, iterator(iter.get_node()).operator->(), 1);

  NodeTraits::destroy(node_allocator_, reinterpret_cast<Node*>(node_to_delete));
  NodeTraits::deallocate(node_allocator_, reinterpret_cast<Node*>(node_to_delete), 1);

  --size_;
  return iterator(next);
}

template<typename T, typename Allocator>
typename List<T, Allocator>::iterator List<T, Allocator>::begin() noexcept {
  return iterator(ptr_on_fake_node_->next);
}

template<typename T, typename Allocator>
typename List<T, Allocator>::const_iterator List<T, Allocator>::begin() const noexcept {
  return const_iterator(ptr_on_fake_node_->next);
}

template<typename T, typename Allocator>
typename List<T, Allocator>::const_iterator List<T, Allocator>::cbegin() const noexcept {
  return const_iterator(ptr_on_fake_node_->next);
}

template<typename T, typename Allocator>
typename List<T, Allocator>::iterator List<T, Allocator>::end() noexcept {
  return iterator(ptr_on_fake_node_);
}

template<typename T, typename Allocator>
typename List<T, Allocator>::const_iterator List<T, Allocator>::end() const noexcept {
  return const_iterator(ptr_on_fake_node_);
}

template<typename T, typename Allocator>
typename List<T, Allocator>::const_iterator List<T, Allocator>::cend() const noexcept {
  return const_iterator(ptr_on_fake_node_);
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

  CommonIterator<is_const>& operator+=(ssize_t) noexcept;
  CommonIterator<is_const> operator+(ssize_t) const noexcept;

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
typename List<T, Allocator>::template CommonIterator<is_const>&
List<T, Allocator>::CommonIterator<is_const>::operator+=(ssize_t val) noexcept {
  while (val-- > 0) {
    ++(*this);
  }
  return *this;
}

template<typename T, typename Allocator>
template<bool is_const>
typename List<T, Allocator>::template CommonIterator<is_const>
List<T, Allocator>::CommonIterator<is_const>::operator+(ssize_t val) const noexcept {
  CommonIterator temp_iterator(*this);
  temp_iterator += val;
  return temp_iterator;
}

template<typename T, typename Allocator>
template<bool is_const>
typename List<T, Allocator>::template CommonIterator<is_const>::reference
List<T, Allocator>::CommonIterator<is_const>::operator*() const noexcept {
  return *reinterpret_cast<Node*>(node_)->ptr_on_value;
}

template<typename T, typename Allocator>
template<bool is_const>
typename List<T, Allocator>::template CommonIterator<is_const>::pointer
List<T, Allocator>::CommonIterator<is_const>::operator->() const noexcept {
  return reinterpret_cast<Node*>(node_)->ptr_on_value;
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

template<typename Key, typename Value, typename Hash = std::hash<Key>,
    typename Equal = std::equal_to<Key>,
    typename Alloc = std::allocator<std::pair<const Key, Value>>>
class UnorderedMap {
 public:
  using AllocTraits = std::allocator_traits<Alloc>;
  using node_type = std::pair<const Key, Value>;
  using iterator = typename List<node_type, Alloc>::iterator;
  using const_iterator = typename List<node_type, Alloc>::const_iterator;

 private:
  static const size_t DEFAULT_TABLE_SIZE_ = 64;
  float max_load_factor_ = 0.66;

  Alloc allocator_ = Alloc();
  using bucket_bounds = std::pair<iterator, iterator>;
  using hash_table_type = std::vector<bucket_bounds,
      typename AllocTraits::template rebind_alloc<bucket_bounds>>;
  List<node_type, Alloc> list_ = List<node_type, Alloc>(allocator_);
  hash_table_type hash_table_ = hash_table_type(
      DEFAULT_TABLE_SIZE_, {end(), end()}, allocator_);
  Equal key_equal_ = Equal();
  Hash hasher_ = Hash();

  void delete_elements() noexcept;

  void swap_alloc(UnorderedMap&) noexcept;
  void swap_data(UnorderedMap&) noexcept;

  void move_data(UnorderedMap&) noexcept;

  size_t get_hash(const Key&) const noexcept;
  void rehash(size_t);
  void add_elements(const UnorderedMap&);

  iterator find(const Key&, size_t) noexcept;
  iterator find(Key&&, size_t) noexcept;

 public:
  UnorderedMap();
  UnorderedMap(size_t, const Alloc&);
  UnorderedMap(size_t, const Hash& hasher,
               const Equal& key_equal, const Alloc& alloc);
  UnorderedMap(const UnorderedMap&);
  UnorderedMap& operator=(const UnorderedMap&);
  UnorderedMap(UnorderedMap&&) noexcept;
  UnorderedMap& operator=(UnorderedMap&&) noexcept;
  ~UnorderedMap() noexcept;

  size_t size() const noexcept;

  iterator find(const Key&) noexcept;
  const_iterator find(const Key&) const noexcept;
  iterator find(Key&&) noexcept;
  const_iterator find(Key&&) const noexcept;

  template<typename... Args>
  std::pair<iterator, bool> emplace(Args&& ...);

  std::pair<iterator, bool> insert(const node_type&);
  std::pair<iterator, bool> insert(node_type&&);
  template<typename InputIterator>
  void insert(const InputIterator&, const InputIterator&);

  template<typename P>
  std::pair<iterator, bool> insert(P&& value) {
    return emplace(std::forward<P>(value));
  }

  void erase(const iterator&) noexcept;
  template<typename InputIterator>
  void erase(InputIterator, InputIterator) noexcept;

  Value& operator[](const Key&) noexcept;
  Value& at(const Key&);
  const Value& at(const Key&) const;
  Value& at(Key&&);
  const Value& at(Key&&) const;

  void reserve(size_t);
  size_t max_size() const noexcept;
  float max_load_factor() const noexcept;
  void max_load_factor(float);
  float load_factor() const noexcept;
  float load_factor(size_t) const noexcept;
  float load_factor(size_t, size_t) const noexcept;

  iterator begin() noexcept;
  const_iterator begin() const noexcept;
  const_iterator cbegin() const noexcept;
  iterator end() noexcept;
  const_iterator end() const noexcept;
  const_iterator cend() const noexcept;
};

template<typename Key, typename Value, typename Hash, typename Equal, typename Alloc>
void UnorderedMap<Key, Value, Hash, Equal, Alloc>::delete_elements() noexcept {
  list_.delete_elements();
}

template<typename Key, typename Value, typename Hash, typename Equal, typename Alloc>
void UnorderedMap<Key, Value, Hash, Equal, Alloc>::swap_alloc(UnorderedMap& other_map) noexcept {
  std::swap(allocator_, other_map.allocator_);
}

template<typename Key, typename Value, typename Hash, typename Equal, typename Alloc>
void UnorderedMap<Key, Value, Hash, Equal, Alloc>::swap_data(UnorderedMap& other_map) noexcept {
  std::swap(hash_table_, other_map.hash_table_);
  list_.swap_data(other_map.list_);
  std::swap(hasher_, other_map.hasher_);
  std::swap(key_equal_, other_map.key_equal_);
  std::swap(max_load_factor_, other_map.max_load_factor_);
}

template<typename Key, typename Value, typename Hash, typename Equal, typename Alloc>
void UnorderedMap<Key, Value, Hash, Equal, Alloc>::move_data(UnorderedMap& other_map) noexcept {
  max_load_factor_ = other_map.max_load_factor_;
  list_ = std::move(other_map.list_);
  hash_table_ = std::move(other_map.hash_table_);
  key_equal_ = std::move(other_map.key_equal_);
  hasher_ = std::move(other_map.hasher_);
}

template<typename Key, typename Value, typename Hash, typename Equal, typename Alloc>
void UnorderedMap<Key, Value, Hash, Equal, Alloc>::add_elements(
    const UnorderedMap& other_map) {
  for (auto it = other_map.begin(); it != other_map.end(); ++it) {
    try {
      insert(*it);
    } catch (...) {
      while (size() > 0) {
        erase(begin());
      }
      throw;
    }
  }
}

template<typename Key, typename Value, typename Hash, typename Equal, typename Alloc>
UnorderedMap<Key, Value, Hash, Equal, Alloc>::UnorderedMap() = default;

template<typename Key, typename Value, typename Hash, typename Equal, typename Alloc>
UnorderedMap<Key, Value, Hash, Equal, Alloc>::UnorderedMap(
    size_t bucket_count, const Alloc& alloc) :
    UnorderedMap(bucket_count, Hash(), Equal(), alloc) {}

template<typename Key, typename Value, typename Hash, typename Equal, typename Alloc>
UnorderedMap<Key, Value, Hash, Equal, Alloc>::UnorderedMap(
    size_t bucket_count, const Hash& hasher,
    const Equal& key_equal, const Alloc& alloc) : allocator_(alloc), hash_table_(bucket_count, {end(), end()}, alloc),
                                                  key_equal_(key_equal), hasher_(hasher) {}

template<typename Key, typename Value, typename Hash, typename Equal, typename Alloc>
UnorderedMap<Key, Value, Hash, Equal, Alloc>::UnorderedMap(const UnorderedMap& other_map) :
    UnorderedMap(DEFAULT_TABLE_SIZE_, AllocTraits::select_on_container_copy_construction(other_map.allocator_)) {
  insert(other_map.begin(), other_map.end());
}

template<typename Key, typename Value, typename Hash, typename Equal, typename Alloc>
UnorderedMap<Key, Value, Hash, Equal, Alloc>&
UnorderedMap<Key, Value, Hash, Equal, Alloc>::operator=(const UnorderedMap& other_map) {
  UnorderedMap<Key, Value, Hash, Equal, Alloc> tmp_map(
      AllocTraits::propagate_on_container_copy_assignment::value ?
      other_map.allocator_ : allocator_);
  tmp_map.add_elements(other_map);
  swap_alloc(tmp_map);
  swap_data(tmp_map);
  return *this;
}

template<typename Key, typename Value, typename Hash, typename Equal, typename Alloc>
UnorderedMap<Key, Value, Hash, Equal, Alloc>::UnorderedMap(UnorderedMap&& other_map) noexcept:
    max_load_factor_(other_map.max_load_factor_),
    allocator_(std::move(AllocTraits::select_on_container_copy_construction(other_map.allocator_))),
    list_(std::move(other_map.list_)), hash_table_(std::move(other_map.hash_table_)),
    key_equal_(std::move(other_map.key_equal_)), hasher_(std::move(other_map.hasher_)) {}

template<typename Key, typename Value, typename Hash, typename Equal, typename Alloc>
UnorderedMap<Key, Value, Hash, Equal, Alloc>&
UnorderedMap<Key, Value, Hash, Equal, Alloc>::operator=(UnorderedMap&& other_map) noexcept {
  if (this != &other_map) {
    delete_elements();
    allocator_ = std::move(AllocTraits::propagate_on_container_copy_assignment::value ?
                           other_map.allocator_ : allocator_);
    move_data(other_map);
  }
  return *this;
}

template<typename Key, typename Value, typename Hash, typename Equal, typename Alloc>
UnorderedMap<Key, Value, Hash, Equal, Alloc>::~UnorderedMap() noexcept = default;

template<typename Key, typename Value, typename Hash, typename Equal, typename Alloc>
size_t UnorderedMap<Key, Value, Hash, Equal, Alloc>::get_hash(const Key& key) const noexcept {
  return hasher_(key) % hash_table_.size();
}

template<typename Key, typename Value, typename Hash, typename Equal, typename Alloc>
void UnorderedMap<Key, Value, Hash, Equal, Alloc>::rehash(size_t bucket_count) {
  auto new_list = List<node_type, Alloc>(allocator_);
  hash_table_ = hash_table_type(bucket_count, {new_list.end(), new_list.end()}, allocator_);
  while (begin() != end()) {
    auto it = begin();
    const Key& key = it->first;
    size_t hash = get_hash(key);
    if (hash_table_[hash].first == hash_table_[hash].second) {
      new_list.push_front(&(*it));
      hash_table_[hash] = {new_list.begin(), new_list.begin() + 1};
    } else {
      new_list.emplace(hash_table_[hash].second, &(*it));
      ++hash_table_[hash].second;
    }
    list_.delete_node(it);
  }
  list_ = std::move(new_list);
}

template<typename Key, typename Value, typename Hash, typename Equal, typename Alloc>
typename UnorderedMap<Key, Value, Hash, Equal, Alloc>::iterator
UnorderedMap<Key, Value, Hash, Equal, Alloc>::find(
    const Key& key, size_t key_hash) noexcept {
  for (auto it = hash_table_[key_hash].first; it != hash_table_[key_hash].second && it != end(); ++it) {
    if (key_equal_(key, it->first)) {
      return it;
    }
  }
  return end();
}

template<typename Key, typename Value, typename Hash, typename Equal, typename Alloc>
typename UnorderedMap<Key, Value, Hash, Equal, Alloc>::iterator
UnorderedMap<Key, Value, Hash, Equal, Alloc>::find(const Key& key) noexcept {
  return find(key, get_hash(key));
}

template<typename Key, typename Value, typename Hash, typename Equal, typename Alloc>
typename UnorderedMap<Key, Value, Hash, Equal, Alloc>::const_iterator
UnorderedMap<Key, Value, Hash, Equal, Alloc>::find(const Key& key) const noexcept {
  return static_cast<const_iterator>(find(key, get_hash(key)));
}

template<typename Key, typename Value, typename Hash, typename Equal, typename Alloc>
typename UnorderedMap<Key, Value, Hash, Equal, Alloc>::iterator
UnorderedMap<Key, Value, Hash, Equal, Alloc>::find(
    Key&& key, size_t key_hash) noexcept {
  for (auto it = hash_table_[key_hash].first; it != hash_table_[key_hash].second; ++it) {
    if (key_equal_(it->first, std::move(key))) {
      return it;
    }
  }
  return end();
}

template<typename Key, typename Value, typename Hash, typename Equal, typename Alloc>
typename UnorderedMap<Key, Value, Hash, Equal, Alloc>::iterator
UnorderedMap<Key, Value, Hash, Equal, Alloc>::find(Key&& key) noexcept {
  return find(std::move(key), get_hash(key));
}

template<typename Key, typename Value, typename Hash, typename Equal, typename Alloc>
typename UnorderedMap<Key, Value, Hash, Equal, Alloc>::const_iterator
UnorderedMap<Key, Value, Hash, Equal, Alloc>::find(Key&& key) const noexcept {
  return static_cast<const_iterator>(find(std::move(key), get_hash(key)));
}

template<typename Key, typename Value, typename Hash, typename Equal, typename Alloc>
Value& UnorderedMap<Key, Value, Hash, Equal, Alloc>::operator[](const Key& key) noexcept {
  auto pos = find(key);
  if (pos == end()) {
    pos = insert({key, Value()}).first;
  }
  return pos->second;
}

template<typename Key, typename Value, typename Hash, typename Equal, typename Alloc>
Value& UnorderedMap<Key, Value, Hash, Equal, Alloc>::at(const Key& key) {
  auto pos = find(key);
  if (pos == end()) {
    throw std::range_error("key does not exist");
  } else {
    return pos->second;
  }
}

template<typename Key, typename Value, typename Hash, typename Equal, typename Alloc>
const Value& UnorderedMap<Key, Value, Hash, Equal, Alloc>::at(const Key& key) const {
  return at(key);
}

template<typename Key, typename Value, typename Hash, typename Equal, typename Alloc>
Value& UnorderedMap<Key, Value, Hash, Equal, Alloc>::at(Key&& key) {
  auto pos = find(std::move(key));
  if (pos == end()) {
    throw std::range_error("key does not exist");
  } else {
    return pos->second;
  }
}

template<typename Key, typename Value, typename Hash, typename Equal, typename Alloc>
const Value& UnorderedMap<Key, Value, Hash, Equal, Alloc>::at(Key&& key) const {
  return at(std::move(key));
}

template<typename Key, typename Value, typename Hash, typename Equal, typename Alloc>
size_t UnorderedMap<Key, Value, Hash, Equal, Alloc>::size() const noexcept {
  return list_.size();
}

template<typename Key, typename Value, typename Hash, typename Equal, typename Alloc>
void UnorderedMap<Key, Value, Hash, Equal, Alloc>::reserve(size_t count) {
  size_t new_hash_table_size = hash_table_.size();
  while (load_factor(count, new_hash_table_size) > max_load_factor()) {
    new_hash_table_size *= 2;
  }
  if (hash_table_.size() != new_hash_table_size) {
    rehash(new_hash_table_size);
  }
}

template<typename Key, typename Value, typename Hash, typename Equal, typename Alloc>
template<typename... Args>
std::pair<typename UnorderedMap<Key, Value, Hash, Equal, Alloc>::iterator, bool>
UnorderedMap<Key, Value, Hash, Equal, Alloc>::emplace(Args&& ... args) {
  node_type* ptr = AllocTraits::allocate(allocator_, 1);
  try {
    AllocTraits::construct(allocator_, ptr, std::forward<Args>(args)...);
  } catch (...) {
    AllocTraits::deallocate(allocator_, ptr, 1);
    throw;
  }
  const Key& key = ptr->first;
  size_t hash = get_hash(key);
  iterator pos = find(key);
  std::pair<iterator, bool> ans(pos, false);
  if (pos == end()) {
    if (load_factor(size() + 1, hash_table_.size()) > max_load_factor_) {
      try {
        rehash(2 * hash_table_.size());
      } catch (...) {
        AllocTraits::destroy(allocator_, ptr);
        AllocTraits::deallocate(allocator_, ptr, 1);
      }
    }
    auto& bucket_begin = hash_table_[hash].first, bucket_end = hash_table_[hash].second;
    try {
      if (bucket_begin == bucket_end) {
        list_.push_front(ptr);
        bucket_begin = begin(), bucket_end = begin() + 1;
        ans = {bucket_begin, true};
      } else {
        list_.emplace(bucket_end, ptr);
        ans = {bucket_end++, true};
      }
    } catch (...) {
      AllocTraits::destroy(allocator_, ptr);
      AllocTraits::deallocate(allocator_, ptr, 1);
    }
  } else {
    AllocTraits::destroy(allocator_, ptr);
    AllocTraits::deallocate(allocator_, ptr, 1);
  }
  return ans;
}

template<typename Key, typename Value, typename Hash, typename Equal, typename Alloc>
std::pair<typename UnorderedMap<Key, Value, Hash, Equal, Alloc>::iterator, bool>
UnorderedMap<Key, Value, Hash, Equal, Alloc>::insert(const node_type& kv) {
  return emplace(kv);
}

template<typename Key, typename Value, typename Hash, typename Equal, typename Alloc>
std::pair<typename UnorderedMap<Key, Value, Hash, Equal, Alloc>::iterator, bool>
UnorderedMap<Key, Value, Hash, Equal, Alloc>::insert(node_type&& kv) {
  return emplace(std::move(kv));
}

template<typename Key, typename Value, typename Hash, typename Equal, typename Alloc>
template<typename InputIterator>
void UnorderedMap<Key, Value, Hash, Equal, Alloc>::insert(
    const InputIterator& left_bound, const InputIterator& right_bound) {
  reserve(size() + std::distance(left_bound, right_bound));
  for (auto it = left_bound; it != right_bound; ++it) {
    insert(*it);
  }
}

template<typename Key, typename Value, typename Hash, typename Equal, typename Alloc>
void UnorderedMap<Key, Value, Hash, Equal, Alloc>::erase(const iterator& it) noexcept {
  size_t hash = get_hash(it->first);
  if (hash_table_[hash].first + 1 == hash_table_[hash].second) {
    hash_table_[hash] = {end(), end()};
  } else if (it + 1 == hash_table_[hash].second) {
    --hash_table_[hash].second;
  } else if (it == hash_table_[hash].first) {
    ++hash_table_[hash].first;
  }
  list_.erase(it);
}

template<typename Key, typename Value, typename Hash, typename Equal, typename Alloc>
template<typename InputIterator>
void UnorderedMap<Key, Value, Hash, Equal, Alloc>::erase(
    InputIterator left_bound, InputIterator right_bound) noexcept {
  auto it = left_bound;
  while (left_bound != right_bound) {
    ++left_bound;
    erase(it);
    it = left_bound;
  }
}

template<typename Key, typename Value, typename Hash, typename Equal, typename Alloc>
typename UnorderedMap<Key, Value, Hash, Equal, Alloc>::iterator
UnorderedMap<Key, Value, Hash, Equal, Alloc>::begin() noexcept {
  return list_.begin();
}

template<typename Key, typename Value, typename Hash, typename Equal, typename Alloc>
typename UnorderedMap<Key, Value, Hash, Equal, Alloc>::const_iterator
UnorderedMap<Key, Value, Hash, Equal, Alloc>::begin() const noexcept {
  return list_.begin();
}

template<typename Key, typename Value, typename Hash, typename Equal, typename Alloc>
typename UnorderedMap<Key, Value, Hash, Equal, Alloc>::const_iterator
UnorderedMap<Key, Value, Hash, Equal, Alloc>::cbegin() const noexcept {
  return list_.cbegin();
}

template<typename Key, typename Value, typename Hash, typename Equal, typename Alloc>
typename UnorderedMap<Key, Value, Hash, Equal, Alloc>::iterator
UnorderedMap<Key, Value, Hash, Equal, Alloc>::end() noexcept {
  return list_.end();
}

template<typename Key, typename Value, typename Hash, typename Equal, typename Alloc>
typename UnorderedMap<Key, Value, Hash, Equal, Alloc>::const_iterator
UnorderedMap<Key, Value, Hash, Equal, Alloc>::end() const noexcept {
  return list_.cend();
}

template<typename Key, typename Value, typename Hash, typename Equal, typename Alloc>
typename UnorderedMap<Key, Value, Hash, Equal, Alloc>::const_iterator
UnorderedMap<Key, Value, Hash, Equal, Alloc>::cend() const noexcept {
  return list_.cend();
}

template<typename Key, typename Value, typename Hash, typename Equal, typename Alloc>
size_t UnorderedMap<Key, Value, Hash, Equal, Alloc>::max_size() const noexcept {
  return AllocTraits::max_size(allocator_);
}

template<typename Key, typename Value, typename Hash, typename Equal, typename Alloc>
float UnorderedMap<Key, Value, Hash, Equal, Alloc>::max_load_factor() const noexcept {
  return max_load_factor_;
}

template<typename Key, typename Value, typename Hash, typename Equal, typename Alloc>
void UnorderedMap<Key, Value, Hash, Equal, Alloc>::max_load_factor(float value) {
  max_load_factor_ = value;
}

template<typename Key, typename Value, typename Hash, typename Equal, typename Alloc>
float UnorderedMap<Key, Value, Hash, Equal, Alloc>::load_factor() const noexcept {
  return static_cast<float>(size()) / static_cast<float>(hash_table_.size());
}

template<typename Key, typename Value, typename Hash, typename Equal, typename Alloc>
float UnorderedMap<Key, Value, Hash, Equal, Alloc>::load_factor(size_t count, size_t bucket_count) const noexcept {
  return static_cast<float>(count) / static_cast<float>(bucket_count);
}
