#include <iostream>

template<typename T>
class Deque {
 private:
  T** deque_;
  size_t size_ = 0;
  size_t array_count_ = START_ARRAY_COUNT_;
  static const size_t START_ARRAY_COUNT_;
  static const size_t MAX_SIZE_;
  void swap(Deque<T>&);
  void reallocate(size_t);

  template<bool is_const>
  class CommonIterator;

  CommonIterator<false> begin_, start_, finish_;

 public:
  Deque(int);
  Deque();
  Deque(int, const T&);
  Deque(const Deque<T>&);
  ~Deque();

  Deque<T>& operator=(const Deque<T>&);

  using iterator = CommonIterator<false>;
  using const_iterator = CommonIterator<true>;

  size_t size() const;
  T& operator[](ssize_t);
  const T& operator[](ssize_t) const;
  T& at(ssize_t);
  const T& at(ssize_t) const;

  void push_front(const T&);
  void push_back(const T&);
  void pop_front();
  void pop_back();
  void insert(iterator, const T&);
  void erase(iterator);

  iterator begin();
  const_iterator begin() const;
  iterator end();
  const_iterator end() const;
  const_iterator cbegin() const;
  const_iterator cend() const;

  std::reverse_iterator<iterator> rbegin();
  std::reverse_iterator<iterator> rend();
  std::reverse_iterator<const_iterator> crbegin();
  std::reverse_iterator<const_iterator> crend();
};

template<typename T>
const size_t Deque<T>::MAX_SIZE_ = 32;

template<typename T>
const size_t Deque<T>::START_ARRAY_COUNT_ = 8;

template<typename T>
Deque<T>::Deque(int size) : size_(size), array_count_(START_ARRAY_COUNT_) {
  while (array_count_ * MAX_SIZE_ <= 2 * size_t(size)) { // <= !!!
    array_count_ *= 2;
  }
  deque_ = reinterpret_cast<T**>(new uint8_t[array_count_ * sizeof(T*)]);
  for (size_t i = 0; i < array_count_; ++i) {
    deque_[i] = reinterpret_cast<T*>(new uint8_t[MAX_SIZE_ * sizeof(T)]);
  }
  begin_ = {deque_ + (array_count_ / 2), MAX_SIZE_ - 1};
  start_ = {deque_, 0};
  finish_ = {deque_ + (array_count_ - 1), MAX_SIZE_};
}

template<typename T>
Deque<T>::Deque() : Deque<T>(0) {}

template<typename T>
Deque<T>::Deque(int size, const T& to_fill) : Deque<T>(size) {
  for (iterator it = begin(); it != end(); ++it) {
    new(it.get_array() + it.get_index()) T(to_fill);
  }
}

template<typename T>
Deque<T>::Deque(const Deque<T>& arg_deque) : Deque<T>(arg_deque.size()) {
  for (auto it = begin(), arg_it = arg_deque.begin(); it != end(); ++it, ++arg_it) {
    new(it.get_array() + it.get_index()) T(*arg_it);
  }
}

template<typename T>
Deque<T>::~Deque() {
  for (iterator it = begin(); it != end(); ++it) {
    it->~T();
  }
  for (size_t i = 0; i < array_count_; ++i) {
    delete[] reinterpret_cast<uint8_t*>(deque_[i]);
  }
  delete[] reinterpret_cast<uint8_t*>(deque_);
}

template<typename T>
void Deque<T>::reallocate(size_t new_array_count) {
  T** new_deque = reinterpret_cast<T**>(new uint8_t[new_array_count * sizeof(T*)]);
  for (size_t i = 0; i < new_array_count; ++i) {
    if (i >= array_count_ / 2 && i < array_count_ / 2 + array_count_) {
      new_deque[i] = deque_[i - array_count_ / 2];
    } else {
      new_deque[i] = reinterpret_cast<T*>(new uint8_t[MAX_SIZE_ * sizeof(T)]);
    }
  }
  iterator new_begin(new_deque + array_count_ / 2 + (begin().get_ptr() - deque_), begin().get_index());
  delete[] reinterpret_cast<uint8_t*>(deque_);
  deque_ = new_deque;
  array_count_ = new_array_count;
  begin_ = new_begin;
  start_ = {deque_, 0};
  finish_ = {deque_ + (array_count_ - 1), MAX_SIZE_};
}

template<typename T>
void Deque<T>::swap(Deque<T>& arg_deque) {
  std::swap(deque_, arg_deque.deque_);
  std::swap(size_, arg_deque.size_);
  std::swap(begin_, arg_deque.begin_);
}

template<typename T>
Deque<T>& Deque<T>::operator=(const Deque<T>& deque) {
  Deque<T> tmp_deque(deque);
  swap(tmp_deque);
  return *this;
}

template<typename T>
size_t Deque<T>::size() const {
  return size_;
}

template<typename T>
T& Deque<T>::operator[](ssize_t index) {
  return *(begin_ + index);
}

template<typename T>
const T& Deque<T>::operator[](ssize_t index) const {
  return *(begin_ + index);
}

template<typename T>
T& Deque<T>::at(ssize_t index) {
  if (index < 0 || index >= ssize_t(size_)) {
    throw std::out_of_range("out of range");
  } else {
    return this->operator[](index);
  }
}

template<typename T>
const T& Deque<T>::at(ssize_t index) const {
  if (index < 0 || index >= size_) {
    throw std::out_of_range("out of range");
  } else {
    return this->operator[](index);
  }
}

template<typename T>
void Deque<T>::push_front(const T& element) {
  //std::cout << "start.ptr = " << start_.get_ptr() << ' ' << "start.index = " << start_.get_index() << '\n';
  //std::cout << "begin.ptr = " << begin().get_ptr() << ' ' << "begin.index = " << begin().get_index() << '\n';
  if (begin() == start_) {
    //std::cout << "QQ\n";
    reallocate(2 * array_count_); // iterator's invalidation
  }
  --begin_;
  auto it = begin();
  new(it.get_array() + it.get_index()) T(element);
  ++size_;
}

template<typename T>
void Deque<T>::push_back(const T& element) {
  if (end() == finish_ - 1) {
    reallocate(2 * array_count_); // iterator's invalidation
  }
  auto it = end();
  new(it.get_array() + it.get_index()) T(element);
  ++size_;
}

template<typename T>
void Deque<T>::pop_front() {
  --size_;
  begin_->~T();
  ++begin_;
}

template<typename T>
void Deque<T>::pop_back() {
  --size_;
  (begin_ + size_)->~T();
}

template<typename T>
void Deque<T>::insert(iterator iter, const T& element) {
  if (end() == finish_) {
    reallocate(2 * array_count_); // iterator's invalidation
  }
  ++size_;
  for (auto it = end() - 1; it != iter; --it) {
    *it = *(it - 1);
  }
  *iter = element;
}

template<typename T>
void Deque<T>::erase(iterator iter) {
  for (auto it = iter; it + 1 != end(); ++it) {
    *it = *(it + 1);
  }
  --size_;
}

template<typename T>
typename Deque<T>::iterator Deque<T>::begin() {
  return begin_;
}

template<typename T>
typename Deque<T>::iterator Deque<T>::end() {
  return begin_ + size_;
}

template<typename T>
typename Deque<T>::const_iterator Deque<T>::cbegin() const {
  return const_iterator(begin_);
}

template<typename T>
typename Deque<T>::const_iterator Deque<T>::begin() const {
  return cbegin();
}

template<typename T>
typename Deque<T>::const_iterator Deque<T>::cend() const {
  return const_iterator(begin_ + size_);
}

template<typename T>
typename Deque<T>::const_iterator Deque<T>::end() const {
  return cend();
}

template<typename T>
std::reverse_iterator<typename Deque<T>::iterator> rbegin() {
  return std::reverse_iterator(Deque<T>::end());
}

template<typename T>
std::reverse_iterator<typename Deque<T>::iterator> Deque<T>::rend() {
  return std::reverse_iterator(Deque<T>::begin());
}

template<typename T>
std::reverse_iterator<typename Deque<T>::const_iterator> Deque<T>::crbegin() {
  return std::reverse_iterator(Deque<T>::cend());
}

template<typename T>
std::reverse_iterator<typename Deque<T>::const_iterator> Deque<T>::crend() {
  return std::reverse_iterator(Deque<T>::cbegin());
}

template<typename T>
template<bool is_const>
class Deque<T>::CommonIterator {
 private:
  T** ptr_;
  size_t index_;

 public:
  CommonIterator() = default;

  CommonIterator(T** ptr, size_t index) : ptr_(ptr), index_(index) {}

  using value_type = T;
  using iterator_category = std::random_access_iterator_tag;
  using difference_type = ssize_t;
  using reference = typename std::conditional<is_const, const T&, T&>::type;
  using pointer = typename std::conditional<is_const, const T*, T*>::type;

  operator CommonIterator<true>() const;

  const CommonIterator<is_const> operator--(int);
  const CommonIterator<is_const> operator++(int);
  CommonIterator<is_const>& operator--();
  CommonIterator<is_const>& operator++();
  CommonIterator<is_const>& operator+=(ssize_t);
  CommonIterator<is_const>& operator-=(ssize_t);
  CommonIterator<is_const> operator+(ssize_t) const;
  CommonIterator<is_const> operator-(ssize_t) const;

  reference operator*() const;
  pointer operator->() const;

  size_t operator-(CommonIterator<is_const>);
  bool operator<(CommonIterator<is_const>);
  bool operator==(CommonIterator<is_const>);
  bool operator>(CommonIterator<is_const>);
  bool operator<=(CommonIterator<is_const>);
  bool operator>=(CommonIterator<is_const>);
  bool operator!=(CommonIterator<is_const>);

  T* get_array() const;
  T** get_ptr() const;
  size_t get_index() const;
};

template<typename T>
template<bool is_const>
Deque<T>::CommonIterator<is_const>::operator CommonIterator<true>() const {
  return CommonIterator<true>(ptr_, index_);
}

template<typename T>
template<bool is_const>
const typename Deque<T>::template CommonIterator<is_const>
Deque<T>::CommonIterator<is_const>::operator--(int) {
  CommonIterator temp_iterator(*this);
  --(*this);
  return temp_iterator;
}

template<typename T>
template<bool is_const>
const typename Deque<T>::template CommonIterator<is_const>
Deque<T>::CommonIterator<is_const>::operator++(int) {
  CommonIterator temp_iterator(*this);
  ++(*this);
  return temp_iterator;
}

template<typename T>
template<bool is_const>
typename Deque<T>::template CommonIterator<is_const>&
Deque<T>::CommonIterator<is_const>::operator--() {
  (*this) -= 1;
  return *this;
}

template<typename T>
template<bool is_const>
typename Deque<T>::template CommonIterator<is_const>&
Deque<T>::CommonIterator<is_const>::operator++() {
  (*this) += 1;
  return *this;
}

template<typename T>
template<bool is_const>
typename Deque<T>::template CommonIterator<is_const>&
Deque<T>::CommonIterator<is_const>::operator+=(ssize_t val) {
  if (val < 0) {
    return (*this) -= (-val);
  } else {
    if (index_ + val < MAX_SIZE_) {
      index_ += val;
    } else {
      ptr_ += (val - (MAX_SIZE_ - index_)) / MAX_SIZE_ + 1;
      index_ = (val - (MAX_SIZE_ - index_)) % MAX_SIZE_;
    }
    return *this;
  }
}

template<typename T>
template<bool is_const>
typename Deque<T>::template CommonIterator<is_const>&
Deque<T>::CommonIterator<is_const>::operator-=(ssize_t val) {
  if (val < 0) {
    return (*this) += (-val);
  } else {
    if (index_ >= size_t(val)) {
      index_ -= val;
    } else {
      ptr_ -= (val - index_ - 1) / MAX_SIZE_ + 1;
      index_ = (MAX_SIZE_ - (val - index_) % MAX_SIZE_) % MAX_SIZE_;
    }
    return *this;
  }
}

template<typename T>
template<bool is_const>
typename Deque<T>::template CommonIterator<is_const>
Deque<T>::CommonIterator<is_const>::operator+(ssize_t val) const {
  CommonIterator temp_iterator(*this);
  temp_iterator += val;
  return temp_iterator;
}

template<typename T>
template<bool is_const>
typename Deque<T>::template CommonIterator<is_const>
Deque<T>::CommonIterator<is_const>::operator-(ssize_t val) const {
  return (*this) + (-val);
}

template<typename T>
template<bool is_const>
typename Deque<T>::template CommonIterator<is_const>::reference
Deque<T>::CommonIterator<is_const>::operator*() const {
  return (*ptr_)[index_];
}

template<typename T>
template<bool is_const>
typename Deque<T>::template CommonIterator<is_const>::pointer
Deque<T>::CommonIterator<is_const>::operator->() const {
  return &(operator*());
}

template<typename T>
template<bool is_const>
size_t Deque<T>::CommonIterator<is_const>::operator-(typename Deque<T>::template CommonIterator<is_const> arg_it) {
  if (*this < arg_it) {
    return -(arg_it - *this);
  } else {
    size_t level_difference = (ptr_ != arg_it.ptr_) ?
                              std::abs(arg_it.ptr_ - ptr_) - 1 : 0;
    if (ptr_ == arg_it.ptr_) {
      return index_ - arg_it.index_;
    } else {
      return Deque<T>::MAX_SIZE_ - arg_it.index_ +
             level_difference * Deque<T>::MAX_SIZE_ + index_;
    }
  }
}

template<typename T>
template<bool is_const>
bool Deque<T>::CommonIterator<is_const>::operator<(typename Deque<T>::template CommonIterator<is_const> arg_it) {
  return (ptr_ < arg_it.ptr_ ||
          (ptr_ == arg_it.ptr_ && index_ < arg_it.index_));
}

template<typename T>
template<bool is_const>
bool Deque<T>::CommonIterator<is_const>::operator==(typename Deque<T>::template CommonIterator<is_const> arg_it) {
  return (ptr_ == arg_it.ptr_ && index_ == arg_it.index_);
}

template<typename T>
template<bool is_const>
bool Deque<T>::CommonIterator<is_const>::operator>(typename Deque<T>::template CommonIterator<is_const> arg_it) {
  return !(*this < arg_it || *this == arg_it);
}

template<typename T>
template<bool is_const>
bool Deque<T>::CommonIterator<is_const>::operator<=(typename Deque<T>::template CommonIterator<is_const> arg_it) {
  return (*this < arg_it || *this == arg_it);
}

template<typename T>
template<bool is_const>
bool Deque<T>::CommonIterator<is_const>::operator>=(typename Deque<T>::template CommonIterator<is_const> arg_it) {
  return !(*this < arg_it);
}

template<typename T>
template<bool is_const>
bool Deque<T>::CommonIterator<is_const>::operator!=(typename Deque<T>::template CommonIterator<is_const> arg_it) {
  return !(*this == arg_it);
}

template<typename T>
template<bool is_const>
T* Deque<T>::CommonIterator<is_const>::get_array() const {
  return *ptr_;
}

template<typename T>
template<bool is_const>
T** Deque<T>::CommonIterator<is_const>::get_ptr() const {
  return ptr_;
}

template<typename T>
template<bool is_const>
size_t Deque<T>::CommonIterator<is_const>::get_index() const {
  return index_;
}