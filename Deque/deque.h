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
  ~Deque() noexcept;

  Deque<T>& operator=(const Deque<T>&);

  using iterator = CommonIterator<false>;
  using const_iterator = CommonIterator<true>;

  size_t size() const noexcept;
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

  iterator begin() noexcept;
  const_iterator begin() const noexcept;
  iterator end() noexcept;
  const_iterator end() const noexcept;
  const_iterator cbegin() const noexcept;
  const_iterator cend() const noexcept;

  std::reverse_iterator<iterator> rbegin() noexcept;
  std::reverse_iterator<iterator> rend() noexcept;
  std::reverse_iterator<const_iterator> crbegin() noexcept;
  std::reverse_iterator<const_iterator> crend() noexcept;
};

template<typename T>
const size_t Deque<T>::MAX_SIZE_ = 32;

template<typename T>
const size_t Deque<T>::START_ARRAY_COUNT_ = 8;

template<typename T>
Deque<T>::Deque(int size) : size_(size), array_count_(START_ARRAY_COUNT_) {
  while (array_count_ * MAX_SIZE_ <= 2 * size_) { // <= !!!
    array_count_ *= 2;
  }
  try {
    deque_ = reinterpret_cast<T**>(new uint8_t[array_count_ * sizeof(T*)]);
    for (size_t i = 0; i < array_count_; ++i) {
      deque_[i] = reinterpret_cast<T*>(new uint8_t[MAX_SIZE_ * sizeof(T)]);
    }
  } catch (...) {
    throw;
  }
  begin_ = {deque_ + (array_count_ / 2), MAX_SIZE_ - 1};
  start_ = {deque_, 0};
  finish_ = {deque_ + (array_count_ - 1), MAX_SIZE_};
}

template<typename T>
Deque<T>::Deque() try : Deque<T>(0) {} catch (...) {
  throw;
}

template<typename T>
Deque<T>::Deque(int size, const T& to_fill) try : Deque<T>(size) {
  for (iterator it = begin(); it != end(); ++it) {
    new(it.get_array() + it.get_index()) T(to_fill);
  }
} catch (...) {
  throw;
}

template<typename T>
Deque<T>::Deque(const Deque<T>& arg_deque) try : Deque<T>(arg_deque.size()) {
  for (auto it = begin(), arg_it = arg_deque.begin(); it != end(); ++it, ++arg_it) {
    new(it.get_array() + it.get_index()) T(*arg_it);
  }
} catch (...) {
  throw;
}

template<typename T>
Deque<T>::~Deque() noexcept {
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
    try {
      if (i >= array_count_ / 2 && i < array_count_ / 2 + array_count_) {
        new_deque[i] = deque_[i - array_count_ / 2];
      } else {
        new_deque[i] = reinterpret_cast<T*>(new uint8_t[MAX_SIZE_ * sizeof(T)]);
      }
    } catch (...) {
      for (size_t j = 0; j < i; ++j) {
        if (j < array_count_ || j >= array_count_ / 2 + array_count_) {
          delete[] reinterpret_cast<uint8_t*>(new_deque[j]);
        }
        new_deque[j] = nullptr;
      }
      delete[] reinterpret_cast<uint8_t*>(new_deque);
      throw;
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
void Deque<T>::swap(Deque<T>& arg_deque) try {
  std::swap(deque_, arg_deque.deque_);
  std::swap(size_, arg_deque.size_);
  std::swap(begin_, arg_deque.begin_);
  std::swap(start_, arg_deque.start_);
  std::swap(finish_, arg_deque.finish_);
} catch (...) {
  throw;
}

template<typename T>
Deque<T>& Deque<T>::operator=(const Deque<T>& deque) try {
  Deque<T> tmp_deque(deque);
  swap(tmp_deque);
  return *this;
} catch (...) {
  throw;
}

template<typename T>
size_t Deque<T>::size() const noexcept {
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
  if (begin() == start_) {
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
void Deque<T>::pop_front() try {
  this->erase(begin());
} catch (...) {
  throw;
}

template<typename T>
void Deque<T>::pop_back() try {
  this->erase(end() - 1);
} catch (...) {
  throw;
}

template<typename T>
void Deque<T>::erase(iterator iter) {
  if (size_ == 0) {
    throw std::out_of_range("deque is empty");
  } else if (iter < begin() || iter >= end()) {
    throw std::out_of_range("out of range");
  }
  if (iter == begin_) {
    ++begin_;
  } else {
    for (auto it = iter; it + 1 != end(); ++it) {
      *it = *(it + 1);
    }
  }
  --size_;
}

template<typename T>
void Deque<T>::insert(iterator iter, const T& element) {
  if (iter < begin() || iter > end()) {
    throw std::out_of_range("out of range");
  }
  if (end() == finish_) {
    reallocate(2 * array_count_); // iterator's invalidation
  }
  Deque<T> tmp_deque(*this);
  try {
    ++size_;
    for (auto it = end(); it != iter; --it) {
      *it = *(it - 1);
    }
    *iter = element;
  } catch (...) {
    *this = tmp_deque;
    throw;
  }
}

template<typename T>
typename Deque<T>::iterator Deque<T>::begin() noexcept {
  return begin_;
}

template<typename T>
typename Deque<T>::iterator Deque<T>::end() noexcept {
  return begin_ + size_;
}

template<typename T>
typename Deque<T>::const_iterator Deque<T>::cbegin() const noexcept {
  return const_iterator(begin_);
}

template<typename T>
typename Deque<T>::const_iterator Deque<T>::begin() const noexcept {
  return cbegin();
}

template<typename T>
typename Deque<T>::const_iterator Deque<T>::cend() const noexcept {
  return const_iterator(begin_ + size_);
}

template<typename T>
typename Deque<T>::const_iterator Deque<T>::end() const noexcept {
  return cend();
}

template<typename T>
std::reverse_iterator<typename Deque<T>::iterator> rbegin() noexcept {
  return std::reverse_iterator(Deque<T>::end());
}

template<typename T>
std::reverse_iterator<typename Deque<T>::iterator> Deque<T>::rend() noexcept {
  return std::reverse_iterator(Deque<T>::begin());
}

template<typename T>
std::reverse_iterator<typename Deque<T>::const_iterator> Deque<T>::crbegin() noexcept {
  return std::reverse_iterator(Deque<T>::cend());
}

template<typename T>
std::reverse_iterator<typename Deque<T>::const_iterator> Deque<T>::crend() noexcept {
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

  const CommonIterator<is_const> operator--(int) noexcept;
  const CommonIterator<is_const> operator++(int) noexcept;
  CommonIterator<is_const>& operator--() noexcept;
  CommonIterator<is_const>& operator++() noexcept;
  CommonIterator<is_const>& operator+=(ssize_t) noexcept;
  CommonIterator<is_const>& operator-=(ssize_t) noexcept;
  CommonIterator<is_const> operator+(ssize_t) const noexcept;
  CommonIterator<is_const> operator-(ssize_t) const noexcept;

  reference operator*() const;
  pointer operator->() const;

  size_t operator-(CommonIterator<is_const>) noexcept;
  bool operator<(CommonIterator<is_const>) noexcept;
  bool operator==(CommonIterator<is_const>) noexcept;
  bool operator>(CommonIterator<is_const>) noexcept;
  bool operator<=(CommonIterator<is_const>) noexcept;
  bool operator>=(CommonIterator<is_const>) noexcept;
  bool operator!=(CommonIterator<is_const>) noexcept;

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
Deque<T>::CommonIterator<is_const>::operator--(int) noexcept {
  CommonIterator temp_iterator(*this);
  --(*this);
  return temp_iterator;
}

template<typename T>
template<bool is_const>
const typename Deque<T>::template CommonIterator<is_const>
Deque<T>::CommonIterator<is_const>::operator++(int) noexcept {
  CommonIterator temp_iterator(*this);
  ++(*this);
  return temp_iterator;
}

template<typename T>
template<bool is_const>
typename Deque<T>::template CommonIterator<is_const>&
Deque<T>::CommonIterator<is_const>::operator--() noexcept {
  (*this) -= 1;
  return *this;
}

template<typename T>
template<bool is_const>
typename Deque<T>::template CommonIterator<is_const>&
Deque<T>::CommonIterator<is_const>::operator++() noexcept {
  (*this) += 1;
  return *this;
}

template<typename T>
template<bool is_const>
typename Deque<T>::template CommonIterator<is_const>&
Deque<T>::CommonIterator<is_const>::operator+=(ssize_t val) noexcept {
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
Deque<T>::CommonIterator<is_const>::operator-=(ssize_t val) noexcept {
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
Deque<T>::CommonIterator<is_const>::operator+(ssize_t val) const noexcept {
  CommonIterator temp_iterator(*this);
  temp_iterator += val;
  return temp_iterator;
}

template<typename T>
template<bool is_const>
typename Deque<T>::template CommonIterator<is_const>
Deque<T>::CommonIterator<is_const>::operator-(ssize_t val) const noexcept {
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
size_t
Deque<T>::CommonIterator<is_const>::operator-(typename Deque<T>::template CommonIterator<is_const> arg_it) noexcept {
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
bool
Deque<T>::CommonIterator<is_const>::operator<(typename Deque<T>::template CommonIterator<is_const> arg_it) noexcept {
  return (ptr_ < arg_it.ptr_ ||
          (ptr_ == arg_it.ptr_ && index_ < arg_it.index_));
}

template<typename T>
template<bool is_const>
bool
Deque<T>::CommonIterator<is_const>::operator==(typename Deque<T>::template CommonIterator<is_const> arg_it) noexcept {
  return (ptr_ == arg_it.ptr_ && index_ == arg_it.index_);
}

template<typename T>
template<bool is_const>
bool
Deque<T>::CommonIterator<is_const>::operator>(typename Deque<T>::template CommonIterator<is_const> arg_it) noexcept {
  return !(*this < arg_it || *this == arg_it);
}

template<typename T>
template<bool is_const>
bool
Deque<T>::CommonIterator<is_const>::operator<=(typename Deque<T>::template CommonIterator<is_const> arg_it) noexcept {
  return (*this < arg_it || *this == arg_it);
}

template<typename T>
template<bool is_const>
bool
Deque<T>::CommonIterator<is_const>::operator>=(typename Deque<T>::template CommonIterator<is_const> arg_it) noexcept {
  return !(*this < arg_it);
}

template<typename T>
template<bool is_const>
bool
Deque<T>::CommonIterator<is_const>::operator!=(typename Deque<T>::template CommonIterator<is_const> arg_it) noexcept {
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