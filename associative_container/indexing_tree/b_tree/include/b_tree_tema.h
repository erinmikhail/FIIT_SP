#ifndef SYS_PROG_B_TREE_H
#define SYS_PROG_B_TREE_H

#include "../../../include/associative_container.h"
#include "not_implemented.h"
#include "pp_allocator.h"
#include <boost/container/static_vector.hpp>
#include <cstddef>
#include <initializer_list>
#include <iterator>
#include <stack>
#include <utility>

template <typename tkey, typename tvalue,
          comparator<tkey> compare = std::less<tkey>, std::size_t t = 5>
class B_tree final : private compare // EBCO
{
public:
  using tree_data_type = std::pair<tkey, tvalue>;
  using tree_data_type_const = std::pair<const tkey, tvalue>;
  using value_type = tree_data_type_const;

private:
  static constexpr const size_t minimum_keys_in_node = t - 1;
  static constexpr const size_t maximum_keys_in_node = 2 * t - 1;

  // region comparators declaration

  inline bool compare_keys(const tkey &lhs, const tkey &rhs) const;
  inline bool compare_pairs(const tree_data_type &lhs,
                            const tree_data_type &rhs) const;

  // endregion comparators declaration

  struct btree_node {
    boost::container::static_vector<tree_data_type, maximum_keys_in_node + 1>
        _keys;
    boost::container::static_vector<btree_node *, maximum_keys_in_node + 2>
        _pointers;
    btree_node() noexcept;
  };

  pp_allocator<value_type> _allocator;
  btree_node *_root;
  size_t _size;

  pp_allocator<value_type> get_allocator() const noexcept;

  using node_allocator_type = typename std::allocator_traits<
      pp_allocator<value_type>>::template rebind_alloc<btree_node>;
  using node_traits = std::allocator_traits<node_allocator_type>;

  void destroy_node(btree_node *node) noexcept {
    if (!node)
      return;

    for (auto *child_ptr : node->_pointers) {
      destroy_node(child_ptr);
    }

    node_allocator_type node_alloc{_allocator};
    node_traits::destroy(node_alloc, node);
    node_traits::deallocate(node_alloc, node, 1);
  }

  btree_node *create_node();

  btree_node *copy_node(const btree_node *other_node);

  void insert_into_node(btree_node *node, tree_data_type &&data,
                        btree_node *right_child = nullptr);

  btree_node *split_node(btree_node *node, tree_data_type &median_data);

  void deallocate_single_node(btree_node *node) noexcept;

  void erase_from_node(btree_node *node, const tkey &key);

  void remove_from_leaf(btree_node *node, size_t idx);
  void remove_from_non_leaf(btree_node *node, size_t idx);

  tree_data_type get_predecessor(btree_node *node, size_t idx);
  tree_data_type get_successor(btree_node *node, size_t idx);

  void fill(btree_node *node, size_t idx);
  void borrow_from_prev(btree_node *node, size_t idx);
  void borrow_from_next(btree_node *node, size_t idx);
  void merge_nodes(btree_node *node, size_t idx);

public:
  // region constructors declaration

  explicit B_tree(const compare &cmp = compare(),
                  pp_allocator<value_type> = pp_allocator<value_type>());

  explicit B_tree(pp_allocator<value_type> alloc,
                  const compare &comp = compare());

  template <input_iterator_for_pair<tkey, tvalue> iterator>
  explicit B_tree(iterator begin, iterator end, const compare &cmp = compare(),
                  pp_allocator<value_type> = pp_allocator<value_type>());

  B_tree(std::initializer_list<std::pair<tkey, tvalue>> data,
         const compare &cmp = compare(),
         pp_allocator<value_type> = pp_allocator<value_type>());

  // endregion constructors declaration

  // region five declaration

  B_tree(const B_tree &other);

  B_tree(B_tree &&other) noexcept;

  B_tree &operator=(const B_tree &other);

  B_tree &operator=(B_tree &&other) noexcept;

  ~B_tree() noexcept;

  // endregion five declaration

  // region iterators declaration

  class btree_iterator;
  class btree_reverse_iterator;
  class btree_const_iterator;
  class btree_const_reverse_iterator;

  class btree_iterator final {
    std::stack<std::pair<btree_node **, size_t>> _path;
    size_t _index;

  public:
    using value_type = tree_data_type_const;
    using reference = value_type &;
    using pointer = value_type *;
    using iterator_category = std::bidirectional_iterator_tag;
    using difference_type = ptrdiff_t;
    using self = btree_iterator;

    friend class B_tree;
    friend class btree_reverse_iterator;
    friend class btree_const_iterator;
    friend class btree_const_reverse_iterator;

    reference operator*() const noexcept;
    pointer operator->() const noexcept;

    self &operator++();
    self operator++(int);

    self &operator--();
    self operator--(int);

    bool operator==(const self &other) const noexcept;
    bool operator!=(const self &other) const noexcept;

    size_t depth() const noexcept;
    size_t current_node_keys_count() const noexcept;
    bool is_terminate_node() const noexcept;
    size_t index() const noexcept;

    explicit btree_iterator(
        const std::stack<std::pair<btree_node **, size_t>> &path =
            std::stack<std::pair<btree_node **, size_t>>(),
        size_t index = 0);
  };

  class btree_const_iterator final {
    std::stack<std::pair<btree_node *const *, size_t>> _path;
    size_t _index;

  public:
    using value_type = tree_data_type_const;
    using reference = const value_type &;
    using pointer = const value_type *;
    using iterator_category = std::bidirectional_iterator_tag;
    using difference_type = ptrdiff_t;
    using self = btree_const_iterator;

    friend class B_tree;
    friend class btree_reverse_iterator;
    friend class btree_iterator;
    friend class btree_const_reverse_iterator;

    btree_const_iterator(const btree_iterator &it) noexcept;

    reference operator*() const noexcept;
    pointer operator->() const noexcept;

    self &operator++();
    self operator++(int);

    self &operator--();
    self operator--(int);

    bool operator==(const self &other) const noexcept;
    bool operator!=(const self &other) const noexcept;

    size_t depth() const noexcept;
    size_t current_node_keys_count() const noexcept;
    bool is_terminate_node() const noexcept;
    size_t index() const noexcept;

    explicit btree_const_iterator(
        const std::stack<std::pair<btree_node *const *, size_t>> &path =
            std::stack<std::pair<btree_node *const *, size_t>>(),
        size_t index = 0);
  };

  class btree_reverse_iterator final {
    std::stack<std::pair<btree_node **, size_t>> _path;
    size_t _index;

  public:
    using value_type = tree_data_type_const;
    using reference = value_type &;
    using pointer = value_type *;
    using iterator_category = std::bidirectional_iterator_tag;
    using difference_type = ptrdiff_t;
    using self = btree_reverse_iterator;

    friend class B_tree;
    friend class btree_iterator;
    friend class btree_const_iterator;
    friend class btree_const_reverse_iterator;

    btree_reverse_iterator(const btree_iterator &it) noexcept;
    operator btree_iterator() const noexcept;

    reference operator*() const noexcept;
    pointer operator->() const noexcept;

    self &operator++();
    self operator++(int);

    self &operator--();
    self operator--(int);

    bool operator==(const self &other) const noexcept;
    bool operator!=(const self &other) const noexcept;

    size_t depth() const noexcept;
    size_t current_node_keys_count() const noexcept;
    bool is_terminate_node() const noexcept;
    size_t index() const noexcept;

    explicit btree_reverse_iterator(
        const std::stack<std::pair<btree_node **, size_t>> &path =
            std::stack<std::pair<btree_node **, size_t>>(),
        size_t index = 0);
  };

  class btree_const_reverse_iterator final {
    std::stack<std::pair<btree_node *const *, size_t>> _path;
    size_t _index;

  public:
    using value_type = tree_data_type_const;
    using reference = const value_type &;
    using pointer = const value_type *;
    using iterator_category = std::bidirectional_iterator_tag;
    using difference_type = ptrdiff_t;
    using self = btree_const_reverse_iterator;

    friend class B_tree;
    friend class btree_reverse_iterator;
    friend class btree_const_iterator;
    friend class btree_iterator;

    btree_const_reverse_iterator(const btree_reverse_iterator &it) noexcept;
    operator btree_const_iterator() const noexcept;

    reference operator*() const noexcept;
    pointer operator->() const noexcept;

    self &operator++();
    self operator++(int);

    self &operator--();
    self operator--(int);

    bool operator==(const self &other) const noexcept;
    bool operator!=(const self &other) const noexcept;

    size_t depth() const noexcept;
    size_t current_node_keys_count() const noexcept;
    bool is_terminate_node() const noexcept;
    size_t index() const noexcept;

    explicit btree_const_reverse_iterator(
        const std::stack<std::pair<btree_node *const *, size_t>> &path =
            std::stack<std::pair<btree_node *const *, size_t>>(),
        size_t index = 0);
  };

  friend class btree_iterator;
  friend class btree_const_iterator;
  friend class btree_reverse_iterator;
  friend class btree_const_reverse_iterator;

  // endregion iterators declaration

  // region element access declaration

  /*
   * Returns a reference to the mapped value of the element with specified key.
   * If no such element exists, an exception of type std::out_of_range is
   * thrown.
   */
  tvalue &at(const tkey &);
  const tvalue &at(const tkey &) const;

  /*
   * If key not exists, makes default initialization of value
   */
  tvalue &operator[](const tkey &key);
  tvalue &operator[](tkey &&key);

  // endregion element access declaration
  // region iterator begins declaration

  btree_iterator begin();
  btree_iterator end();

  btree_const_iterator begin() const;
  btree_const_iterator end() const;

  btree_const_iterator cbegin() const;
  btree_const_iterator cend() const;

  btree_reverse_iterator rbegin();
  btree_reverse_iterator rend();

  btree_const_reverse_iterator rbegin() const;
  btree_const_reverse_iterator rend() const;

  btree_const_reverse_iterator crbegin() const;
  btree_const_reverse_iterator crend() const;

  // endregion iterator begins declaration

  // region lookup declaration

  size_t size() const noexcept;
  bool empty() const noexcept;

  /*
   * Returns end() if not exist
   */

  btree_iterator find(const tkey &key);
  btree_const_iterator find(const tkey &key) const;

  btree_iterator lower_bound(const tkey &key);
  btree_const_iterator lower_bound(const tkey &key) const;

  btree_iterator upper_bound(const tkey &key);
  btree_const_iterator upper_bound(const tkey &key) const;

  bool contains(const tkey &key) const;

  // endregion lookup declaration

  // region modifiers declaration

  void clear() noexcept;

  /*
   * Does nothing if key exists, delegates to emplace.
   * Second return value is true, when inserted
   */
  std::pair<btree_iterator, bool> insert(const tree_data_type &data);
  std::pair<btree_iterator, bool> insert(tree_data_type &&data);

  template <typename... Args>
  std::pair<btree_iterator, bool> emplace(Args &&...args);

  /*
   * Updates value if key exists, delegates to emplace.
   */
  btree_iterator insert_or_assign(const tree_data_type &data);
  btree_iterator insert_or_assign(tree_data_type &&data);

  template <typename... Args> btree_iterator emplace_or_assign(Args &&...args);

  /*
   * Return iterator to node next ro removed or end() if key not exists
   */
  btree_iterator erase(btree_iterator pos);
  btree_iterator erase(btree_const_iterator pos);

  btree_iterator erase(btree_iterator beg, btree_iterator en);
  btree_iterator erase(btree_const_iterator beg, btree_const_iterator en);

  btree_iterator erase(const tkey &key);

  // endregion modifiers declaration
};

template <
    std::input_iterator iterator,
    comparator<typename std::iterator_traits<iterator>::value_type::first_type>
        compare = std::less<
            typename std::iterator_traits<iterator>::value_type::first_type>,
    std::size_t t = 5, typename U>
B_tree(iterator begin, iterator end, const compare &cmp = compare(),
       pp_allocator<U> = pp_allocator<U>())
    -> B_tree<typename std::iterator_traits<iterator>::value_type::first_type,
              typename std::iterator_traits<iterator>::value_type::second_type,
              compare, t>;

template <typename tkey, typename tvalue,
          comparator<tkey> compare = std::less<tkey>, std::size_t t = 5,
          typename U>
B_tree(std::initializer_list<std::pair<tkey, tvalue>> data,
       const compare &cmp = compare(), pp_allocator<U> = pp_allocator<U>())
    -> B_tree<tkey, tvalue, compare, t>;

template <typename tkey, typename tvalue, comparator<tkey> compare,
          std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::compare_pairs(
    const B_tree::tree_data_type &lhs,
    const B_tree::tree_data_type &rhs) const {
  return compare_keys(lhs.first, rhs.first);
}

template <typename tkey, typename tvalue, comparator<tkey> compare,
          std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::compare_keys(const tkey &lhs,
                                                    const tkey &rhs) const {
  return compare::operator()(lhs, rhs);
}

template <typename tkey, typename tvalue, comparator<tkey> compare,
          std::size_t t>
B_tree<tkey, tvalue, compare, t>::btree_node::btree_node() noexcept {}

template <typename tkey, typename tvalue, comparator<tkey> compare,
          std::size_t t>
pp_allocator<typename B_tree<tkey, tvalue, compare, t>::value_type>
B_tree<tkey, tvalue, compare, t>::get_allocator() const noexcept {
  return _allocator;
}

// region constructors implementation

template <typename tkey, typename tvalue, comparator<tkey> compare,
          std::size_t t>
B_tree<tkey, tvalue, compare, t>::B_tree(const compare &cmp,
                                         pp_allocator<value_type> alloc)
    : _allocator(alloc), _root(nullptr), _size(0) {
  *static_cast<compare *>(this) = cmp;
}

template <typename tkey, typename tvalue, comparator<tkey> compare,
          std::size_t t>
B_tree<tkey, tvalue, compare, t>::B_tree(pp_allocator<value_type> alloc,
                                         const compare &comp)
    : _allocator(alloc), _root(nullptr), _size(0) {
  *static_cast<compare *>(this) = comp;
}

template <typename tkey, typename tvalue, comparator<tkey> compare,
          std::size_t t>
template <input_iterator_for_pair<tkey, tvalue> iterator>
B_tree<tkey, tvalue, compare, t>::B_tree(iterator begin, iterator end,
                                         const compare &cmp,
                                         pp_allocator<value_type> alloc)
    : _allocator(alloc), _root(nullptr), _size(0) {
  *static_cast<compare *>(this) = cmp;
  for (auto it = begin; it != end; ++it) {
    insert(*it);
  }
}

template <typename tkey, typename tvalue, comparator<tkey> compare,
          std::size_t t>
B_tree<tkey, tvalue, compare, t>::B_tree(
    std::initializer_list<std::pair<tkey, tvalue>> data, const compare &cmp,
    pp_allocator<value_type> alloc)
    : _allocator(alloc), _root(nullptr), _size(0) {
  *static_cast<compare *>(this) = cmp;
  for (const auto &item : data) {
    insert(item);
  }
}

// endregion constructors implementation

// region five implementation

template <typename tkey, typename tvalue, comparator<tkey> compare,
          std::size_t t>
B_tree<tkey, tvalue, compare, t>::~B_tree() noexcept {
  clear();
}

template <typename tkey, typename tvalue, comparator<tkey> compare,
          std::size_t t>
B_tree<tkey, tvalue, compare, t>::B_tree(const B_tree &other)
    : _allocator(other._allocator), _root(nullptr), _size(other._size) {
  *static_cast<compare *>(this) = static_cast<const compare &>(other);

  if (other._root != nullptr) {
    _root = copy_node(other._root);
  }
}

template <typename tkey, typename tvalue, comparator<tkey> compare,
          std::size_t t>
B_tree<tkey, tvalue, compare, t> &
B_tree<tkey, tvalue, compare, t>::operator=(const B_tree &other) {
  if (this != &other) {
    clear();

    _allocator = other._allocator;
    *static_cast<compare *>(this) = static_cast<const compare &>(other);
    _size = other._size;

    if (other._root != nullptr) {
      _root = copy_node(other._root);
    }
  }
  return *this;
}

template <typename tkey, typename tvalue, comparator<tkey> compare,
          std::size_t t>
B_tree<tkey, tvalue, compare, t>::B_tree(B_tree &&other) noexcept
    : _allocator(std::move(other._allocator)), _root(other._root),
      _size(other._size) {
  *static_cast<compare *>(this) = std::move(static_cast<compare &>(other));

  other._root = nullptr;
  other._size = 0;
}

template <typename tkey, typename tvalue, comparator<tkey> compare,
          std::size_t t>
B_tree<tkey, tvalue, compare, t> &
B_tree<tkey, tvalue, compare, t>::operator=(B_tree &&other) noexcept {
  if (this != &other) {
    clear();

    _allocator = std::move(other._allocator);
    *static_cast<compare *>(this) = std::move(static_cast<compare &>(other));

    _root = other._root;
    _size = other._size;

    other._root = nullptr;
    other._size = 0;
  }
  return *this;
}

// endregion five implementation

// region iterators implementation

template <typename tkey, typename tvalue, comparator<tkey> compare,
          std::size_t t>
B_tree<tkey, tvalue, compare, t>::btree_iterator::btree_iterator(
    const std::stack<std::pair<btree_node **, size_t>> &path, size_t index)
    : _path(path), _index(index) {}

template <typename tkey, typename tvalue, comparator<tkey> compare,
          std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator::reference
B_tree<tkey, tvalue, compare, t>::btree_iterator::operator*() const noexcept {
  return reinterpret_cast<reference>((*(_path.top().first))->_keys[_index]);
}

template <typename tkey, typename tvalue, comparator<tkey> compare,
          std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator::pointer
B_tree<tkey, tvalue, compare, t>::btree_iterator::operator->() const noexcept {
  return &((*this).operator*());
}

template <typename tkey, typename tvalue, comparator<tkey> compare,
          std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator &
B_tree<tkey, tvalue, compare, t>::btree_iterator::operator++() {
  if (_path.empty()) {
    return *this;
  }

  btree_node *curr = *(_path.top().first);

  if (!curr->_pointers.empty()) {
    _path.top().second = _index + 1;
    btree_node **child_ptr = &(curr->_pointers[_index + 1]);
    _path.push({child_ptr, 0});
    curr = *child_ptr;
    while (!curr->_pointers.empty()) {
      child_ptr = &(curr->_pointers[0]);
      _path.push({child_ptr, 0});
      curr = *child_ptr;
    }
    _index = 0;
  } else {
    _index++;
    while (!_path.empty() && _index >= (*(_path.top().first))->_keys.size()) {
      _path.pop();
      if (!_path.empty()) {
        _index = _path.top().second;
      }
    }
  }
  return *this;
}

template <typename tkey, typename tvalue, comparator<tkey> compare,
          std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator
B_tree<tkey, tvalue, compare, t>::btree_iterator::operator++(int) {
  auto temp = *this;
  ++(*this);
  return temp;
}

template <typename tkey, typename tvalue, comparator<tkey> compare,
          std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator &
B_tree<tkey, tvalue, compare, t>::btree_iterator::operator--() {
  if (_path.empty()) {
    return *this;
  }
  btree_node *curr = *(_path.top().first);

  if (!curr->_pointers.empty()) {
    _path.top().second = _index;
    btree_node **child_ptr = &(curr->_pointers[_index]);
    curr = *child_ptr;
    _path.push({child_ptr, curr->_keys.size()});
    while (!curr->_pointers.empty()) {
      child_ptr = &(curr->_pointers.back());
      curr = *child_ptr;
      _path.push({child_ptr, curr->_keys.size()});
    }
    _index = curr->_keys.size() - 1;
  } else {
    if (_index > 0) {
      _index--;
    } else {
      while (!_path.empty()) {
        _path.pop();
        if (!_path.empty()) {
          size_t parent_idx = _path.top().second;
          if (parent_idx > 0) {
            _index = parent_idx - 1;
            break;
          }
        }
      }
    }
  }
  return *this;
}

template <typename tkey, typename tvalue, comparator<tkey> compare,
          std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator
B_tree<tkey, tvalue, compare, t>::btree_iterator::operator--(int) {
  auto temp = *this;
  --(*this);
  return temp;
}

template <typename tkey, typename tvalue, comparator<tkey> compare,
          std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::btree_iterator::operator==(
    const self &other) const noexcept {
  if (_path.empty() || other._path.empty()) {
    return _path.empty() == other._path.empty();
  }
  return _path.top().first == other._path.top().first && _index == other._index;
}

template <typename tkey, typename tvalue, comparator<tkey> compare,
          std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::btree_iterator::operator!=(
    const self &other) const noexcept {
  return !(*this == other);
}

template <typename tkey, typename tvalue, comparator<tkey> compare,
          std::size_t t>
size_t
B_tree<tkey, tvalue, compare, t>::btree_iterator::depth() const noexcept {
  return _path.empty() ? 0 : _path.size() - 1;
}

template <typename tkey, typename tvalue, comparator<tkey> compare,
          std::size_t t>
size_t
B_tree<tkey, tvalue, compare, t>::btree_iterator::current_node_keys_count()
    const noexcept {
  return _path.empty() ? 0 : (*(_path.top().first))->_keys.size();
}

template <typename tkey, typename tvalue, comparator<tkey> compare,
          std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::btree_iterator::is_terminate_node()
    const noexcept {
  return _path.empty();
}

template <typename tkey, typename tvalue, comparator<tkey> compare,
          std::size_t t>
size_t
B_tree<tkey, tvalue, compare, t>::btree_iterator::index() const noexcept {
  return _index;
}

template <typename tkey, typename tvalue, comparator<tkey> compare,
          std::size_t t>
B_tree<tkey, tvalue, compare, t>::btree_const_iterator::btree_const_iterator(
    const std::stack<std::pair<btree_node *const *, size_t>> &path,
    size_t index)
    : _path(path), _index(index) {}

template <typename tkey, typename tvalue, comparator<tkey> compare,
          std::size_t t>
B_tree<tkey, tvalue, compare, t>::btree_const_iterator::btree_const_iterator(
    const btree_iterator &it) noexcept
    : _index(it._index) {
  auto temp = it._path;
  std::vector<std::pair<btree_node *const *, size_t>> rev;
  while (!temp.empty()) {
    rev.push_back({temp.top().first, temp.top().second});
    temp.pop();
  }
  for (auto rit = rev.rbegin(); rit != rev.rend(); ++rit)
    _path.push(*rit);
}

template <typename tkey, typename tvalue, comparator<tkey> compare,
          std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_iterator::reference
B_tree<tkey, tvalue, compare, t>::btree_const_iterator::operator*()
    const noexcept {
  return reinterpret_cast<reference>((*(_path.top().first))->_keys[_index]);
}

template <typename tkey, typename tvalue, comparator<tkey> compare,
          std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_iterator::pointer
B_tree<tkey, tvalue, compare, t>::btree_const_iterator::operator->()
    const noexcept {
  return &((*this).operator*());
}

template <typename tkey, typename tvalue, comparator<tkey> compare,
          std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_iterator &
B_tree<tkey, tvalue, compare, t>::btree_const_iterator::operator++() {
  if (_path.empty()) {
    return *this;
  }

  const btree_node *curr = *(_path.top().first);

  if (!curr->_pointers.empty()) {
    _path.top().second = _index + 1;
    btree_node *const *child_ptr = &(curr->_pointers[_index + 1]);
    _path.push({child_ptr, 0});
    curr = *child_ptr;
    while (!curr->_pointers.empty()) {
      child_ptr = &(curr->_pointers[0]);
      _path.push({child_ptr, 0});
      curr = *child_ptr;
    }
    _index = 0;
  } else {
    _index++;
    while (!_path.empty() && _index >= (*(_path.top().first))->_keys.size()) {
      _path.pop();
      if (!_path.empty()) {
        _index = _path.top().second;
      }
    }
  }
  return *this;
}

template <typename tkey, typename tvalue, comparator<tkey> compare,
          std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_iterator
B_tree<tkey, tvalue, compare, t>::btree_const_iterator::operator++(int) {
  auto temp = *this;
  ++(*this);
  return temp;
}

template <typename tkey, typename tvalue, comparator<tkey> compare,
          std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_iterator &
B_tree<tkey, tvalue, compare, t>::btree_const_iterator::operator--() {
  if (_path.empty()) {
    return *this;
  }
  const btree_node *curr = *(_path.top().first);

  if (!curr->_pointers.empty()) {
    _path.top().second = _index;
    btree_node *const *child_ptr = &(curr->_pointers[_index]);
    curr = *child_ptr;
    _path.push({child_ptr, curr->_keys.size()});
    while (!curr->_pointers.empty()) {
      child_ptr = &(curr->_pointers.back());
      curr = *child_ptr;
      _path.push({child_ptr, curr->_keys.size()});
    }
    _index = curr->_keys.size() - 1;
  } else {
    if (_index > 0) {
      _index--;
    } else {
      while (!_path.empty()) {
        _path.pop();
        if (!_path.empty()) {
          size_t parent_idx = _path.top().second;
          if (parent_idx > 0) {
            _index = parent_idx - 1;
            break;
          }
        }
      }
    }
  }
  return *this;
}

template <typename tkey, typename tvalue, comparator<tkey> compare,
          std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_iterator
B_tree<tkey, tvalue, compare, t>::btree_const_iterator::operator--(int) {
  auto temp = *this;
  --(*this);
  return temp;
}

template <typename tkey, typename tvalue, comparator<tkey> compare,
          std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::btree_const_iterator::operator==(
    const self &other) const noexcept {
  if (_path.empty() || other._path.empty()) {
    return _path.empty() == other._path.empty();
  }
  return _path.top().first == other._path.top().first && _index == other._index;
}

template <typename tkey, typename tvalue, comparator<tkey> compare,
          std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::btree_const_iterator::operator!=(
    const self &other) const noexcept {
  return !(*this == other);
}

template <typename tkey, typename tvalue, comparator<tkey> compare,
          std::size_t t>
size_t
B_tree<tkey, tvalue, compare, t>::btree_const_iterator::depth() const noexcept {
  return _path.empty() ? 0 : _path.size() - 1;
}

template <typename tkey, typename tvalue, comparator<tkey> compare,
          std::size_t t>
size_t
B_tree<tkey, tvalue, compare,
       t>::btree_const_iterator::current_node_keys_count() const noexcept {
  return _path.empty() ? 0 : (*(_path.top().first))->_keys.size();
}

template <typename tkey, typename tvalue, comparator<tkey> compare,
          std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::btree_const_iterator::is_terminate_node()
    const noexcept {
  return _path.empty();
}

template <typename tkey, typename tvalue, comparator<tkey> compare,
          std::size_t t>
size_t
B_tree<tkey, tvalue, compare, t>::btree_const_iterator::index() const noexcept {
  return _index;
}

template <typename tkey, typename tvalue, comparator<tkey> compare,
          std::size_t t>
B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::
    btree_reverse_iterator(
        const std::stack<std::pair<btree_node **, size_t>> &path, size_t index)
    : _path(path), _index(index) {}

template <typename tkey, typename tvalue, comparator<tkey> compare,
          std::size_t t>
B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::
    btree_reverse_iterator(const btree_iterator &it) noexcept
    : _path(it._path), _index(it._index) {}

template <typename tkey, typename tvalue, comparator<tkey> compare,
          std::size_t t>
B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::operator B_tree<
    tkey, tvalue, compare, t>::btree_iterator() const noexcept {
  return btree_iterator(_path, _index);
}

template <typename tkey, typename tvalue, comparator<tkey> compare,
          std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::reference
B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::operator*()
    const noexcept {
  return reinterpret_cast<reference>((*(_path.top().first))->_keys[_index]);
}

template <typename tkey, typename tvalue, comparator<tkey> compare,
          std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::pointer
B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::operator->()
    const noexcept {
  return &((*this).operator*());
}

template <typename tkey, typename tvalue, comparator<tkey> compare,
          std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator &
B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::operator++() {
  btree_iterator it(_path, _index);
  --it;
  _path = it._path;
  _index = it._index;
  return *this;
}

template <typename tkey, typename tvalue, comparator<tkey> compare,
          std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator
B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::operator++(int) {
  auto temp = *this;
  ++(*this);
  return temp;
}

template <typename tkey, typename tvalue, comparator<tkey> compare,
          std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator &
B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::operator--() {
  btree_iterator it(_path, _index);
  ++it;
  _path = it._path;
  _index = it._index;
  return *this;
}

template <typename tkey, typename tvalue, comparator<tkey> compare,
          std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator
B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::operator--(int) {
  auto temp = *this;
  --(*this);
  return temp;
}

template <typename tkey, typename tvalue, comparator<tkey> compare,
          std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::operator==(
    const self &other) const noexcept {
  return _path.empty() || other._path.empty()
             ? _path.empty() == other._path.empty()
             : _path.top().first == other._path.top().first &&
                   _index == other._index;
}

template <typename tkey, typename tvalue, comparator<tkey> compare,
          std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::operator!=(
    const self &other) const noexcept {
  return !(*this == other);
}

template <typename tkey, typename tvalue, comparator<tkey> compare,
          std::size_t t>
size_t B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::depth()
    const noexcept {
  return _path.empty() ? 0 : _path.size() - 1;
}

template <typename tkey, typename tvalue, comparator<tkey> compare,
          std::size_t t>
size_t
B_tree<tkey, tvalue, compare,
       t>::btree_reverse_iterator::current_node_keys_count() const noexcept {
  return _path.empty() ? 0 : (*(_path.top().first))->_keys.size();
}

template <typename tkey, typename tvalue, comparator<tkey> compare,
          std::size_t t>
bool B_tree<tkey, tvalue, compare,
            t>::btree_reverse_iterator::is_terminate_node() const noexcept {
  return _path.empty();
}

template <typename tkey, typename tvalue, comparator<tkey> compare,
          std::size_t t>
size_t B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::index()
    const noexcept {
  return _index;
}

template <typename tkey, typename tvalue, comparator<tkey> compare,
          std::size_t t>
B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::
    btree_const_reverse_iterator(
        const std::stack<std::pair<btree_node *const *, size_t>> &path,
        size_t index)
    : _path(path), _index(index) {}

template <typename tkey, typename tvalue, comparator<tkey> compare,
          std::size_t t>
B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::
    btree_const_reverse_iterator(const btree_reverse_iterator &it) noexcept
    : _index(it._index) {
  auto temp = it._path;
  std::vector<std::pair<btree_node *const *, size_t>> rev;
  while (!temp.empty()) {
    rev.push_back({temp.top().first, temp.top().second});
    temp.pop();
  }
  for (auto rit = rev.rbegin(); rit != rev.rend(); ++rit)
    _path.push(*rit);
}

template <typename tkey, typename tvalue, comparator<tkey> compare,
          std::size_t t>
B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::operator B_tree<
    tkey, tvalue, compare, t>::btree_const_iterator() const noexcept {
  return btree_const_iterator(_path, _index);
}

template <typename tkey, typename tvalue, comparator<tkey> compare,
          std::size_t t>
typename B_tree<tkey, tvalue, compare,
                t>::btree_const_reverse_iterator::reference
B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::operator*()
    const noexcept {
  return reinterpret_cast<reference>((*(_path.top().first))->_keys[_index]);
}

template <typename tkey, typename tvalue, comparator<tkey> compare,
          std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::pointer
B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::operator->()
    const noexcept {
  return &((*this).operator*());
}

template <typename tkey, typename tvalue, comparator<tkey> compare,
          std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator &
B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::operator++() {
  btree_const_iterator it(_path, _index);
  --it;
  _path = it._path;
  _index = it._index;
  return *this;
}

template <typename tkey, typename tvalue, comparator<tkey> compare,
          std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator
B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::operator++(
    int) {
  auto temp = *this;
  ++(*this);
  return temp;
}

template <typename tkey, typename tvalue, comparator<tkey> compare,
          std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator &
B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::operator--() {
  btree_const_iterator it(_path, _index);
  ++it;
  _path = it._path;
  _index = it._index;
  return *this;
}

template <typename tkey, typename tvalue, comparator<tkey> compare,
          std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator
B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::operator--(
    int) {
  auto temp = *this;
  --(*this);
  return temp;
}

template <typename tkey, typename tvalue, comparator<tkey> compare,
          std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::operator==(
    const self &other) const noexcept {
  return _path.empty() || other._path.empty()
             ? _path.empty() == other._path.empty()
             : _path.top().first == other._path.top().first &&
                   _index == other._index;
}

template <typename tkey, typename tvalue, comparator<tkey> compare,
          std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::operator!=(
    const self &other) const noexcept {
  return !(*this == other);
}

template <typename tkey, typename tvalue, comparator<tkey> compare,
          std::size_t t>
size_t B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::depth()
    const noexcept {
  return _path.empty() ? 0 : _path.size() - 1;
}

template <typename tkey, typename tvalue, comparator<tkey> compare,
          std::size_t t>
size_t B_tree<tkey, tvalue, compare,
              t>::btree_const_reverse_iterator::current_node_keys_count()
    const noexcept {
  return _path.empty() ? 0 : (*(_path.top().first))->_keys.size();
}

template <typename tkey, typename tvalue, comparator<tkey> compare,
          std::size_t t>
bool B_tree<tkey, tvalue, compare,
            t>::btree_const_reverse_iterator::is_terminate_node()
    const noexcept {
  return _path.empty();
}

template <typename tkey, typename tvalue, comparator<tkey> compare,
          std::size_t t>
size_t B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::index()
    const noexcept {
  return _index;
}

// endregion iterators implementation

// region element access implementation

template <typename tkey, typename tvalue, comparator<tkey> compare,
          std::size_t t>
tvalue &B_tree<tkey, tvalue, compare, t>::at(const tkey &key) {
  auto it = find(key);
  if (it == end()) {
    throw std::out_of_range("Key not found in B-tree");
  }
  return it->second;
}

template <typename tkey, typename tvalue, comparator<tkey> compare,
          std::size_t t>
const tvalue &B_tree<tkey, tvalue, compare, t>::at(const tkey &key) const {
  auto it = find(key);
  if (it == end()) {
    throw std::out_of_range("Key not found in B-tree");
  }
  return it->second;
}

template <typename tkey, typename tvalue, comparator<tkey> compare,
          std::size_t t>
tvalue &B_tree<tkey, tvalue, compare, t>::operator[](const tkey &key) {
  auto result = insert({key, tvalue()});
  return result.first->second;
}

template <typename tkey, typename tvalue, comparator<tkey> compare,
          std::size_t t>
tvalue &B_tree<tkey, tvalue, compare, t>::operator[](tkey &&key) {
  auto result = insert({std::move(key), tvalue()});
  return result.first->second;
}

// endregion element access implementation

// region iterator begins implementation

template <typename tkey, typename tvalue, comparator<tkey> compare,
          std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator
B_tree<tkey, tvalue, compare, t>::begin() {
  if (_root == nullptr)
    return end();

  std::stack<std::pair<btree_node **, size_t>> path;
  btree_node **curr = &_root;

  while (curr != nullptr && *curr != nullptr) {
    path.push({curr, 0});
    if ((*curr)->_pointers.empty())
      break;
    curr = &((*curr)->_pointers[0]);
  }

  return btree_iterator(path, 0);
}

template <typename tkey, typename tvalue, comparator<tkey> compare,
          std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator
B_tree<tkey, tvalue, compare, t>::end() {
  return btree_iterator();
}

template <typename tkey, typename tvalue, comparator<tkey> compare,
          std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_iterator
B_tree<tkey, tvalue, compare, t>::begin() const {
  return cbegin();
}

template <typename tkey, typename tvalue, comparator<tkey> compare,
          std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_iterator
B_tree<tkey, tvalue, compare, t>::end() const {
  return btree_iterator();
}

template <typename tkey, typename tvalue, comparator<tkey> compare,
          std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_iterator
B_tree<tkey, tvalue, compare, t>::cbegin() const {
  if (_root == nullptr)
    return cend();

  std::stack<std::pair<btree_node *const *, size_t>> path;
  btree_node *const *curr = &_root;

  while (curr != nullptr && *curr != nullptr) {
    path.push({curr, 0});
    if ((*curr)->_pointers.empty())
      break;
    curr = &((*curr)->_pointers[0]);
  }

  return btree_const_iterator(path, 0);
}

template <typename tkey, typename tvalue, comparator<tkey> compare,
          std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_iterator
B_tree<tkey, tvalue, compare, t>::cend() const {
  return btree_const_iterator();
}

template <typename tkey, typename tvalue, comparator<tkey> compare,
          std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator
B_tree<tkey, tvalue, compare, t>::rbegin() {
  if (_root == nullptr) {
    return rend();
  }
  std::stack<std::pair<btree_node **, size_t>> path;
  btree_node **curr = &_root;
  while (curr != nullptr && *curr != nullptr) {
    path.push({curr, (*curr)->_keys.size()});
    if ((*curr)->_pointers.empty()) {
      path.top().second--;
      break;
    }
    curr = &((*curr)->_pointers.back());
  }
  return btree_reverse_iterator(path, path.top().second);
}

template <typename tkey, typename tvalue, comparator<tkey> compare,
          std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator
B_tree<tkey, tvalue, compare, t>::rend() {
  return btree_reverse_iterator();
}

template <typename tkey, typename tvalue, comparator<tkey> compare,
          std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator
B_tree<tkey, tvalue, compare, t>::rbegin() const {
  return crbegin();
}

template <typename tkey, typename tvalue, comparator<tkey> compare,
          std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator
B_tree<tkey, tvalue, compare, t>::rend() const {
  return crend();
}

template <typename tkey, typename tvalue, comparator<tkey> compare,
          std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator
B_tree<tkey, tvalue, compare, t>::crbegin() const {
  if (_root == nullptr) {
    return crend();
  }
  std::stack<std::pair<btree_node *const *, size_t>> path;
  btree_node *const *curr = &_root;
  while (curr != nullptr && *curr != nullptr) {
    path.push({curr, (*curr)->_keys.size()});
    if ((*curr)->_pointers.empty()) {
      path.top().second--;
      break;
    }
    curr = &((*curr)->_pointers.back());
  }
  return btree_const_reverse_iterator(path, path.top().second);
}

template <typename tkey, typename tvalue, comparator<tkey> compare,
          std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator
B_tree<tkey, tvalue, compare, t>::crend() const {
  return btree_const_reverse_iterator();
}

// endregion iterator begins implementation

// region lookup implementation

template <typename tkey, typename tvalue, comparator<tkey> compare,
          std::size_t t>
size_t B_tree<tkey, tvalue, compare, t>::size() const noexcept {
  return _size;
}

template <typename tkey, typename tvalue, comparator<tkey> compare,
          std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::empty() const noexcept {
  return _size == 0;
}

template <typename tkey, typename tvalue, comparator<tkey> compare,
          std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator
B_tree<tkey, tvalue, compare, t>::find(const tkey &key) {
  std::stack<std::pair<btree_node **, size_t>> path;

  btree_node **current_ptr = &_root;

  while (*current_ptr != nullptr) {
    size_t i = 0;

    while (i < (*current_ptr)->_keys.size() &&
           compare_keys((*current_ptr)->_keys[i].first, key)) {
      ++i;
    }

    if (i < (*current_ptr)->_keys.size() &&
        !compare_keys((*current_ptr)->_keys[i].first, key) &&
        !compare_keys(key, (*current_ptr)->_keys[i].first)) {

      path.push({current_ptr, i});
      return btree_iterator(path, i);
    }

    if ((*current_ptr)->_pointers.empty()) {
      return end();
    }

    path.push({current_ptr, i});

    current_ptr = &((*current_ptr)->_pointers[i]);
  }

  return end();
}

template <typename tkey, typename tvalue, comparator<tkey> compare,
          std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_iterator
B_tree<tkey, tvalue, compare, t>::find(const tkey &key) const {
  std::stack<std::pair<btree_node *const *, size_t>> path;
  btree_node *const *current_ptr = &_root;

  while (*current_ptr != nullptr) {
    size_t i = 0;
    while (i < (*current_ptr)->_keys.size() &&
           compare_keys((*current_ptr)->_keys[i].first, key)) {
      ++i;
    }

    if (i < (*current_ptr)->_keys.size() &&
        !compare_keys((*current_ptr)->_keys[i].first, key) &&
        !compare_keys(key, (*current_ptr)->_keys[i].first)) {
      path.push({current_ptr, i});
      return btree_const_iterator(path, i);
    }

    if ((*current_ptr)->_pointers.empty()) {
      return end();
    }

    path.push({current_ptr, i});
    current_ptr = &((*current_ptr)->_pointers[i]);
  }

  return end();
}

template <typename tkey, typename tvalue, comparator<tkey> compare,
          std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator
B_tree<tkey, tvalue, compare, t>::lower_bound(const tkey &key) {
  std::stack<std::pair<btree_node **, size_t>> best_path;
  size_t best_idx = 0;
  std::stack<std::pair<btree_node **, size_t>> current_path;
  btree_node **curr = &_root;

  while (curr && *curr) {
    size_t i = 0;
    while (i < (*curr)->_keys.size() &&
           compare_keys((*curr)->_keys[i].first, key)) {
      i++;
    }

    if (i < (*curr)->_keys.size()) {
      best_path = current_path;
      best_path.push({curr, i});
      best_idx = i;
    }
    if ((*curr)->_pointers.empty()) {
      break;
    }
    current_path.push({curr, i});
    curr = &((*curr)->_pointers[i]);
  }
  if (!best_path.empty()) {
    return btree_iterator(best_path, best_idx);
  }
  return end();
}

template <typename tkey, typename tvalue, comparator<tkey> compare,
          std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_iterator
B_tree<tkey, tvalue, compare, t>::lower_bound(const tkey &key) const {
  std::stack<std::pair<btree_node *const *, size_t>> best_path;
  size_t best_idx = 0;
  std::stack<std::pair<btree_node *const *, size_t>> current_path;
  btree_node *const *curr = &_root;

  while (curr && *curr) {
    size_t i = 0;
    while (i < (*curr)->_keys.size() &&
           compare_keys((*curr)->_keys[i].first, key)) {
      i++;
    }

    if (i < (*curr)->_keys.size()) {
      best_path = current_path;
      best_path.push({curr, i});
      best_idx = i;
    }
    if ((*curr)->_pointers.empty()) {
      break;
    }
    current_path.push({curr, i});
    curr = &((*curr)->_pointers[i]);
  }
  if (!best_path.empty()) {
    return btree_const_iterator(best_path, best_idx);
  }
  return end();
}

template <typename tkey, typename tvalue, comparator<tkey> compare,
          std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator
B_tree<tkey, tvalue, compare, t>::upper_bound(const tkey &key) {
  std::stack<std::pair<btree_node **, size_t>> best_path;
  size_t best_idx = 0;
  std::stack<std::pair<btree_node **, size_t>> current_path;
  btree_node **curr = &_root;

  while (curr && *curr) {
    size_t i = 0;
    while (i < (*curr)->_keys.size() &&
           !compare_keys(key, (*curr)->_keys[i].first)) {
      i++;
    }

    if (i < (*curr)->_keys.size()) {
      best_path = current_path;
      best_path.push({curr, i});
      best_idx = i;
    }
    if ((*curr)->_pointers.empty()) {
      break;
    }
    current_path.push({curr, i});
    curr = &((*curr)->_pointers[i]);
  }
  if (!best_path.empty()) {
    return btree_iterator(best_path, best_idx);
  }
  return end();
}

template <typename tkey, typename tvalue, comparator<tkey> compare,
          std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_iterator
B_tree<tkey, tvalue, compare, t>::upper_bound(const tkey &key) const {
  std::stack<std::pair<btree_node *const *, size_t>> best_path;
  size_t best_idx = 0;
  std::stack<std::pair<btree_node *const *, size_t>> current_path;
  btree_node *const *curr = &_root;

  while (curr && *curr) {
    size_t i = 0;
    while (i < (*curr)->_keys.size() &&
           !compare_keys(key, (*curr)->_keys[i].first)) {
      i++;
    }

    if (i < (*curr)->_keys.size()) {
      best_path = current_path;
      best_path.push({curr, i});
      best_idx = i;
    }
    if ((*curr)->_pointers.empty()) {
      break;
    }
    current_path.push({curr, i});
    curr = &((*curr)->_pointers[i]);
  }
  if (!best_path.empty()) {
    return btree_const_iterator(best_path, best_idx);
  }
  return end();
}

template <typename tkey, typename tvalue, comparator<tkey> compare,
          std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::contains(const tkey &key) const {
  btree_node *current = _root;

  while (current != nullptr) {

    size_t i = 0;

    while (i < current->_keys.size() &&
           compare_keys(current->_keys[i].first, key)) {
      ++i;
    }

    if (i < current->_keys.size() &&
        !compare_keys(current->_keys[i].first, key) &&
        !compare_keys(key, current->_keys[i].first)) {
      return true;
    }

    if (current->_pointers.empty()) {
      return false;
    }

    current = current->_pointers[i];
  }

  return false;
}

template <typename tkey, typename tvalue, comparator<tkey> compare,
          std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_node *
B_tree<tkey, tvalue, compare, t>::create_node() {
  node_allocator_type node_alloc{_allocator};

  btree_node *node = node_traits::allocate(node_alloc, 1);

  node_traits::construct(node_alloc, node);

  return node;
}

template <typename tkey, typename tvalue, comparator<tkey> compare,
          std::size_t t>
void B_tree<tkey, tvalue, compare, t>::insert_into_node(
    btree_node *node, tree_data_type &&data, btree_node *right_child) {
  auto it = std::lower_bound(
      node->_keys.begin(), node->_keys.end(), data.first,
      [this](const tree_data_type &pair_in_vector, const tkey &key_to_insert) {
        return compare_keys(pair_in_vector.first, key_to_insert);
      });

  auto index = std::distance(node->_keys.begin(), it);

  node->_keys.insert(it, std::move(data));

  if (right_child != nullptr) {
    node->_pointers.insert(node->_pointers.begin() + index + 1, right_child);
  }
}

template <typename tkey, typename tvalue, comparator<tkey> compare,
          std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_node *
B_tree<tkey, tvalue, compare, t>::split_node(btree_node *node,
                                             tree_data_type &median_data) {
  size_t mid_idx = t;

  median_data = std::move(node->_keys[mid_idx]);

  btree_node *right_node = create_node();

  right_node->_keys.assign(
      std::make_move_iterator(node->_keys.begin() + mid_idx + 1),
      std::make_move_iterator(node->_keys.end()));

  if (!node->_pointers.empty()) {
    right_node->_pointers.assign(
        std::make_move_iterator(node->_pointers.begin() + mid_idx + 1),
        std::make_move_iterator(node->_pointers.end()));
    node->_pointers.erase(node->_pointers.begin() + mid_idx + 1,
                          node->_pointers.end());
  }

  node->_keys.erase(node->_keys.begin() + mid_idx, node->_keys.end());

  return right_node;
}

template <typename tkey, typename tvalue, comparator<tkey> compare,
          std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_node *
B_tree<tkey, tvalue, compare, t>::copy_node(const btree_node *other_node) {
  if (!other_node) {
    return nullptr;
  }

  btree_node *new_node = create_node();

  new_node->_keys = other_node->_keys;

  for (const auto *child_ptr : other_node->_pointers) {
    new_node->_pointers.push_back(copy_node(child_ptr));
  }

  return new_node;
}

template <typename tkey, typename tvalue, comparator<tkey> compare,
          std::size_t t>
void B_tree<tkey, tvalue, compare, t>::deallocate_single_node(
    btree_node *node) noexcept {
  if (!node)
    return;
  node_allocator_type node_alloc{_allocator};
  node_traits::destroy(node_alloc, node);
  node_traits::deallocate(node_alloc, node, 1);
}

template <typename tkey, typename tvalue, comparator<tkey> compare,
          std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator
B_tree<tkey, tvalue, compare, t>::erase(const tkey &key) {
  if (_root == nullptr) {
    return end();
  }

  erase_from_node(_root, key);

  if (_root->_keys.empty()) {
    btree_node *tmp = _root;
    if (_root->_pointers.empty()) {
      _root = nullptr;
    } else {
      _root = _root->_pointers[0];
    }
    deallocate_single_node(tmp);
  }

  _size--;

  return end();
}

template <typename tkey, typename tvalue, comparator<tkey> compare,
          std::size_t t>
void B_tree<tkey, tvalue, compare, t>::erase_from_node(btree_node *node,
                                                       const tkey &key) {
  if (!node) {
    return;
  }

  size_t idx = 0;
  while (idx < node->_keys.size() &&
         compare_keys(node->_keys[idx].first, key)) {
    idx++;
  }

  if (idx < node->_keys.size() && !compare_keys(node->_keys[idx].first, key) &&
      !compare_keys(key, node->_keys[idx].first)) {
    if (node->_pointers.empty()) {
      remove_from_leaf(node, idx);
    } else {
      remove_from_non_leaf(node, idx);
    }
  } else {
    if (node->_pointers.empty()) {
      return;
    }

    bool is_last = (idx == node->_keys.size());

    if (node->_pointers[idx]->_keys.size() < t) {
      fill(node, idx);
    }

    if (is_last && idx > node->_keys.size()) {
      erase_from_node(node->_pointers[idx - 1], key);
    } else {
      erase_from_node(node->_pointers[idx], key);
    }
  }
}

template <typename tkey, typename tvalue, comparator<tkey> compare,
          std::size_t t>
void B_tree<tkey, tvalue, compare, t>::remove_from_leaf(btree_node *node,
                                                        size_t idx) {
  node->_keys.erase(node->_keys.begin() + idx);
}

template <typename tkey, typename tvalue, comparator<tkey> compare,
          std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::tree_data_type
B_tree<tkey, tvalue, compare, t>::get_predecessor(btree_node *node,
                                                  size_t idx) {
  btree_node *curr = node->_pointers[idx];
  while (!curr->_pointers.empty()) {
    curr = curr->_pointers.back();
  }
  return curr->_keys.back();
}

template <typename tkey, typename tvalue, comparator<tkey> compare,
          std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::tree_data_type
B_tree<tkey, tvalue, compare, t>::get_successor(btree_node *node, size_t idx) {
  btree_node *curr = node->_pointers[idx + 1];
  while (!curr->_pointers.empty()) {
    curr = curr->_pointers.front();
  }
  return curr->_keys.front();
}

template <typename tkey, typename tvalue, comparator<tkey> compare,
          std::size_t t>
void B_tree<tkey, tvalue, compare, t>::remove_from_non_leaf(btree_node *node,
                                                            size_t idx) {
  tree_data_type k = node->_keys[idx];

  if (node->_pointers[idx]->_keys.size() >= t) {
    tree_data_type pred = get_predecessor(node, idx);
    node->_keys[idx] = pred;
    erase_from_node(node->_pointers[idx], pred.first);
  } else if (node->_pointers[idx + 1]->_keys.size() >= t) {
    tree_data_type succ = get_successor(node, idx);
    node->_keys[idx] = succ;
    erase_from_node(node->_pointers[idx + 1], succ.first);
  } else {
    merge_nodes(node, idx);
    erase_from_node(node->_pointers[idx], k.first);
  }
}
template <typename tkey, typename tvalue, comparator<tkey> compare,
          std::size_t t>
void B_tree<tkey, tvalue, compare, t>::fill(btree_node *node, size_t idx) {
  if (idx != 0 && node->_pointers[idx - 1]->_keys.size() >= t) {
    borrow_from_prev(node, idx);
  } else if (idx != node->_keys.size() &&
             node->_pointers[idx + 1]->_keys.size() >= t) {
    borrow_from_next(node, idx);
  } else {
    if (idx != node->_keys.size()) {
      merge_nodes(node, idx);
    } else {
      merge_nodes(node, idx - 1);
    }
  }
}

template <typename tkey, typename tvalue, comparator<tkey> compare,
          std::size_t t>
void B_tree<tkey, tvalue, compare, t>::borrow_from_prev(btree_node *node,
                                                        size_t idx) {
  btree_node *child = node->_pointers[idx];
  btree_node *sibling = node->_pointers[idx - 1];

  child->_keys.insert(child->_keys.begin(), std::move(node->_keys[idx - 1]));

  if (!child->_pointers.empty()) {
    child->_pointers.insert(child->_pointers.begin(),
                            sibling->_pointers.back());
    sibling->_pointers.pop_back();
  }

  node->_keys[idx - 1] = std::move(sibling->_keys.back());
  sibling->_keys.pop_back();
}

template <typename tkey, typename tvalue, comparator<tkey> compare,
          std::size_t t>
void B_tree<tkey, tvalue, compare, t>::borrow_from_next(btree_node *node,
                                                        size_t idx) {
  btree_node *child = node->_pointers[idx];
  btree_node *sibling = node->_pointers[idx + 1];

  child->_keys.push_back(std::move(node->_keys[idx]));

  if (!child->_pointers.empty()) {
    child->_pointers.push_back(sibling->_pointers.front());
    sibling->_pointers.erase(sibling->_pointers.begin());
  }

  node->_keys[idx] = std::move(sibling->_keys.front());
  sibling->_keys.erase(sibling->_keys.begin());
}

template <typename tkey, typename tvalue, comparator<tkey> compare,
          std::size_t t>
void B_tree<tkey, tvalue, compare, t>::merge_nodes(btree_node *node,
                                                   size_t idx) {
  btree_node *child = node->_pointers[idx];
  btree_node *sibling = node->_pointers[idx + 1];

  child->_keys.push_back(std::move(node->_keys[idx]));

  child->_keys.insert(child->_keys.end(),
                      std::make_move_iterator(sibling->_keys.begin()),
                      std::make_move_iterator(sibling->_keys.end()));

  if (!child->_pointers.empty()) {
    child->_pointers.insert(child->_pointers.end(),
                            std::make_move_iterator(sibling->_pointers.begin()),
                            std::make_move_iterator(sibling->_pointers.end()));
  }

  node->_keys.erase(node->_keys.begin() + idx);
  node->_pointers.erase(node->_pointers.begin() + idx + 1);

  deallocate_single_node(sibling);
}
// endregion lookup implementation

// region modifiers implementation

template <typename tkey, typename tvalue, comparator<tkey> compare,
          std::size_t t>
void B_tree<tkey, tvalue, compare, t>::clear() noexcept {
  destroy_node(_root);
  _root = nullptr;
  _size = 0;
}

template <typename tkey, typename tvalue, comparator<tkey> compare,
          std::size_t t>
std::pair<typename B_tree<tkey, tvalue, compare, t>::btree_iterator, bool>
B_tree<tkey, tvalue, compare, t>::insert(const tree_data_type &data) {
  return insert(tree_data_type(data));
}

template <typename tkey, typename tvalue, comparator<tkey> compare,
          std::size_t t>
std::pair<typename B_tree<tkey, tvalue, compare, t>::btree_iterator, bool>
B_tree<tkey, tvalue, compare, t>::insert(tree_data_type &&data) {
  auto existing_it = find(data.first);
  if (existing_it != end()) {
    return {existing_it, false};
  }

  tkey key_to_return = data.first;

  if (_root == nullptr) {
    _root = create_node();
    _root->_keys.push_back(std::move(data));
    _size++;
    return {find(key_to_return), true};
  }

  std::stack<btree_node *> path;
  btree_node *curr = _root;

  while (curr != nullptr) {
    path.push(curr);
    if (curr->_pointers.empty())
      break;

    auto it =
        std::lower_bound(curr->_keys.begin(), curr->_keys.end(), data.first,
                         [this](const tree_data_type &p, const tkey &k) {
                           return compare_keys(p.first, k);
                         });
    size_t idx = std::distance(curr->_keys.begin(), it);
    curr = curr->_pointers[idx];
  }

  btree_node *node = path.top();
  insert_into_node(node, std::move(data));
  _size++;

  tree_data_type median;
  btree_node *right_child = nullptr;

  while (!path.empty() && node->_keys.size() > maximum_keys_in_node) {
    node = path.top();
    path.pop();

    right_child = split_node(node, median);

    if (path.empty()) {
      btree_node *new_root = create_node();
      new_root->_keys.push_back(std::move(median));
      new_root->_pointers.push_back(node);
      new_root->_pointers.push_back(right_child);
      _root = new_root;
    } else {
      btree_node *parent = path.top();
      insert_into_node(parent, std::move(median), right_child);
      node = parent;
    }
  }

  return {find(key_to_return), true};
}

template <typename tkey, typename tvalue, comparator<tkey> compare,
          std::size_t t>
template <typename... Args>
std::pair<typename B_tree<tkey, tvalue, compare, t>::btree_iterator, bool>
B_tree<tkey, tvalue, compare, t>::emplace(Args &&...args) {
  return insert(tree_data_type(std::forward<Args>(args)...));
}

template <typename tkey, typename tvalue, comparator<tkey> compare,
          std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator
B_tree<tkey, tvalue, compare, t>::insert_or_assign(const tree_data_type &data) {
  auto it = find(data.first);
  if (it != end()) {
    it->second = data.second;
    return it;
  }
  return insert(data).first;
}

template <typename tkey, typename tvalue, comparator<tkey> compare,
          std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator
B_tree<tkey, tvalue, compare, t>::insert_or_assign(tree_data_type &&data) {
  auto it = find(data.first);
  if (it != end()) {
    it->second = std::move(data.second);
    return it;
  }
  return insert(std::move(data)).first;
}

template <typename tkey, typename tvalue, comparator<tkey> compare,
          std::size_t t>
template <typename... Args>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator
B_tree<tkey, tvalue, compare, t>::emplace_or_assign(Args &&...args) {
  tree_data_type data(std::forward<Args>(args)...);
  return insert_or_assign(std::move(data));
}

template <typename tkey, typename tvalue, comparator<tkey> compare,
          std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator
B_tree<tkey, tvalue, compare, t>::erase(btree_iterator pos) {
  if (pos == end()) {
    return end();
  }
  auto next_it = pos;
  ++next_it;
  tkey key = pos->first;
  erase(key);
  return find(next_it != end() ? next_it->first : key);
}

template <typename tkey, typename tvalue, comparator<tkey> compare,
          std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator
B_tree<tkey, tvalue, compare, t>::erase(btree_const_iterator pos) {
  if (pos == cend()) {
    return end();
  }
  tkey key = pos->first;
  erase(key);
  return end();
}

template <typename tkey, typename tvalue, comparator<tkey> compare,
          std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator
B_tree<tkey, tvalue, compare, t>::erase(btree_iterator beg, btree_iterator en) {
  while (beg != en) {
    beg = erase(beg);
  }
  return en;
}

template <typename tkey, typename tvalue, comparator<tkey> compare,
          std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator
B_tree<tkey, tvalue, compare, t>::erase(btree_const_iterator beg,
                                        btree_const_iterator en) {
  boost::container::static_vector<tkey, 100> keys_to_delete;
  for (auto it = beg; it != en; ++it)
    keys_to_delete.push_back(it->first);
  for (const auto &k : keys_to_delete)
    erase(k);
  return end();
}

// endregion modifiers implementation

template <typename tkey, typename tvalue, comparator<tkey> compare,
          std::size_t t>
bool compare_pairs(
    const typename B_tree<tkey, tvalue, compare, t>::tree_data_type &lhs,
    const typename B_tree<tkey, tvalue, compare, t>::tree_data_type &rhs) {
  compare cmp;
  return cmp(lhs.first, rhs.first);
}

template <typename tkey, typename tvalue, comparator<tkey> compare,
          std::size_t t>
bool compare_keys(const tkey &lhs, const tkey &rhs) {
  compare cmp;
  return cmp(lhs, rhs);
}

#endif