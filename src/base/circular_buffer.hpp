// Minimal fixed-capacity circular buffer replacing boost::circular_buffer.
// Interface subset matches what the codebase uses.

#ifndef _HPP_CIRCULAR_BUFFER
#define _HPP_CIRCULAR_BUFFER

#include <cstddef>
#include <deque>

namespace blunted {

template <typename T>
class circular_buffer {
public:
  explicit circular_buffer(std::size_t capacity = 0) : capacity_(capacity) {}

  void push_back(const T &val) {
    if (capacity_ > 0 && buf_.size() == capacity_)
      buf_.pop_front();
    buf_.push_back(val);
  }

  std::size_t size() const { return buf_.size(); }
  bool empty() const { return buf_.empty(); }
  void clear() { buf_.clear(); }

  T &at(std::size_t i) { return buf_.at(i); }
  const T &at(std::size_t i) const { return buf_.at(i); }

  T &back() { return buf_.back(); }
  const T &back() const { return buf_.back(); }

  typename std::deque<T>::iterator begin() { return buf_.begin(); }
  typename std::deque<T>::iterator end() { return buf_.end(); }
  typename std::deque<T>::const_iterator begin() const { return buf_.begin(); }
  typename std::deque<T>::const_iterator end() const { return buf_.end(); }

private:
  std::size_t capacity_;
  std::deque<T> buf_;
};

}  // namespace blunted

#endif
