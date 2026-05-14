#ifndef SYS_PROG_B_TREE_H
#define SYS_PROG_B_TREE_H

#include <iterator>
#include <utility>
#include <boost/container/static_vector.hpp>
#include <stack>
#include <pp_allocator.h>
#include <associative_container.h>
#include <initializer_list>
#include <stdexcept>
#include <algorithm>

template <typename tkey, typename tvalue, comparator<tkey> compare = std::less<tkey>, std::size_t t = 5>
class B_tree final : private compare // EBCO
{
public:

    using tree_data_type = std::pair<tkey, tvalue>;
    using tree_data_type_const = std::pair<const tkey, tvalue>;
    using value_type = tree_data_type_const;

    // Вложенные классы исключений, согласно условию
    class tree_exception : public std::logic_error {
    public:
        explicit tree_exception(const std::string& msg) : std::logic_error(msg) {}
    };

    // Наследуемся от out_of_range, чтобы удовлетворить и комментарий шаблона, и требование задания (nested types)
    class key_not_found : public std::out_of_range {
    public:
        key_not_found() : std::out_of_range("Requested key is missing in the B-Tree") {}
    };

private:

    static constexpr const size_t minimum_keys_in_node = t - 1;
    static constexpr const size_t maximum_keys_in_node = 2 * t - 1;

    // region comparators declaration

    inline bool compare_keys(const tkey& lhs, const tkey& rhs) const;
    inline bool compare_pairs(const tree_data_type& lhs, const tree_data_type& rhs) const;

    // endregion comparators declaration


    struct btree_node
    {
        boost::container::static_vector<tree_data_type, maximum_keys_in_node + 1> _keys;
        boost::container::static_vector<btree_node*, maximum_keys_in_node + 2> _pointers;
        btree_node() noexcept;
    };

    pp_allocator<value_type> _allocator;
    btree_node* _root;
    size_t _size;

    pp_allocator<value_type> get_allocator() const noexcept;

    using alloc_type_for_node = typename std::allocator_traits<pp_allocator<value_type>>::template rebind_alloc<btree_node>;
    using traits_for_node = std::allocator_traits<alloc_type_for_node>;

    btree_node *allocate_node();
    void clear_nodes_recursively(btree_node *node) noexcept;
    btree_node *duplicate_tree(const btree_node *source);
    void free_single_node(btree_node *node) noexcept;

    void insert_into_target(btree_node *node, tree_data_type &&payload, btree_node *new_right = nullptr);
    btree_node *divide_full_node(btree_node *node, tree_data_type &promoted_item);

    void remove_key_internal(btree_node *node, const tkey &key);
    void delete_in_leaf(btree_node *node, size_t pos);
    void delete_in_branch(btree_node *node, size_t pos);

    tree_data_type fetch_predecessor(btree_node *node, size_t pos);
    tree_data_type fetch_successor(btree_node *node, size_t pos);

    void replenish_keys(btree_node *node, size_t pos);
    void pull_from_left_sibling(btree_node *node, size_t pos);
    void pull_from_right_sibling(btree_node *node, size_t pos);
    void fuse_siblings(btree_node *node, size_t pos);

public:

    // region constructors declaration

    explicit B_tree(const compare& cmp = compare(), pp_allocator<value_type> = pp_allocator<value_type>());

    explicit B_tree(pp_allocator<value_type> alloc, const compare& comp = compare());

    template<input_iterator_for_pair<tkey, tvalue> iterator>
    explicit B_tree(iterator begin, iterator end, const compare& cmp = compare(), pp_allocator<value_type> = pp_allocator<value_type>());

    B_tree(std::initializer_list<std::pair<tkey, tvalue>> data, const compare& cmp = compare(), pp_allocator<value_type> = pp_allocator<value_type>());

    // endregion constructors declaration

    // region five declaration

    B_tree(const B_tree& other);

    B_tree(B_tree&& other) noexcept;

    B_tree& operator=(const B_tree& other);

    B_tree& operator=(B_tree&& other) noexcept;

    ~B_tree() noexcept;

    // endregion five declaration

    // region iterators declaration

    class btree_iterator;
    class btree_reverse_iterator;
    class btree_const_iterator;
    class btree_const_reverse_iterator;

    class btree_iterator final
    {
        std::stack<std::pair<btree_node**, size_t>> _path;
        size_t _index;

    public:
        using value_type = tree_data_type_const;
        using reference = value_type&;
        using pointer = value_type*;
        using iterator_category = std::bidirectional_iterator_tag;
        using difference_type = ptrdiff_t;
        using self = btree_iterator;

        friend class B_tree;
        friend class btree_reverse_iterator;
        friend class btree_const_iterator;
        friend class btree_const_reverse_iterator;

        reference operator*() const noexcept;
        pointer operator->() const noexcept;

        self& operator++();
        self operator++(int);

        self& operator--();
        self operator--(int);

        bool operator==(const self& other) const noexcept;
        bool operator!=(const self& other) const noexcept;

        size_t depth() const noexcept;
        size_t current_node_keys_count() const noexcept;
        bool is_terminate_node() const noexcept;
        size_t index() const noexcept;

        explicit btree_iterator(const std::stack<std::pair<btree_node**, size_t>>& path = std::stack<std::pair<btree_node**, size_t>>(), size_t index = 0);

    };

    class btree_const_iterator final
    {
        std::stack<std::pair<btree_node* const*, size_t>> _path;
        size_t _index;

    public:

        using value_type = tree_data_type_const;
        using reference = const value_type&;
        using pointer = const value_type*;
        using iterator_category = std::bidirectional_iterator_tag;
        using difference_type = ptrdiff_t;
        using self = btree_const_iterator;

        friend class B_tree;
        friend class btree_reverse_iterator;
        friend class btree_iterator;
        friend class btree_const_reverse_iterator;

        btree_const_iterator(const btree_iterator& it) noexcept;

        reference operator*() const noexcept;
        pointer operator->() const noexcept;

        self& operator++();
        self operator++(int);

        self& operator--();
        self operator--(int);

        bool operator==(const self& other) const noexcept;
        bool operator!=(const self& other) const noexcept;

        size_t depth() const noexcept;
        size_t current_node_keys_count() const noexcept;
        bool is_terminate_node() const noexcept;
        size_t index() const noexcept;

        explicit btree_const_iterator(const std::stack<std::pair<btree_node* const*, size_t>>& path = std::stack<std::pair<btree_node* const*, size_t>>(), size_t index = 0);
    };

    class btree_reverse_iterator final
    {
        std::stack<std::pair<btree_node**, size_t>> _path;
        size_t _index;

    public:

        using value_type = tree_data_type_const;
        using reference = value_type&;
        using pointer = value_type*;
        using iterator_category = std::bidirectional_iterator_tag;
        using difference_type = ptrdiff_t;
        using self = btree_reverse_iterator;

        friend class B_tree;
        friend class btree_iterator;
        friend class btree_const_iterator;
        friend class btree_const_reverse_iterator;

        btree_reverse_iterator(const btree_iterator& it) noexcept;
        operator btree_iterator() const noexcept;

        reference operator*() const noexcept;
        pointer operator->() const noexcept;

        self& operator++();
        self operator++(int);

        self& operator--();
        self operator--(int);

        bool operator==(const self& other) const noexcept;
        bool operator!=(const self& other) const noexcept;

        size_t depth() const noexcept;
        size_t current_node_keys_count() const noexcept;
        bool is_terminate_node() const noexcept;
        size_t index() const noexcept;

        explicit btree_reverse_iterator(const std::stack<std::pair<btree_node**, size_t>>& path = std::stack<std::pair<btree_node**, size_t>>(), size_t index = 0);
    };

    class btree_const_reverse_iterator final
    {
        std::stack<std::pair<btree_node* const*, size_t>> _path;
        size_t _index;

    public:

        using value_type = tree_data_type_const;
        using reference = const value_type&;
        using pointer = const value_type*;
        using iterator_category = std::bidirectional_iterator_tag;
        using difference_type = ptrdiff_t;
        using self = btree_const_reverse_iterator;

        friend class B_tree;
        friend class btree_reverse_iterator;
        friend class btree_const_iterator;
        friend class btree_iterator;

        btree_const_reverse_iterator(const btree_reverse_iterator& it) noexcept;
        operator btree_const_iterator() const noexcept;

        reference operator*() const noexcept;
        pointer operator->() const noexcept;

        self& operator++();
        self operator++(int);

        self& operator--();
        self operator--(int);

        bool operator==(const self& other) const noexcept;
        bool operator!=(const self& other) const noexcept;

        size_t depth() const noexcept;
        size_t current_node_keys_count() const noexcept;
        bool is_terminate_node() const noexcept;
        size_t index() const noexcept;

        explicit btree_const_reverse_iterator(const std::stack<std::pair<btree_node* const*, size_t>>& path = std::stack<std::pair<btree_node* const*, size_t>>(), size_t index = 0);
    };

    friend class btree_iterator;
    friend class btree_const_iterator;
    friend class btree_reverse_iterator;
    friend class btree_const_reverse_iterator;

    // endregion iterators declaration

    // region element access declaration

    /*
     * Returns a reference to the mapped value of the element with specified key. If no such element exists, an exception of type std::out_of_range is thrown.
     */
    tvalue& at(const tkey&);
    const tvalue& at(const tkey&) const;

    /*
     * If key not exists, makes default initialization of value
     */
    tvalue& operator[](const tkey& key);
    tvalue& operator[](tkey&& key);

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

    btree_iterator find(const tkey& key);
    btree_const_iterator find(const tkey& key) const;

    btree_iterator lower_bound(const tkey& key);
    btree_const_iterator lower_bound(const tkey& key) const;

    btree_iterator upper_bound(const tkey& key);
    btree_const_iterator upper_bound(const tkey& key) const;

    bool contains(const tkey& key) const;

    // endregion lookup declaration

    // region modifiers declaration

    void clear() noexcept;

    /*
     * Does nothing if key exists, delegates to emplace.
     * Second return value is true, when inserted
     */
    std::pair<btree_iterator, bool> insert(const tree_data_type& data);
    std::pair<btree_iterator, bool> insert(tree_data_type&& data);

    template <typename ...Args>
    std::pair<btree_iterator, bool> emplace(Args&&... args);

    /*
     * Updates value if key exists, delegates to emplace.
     */
    btree_iterator insert_or_assign(const tree_data_type& data);
    btree_iterator insert_or_assign(tree_data_type&& data);

    template <typename ...Args>
    btree_iterator emplace_or_assign(Args&&... args);

    /*
     * Return iterator to node next ro removed or end() if key not exists
     */
    btree_iterator erase(btree_iterator pos);
    btree_iterator erase(btree_const_iterator pos);

    btree_iterator erase(btree_iterator beg, btree_iterator en);
    btree_iterator erase(btree_const_iterator beg, btree_const_iterator en);

    btree_iterator erase(const tkey& key);

    // endregion modifiers declaration
};

template<std::input_iterator iterator, comparator<typename std::iterator_traits<iterator>::value_type::first_type> compare = std::less<typename std::iterator_traits<iterator>::value_type::first_type>,
        std::size_t t = 5, typename U>
B_tree(iterator begin, iterator end, const compare &cmp = compare(), pp_allocator<U> = pp_allocator<U>()) -> B_tree<typename std::iterator_traits<iterator>::value_type::first_type, typename std::iterator_traits<iterator>::value_type::second_type, compare, t>;

template<typename tkey, typename tvalue, comparator<tkey> compare = std::less<tkey>, std::size_t t = 5, typename U>
B_tree(std::initializer_list<std::pair<tkey, tvalue>> data, const compare &cmp = compare(), pp_allocator<U> = pp_allocator<U>()) -> B_tree<tkey, tvalue, compare, t>;


// region helpers and private members implementation

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::compare_pairs(const B_tree::tree_data_type &lhs, const B_tree::tree_data_type &rhs) const
{
    return compare_keys(lhs.first, rhs.first);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::compare_keys(const tkey &lhs, const tkey &rhs) const
{
    return compare::operator()(lhs, rhs);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>::btree_node::btree_node() noexcept {}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
pp_allocator<typename B_tree<tkey, tvalue, compare, t>::value_type> B_tree<tkey, tvalue, compare, t>::get_allocator() const noexcept
{
    return _allocator;
}

template <typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_node* B_tree<tkey, tvalue, compare, t>::allocate_node() {
    alloc_type_for_node node_alloc{_allocator};
    btree_node *new_node = traits_for_node::allocate(node_alloc, 1);
    traits_for_node::construct(node_alloc, new_node);
    return new_node;
}

template <typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
void B_tree<tkey, tvalue, compare, t>::free_single_node(btree_node *node) noexcept {
    if (!node) return;
    alloc_type_for_node node_alloc{_allocator};
    traits_for_node::destroy(node_alloc, node);
    traits_for_node::deallocate(node_alloc, node, 1);
}

template <typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
void B_tree<tkey, tvalue, compare, t>::clear_nodes_recursively(btree_node *node) noexcept {
    if (!node) return;
    for (btree_node *child : node->_pointers) {
        clear_nodes_recursively(child);
    }
    free_single_node(node);
}

template <typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_node* B_tree<tkey, tvalue, compare, t>::duplicate_tree(const btree_node *source) {
    if (!source) return nullptr;
    btree_node *cloned = allocate_node();
    cloned->_keys = source->_keys;
    for (const btree_node *child_ptr : source->_pointers) {
        cloned->_pointers.push_back(duplicate_tree(child_ptr));
    }
    return cloned;
}

template <typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
void B_tree<tkey, tvalue, compare, t>::insert_into_target(btree_node *node, tree_data_type &&payload, btree_node *new_right) {
    auto insertion_pos = std::lower_bound(
        node->_keys.begin(), node->_keys.end(), payload.first,
        [this](const tree_data_type &p, const tkey &k) { return compare_keys(p.first, k); });

    size_t index = std::distance(node->_keys.begin(), insertion_pos);
    node->_keys.insert(insertion_pos, std::move(payload));

    if (new_right) {
        node->_pointers.insert(std::next(node->_pointers.begin(), index + 1), new_right);
    }
}

template <typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_node* B_tree<tkey, tvalue, compare, t>::divide_full_node(btree_node *node, tree_data_type &promoted_item) {
    promoted_item = std::move(node->_keys[t]);
    btree_node *new_sibling = allocate_node();

    new_sibling->_keys.assign(
        std::make_move_iterator(std::next(node->_keys.begin(), t + 1)),
        std::make_move_iterator(node->_keys.end()));

    if (!node->_pointers.empty()) {
        new_sibling->_pointers.assign(
            std::make_move_iterator(std::next(node->_pointers.begin(), t + 1)),
            std::make_move_iterator(node->_pointers.end()));
        node->_pointers.erase(std::next(node->_pointers.begin(), t + 1), node->_pointers.end());
    }

    node->_keys.erase(std::next(node->_keys.begin(), t), node->_keys.end());
    return new_sibling;
}

// endregion helpers and private members implementation


// region constructors implementation

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>::B_tree(
        const compare& cmp,
        pp_allocator<value_type> alloc) : _allocator(alloc), _root(nullptr), _size(0)
{
    *static_cast<compare *>(this) = cmp;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>::B_tree(
        pp_allocator<value_type> alloc,
        const compare& comp) : _allocator(alloc), _root(nullptr), _size(0)
{
    *static_cast<compare *>(this) = comp;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
template<input_iterator_for_pair<tkey, tvalue> iterator>
B_tree<tkey, tvalue, compare, t>::B_tree(
        iterator begin,
        iterator end,
        const compare& cmp,
        pp_allocator<value_type> alloc) : _allocator(alloc), _root(nullptr), _size(0)
{
    *static_cast<compare *>(this) = cmp;
    for (; begin != end; ++begin) {
        insert(*begin);
    }
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>::B_tree(
        std::initializer_list<std::pair<tkey, tvalue>> data,
        const compare& cmp,
        pp_allocator<value_type> alloc) : _allocator(alloc), _root(nullptr), _size(0)
{
    *static_cast<compare *>(this) = cmp;
    for (const auto &item : data) {
        insert(item);
    }
}

// endregion constructors implementation

// region five implementation

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>::~B_tree() noexcept
{
    clear();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>::B_tree(const B_tree& other) : _allocator(other._allocator), _root(nullptr), _size(other._size)
{
    *static_cast<compare *>(this) = static_cast<const compare &>(other);
    if (other._root) {
        _root = duplicate_tree(other._root);
    }
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>& B_tree<tkey, tvalue, compare, t>::operator=(const B_tree& other)
{
    if (this == &other) return *this;
    clear();
    _allocator = other._allocator;
    *static_cast<compare *>(this) = static_cast<const compare &>(other);
    _size = other._size;
    if (other._root) {
        _root = duplicate_tree(other._root);
    }
    return *this;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>::B_tree(B_tree&& other) noexcept : _allocator(std::move(other._allocator)), _root(other._root), _size(other._size)
{
    *static_cast<compare *>(this) = std::move(static_cast<compare &>(other));
    other._root = nullptr;
    other._size = 0;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>& B_tree<tkey, tvalue, compare, t>::operator=(B_tree&& other) noexcept
{
    if (this == &other) return *this;
    clear();
    _allocator = std::move(other._allocator);
    *static_cast<compare *>(this) = std::move(static_cast<compare &>(other));
    _root = other._root;
    _size = other._size;
    other._root = nullptr;
    other._size = 0;
    return *this;
}

// endregion five implementation

// region iterators implementation

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>::btree_iterator::btree_iterator(
        const std::stack<std::pair<btree_node**, size_t>>& path, size_t index) : _path(path), _index(index) {}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator::reference
B_tree<tkey, tvalue, compare, t>::btree_iterator::operator*() const noexcept
{
    return reinterpret_cast<reference>((*(_path.top().first))->_keys[_index]);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator::pointer
B_tree<tkey, tvalue, compare, t>::btree_iterator::operator->() const noexcept
{
    return &((*this).operator*());
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator&
B_tree<tkey, tvalue, compare, t>::btree_iterator::operator++()
{
    if (_path.empty()) return *this;
    btree_node *current_node = *(_path.top().first);

    if (current_node->_pointers.empty()) {
        ++_index;
        while (!_path.empty() && _index >= (*(_path.top().first))->_keys.size()) {
            _path.pop();
            if (!_path.empty()) {
                _index = _path.top().second;
            }
        }
    } else {
        _path.top().second = _index + 1;
        btree_node **next_child = &(current_node->_pointers[_index + 1]);
        _path.push({next_child, 0});
        current_node = *next_child;

        while (!current_node->_pointers.empty()) {
            next_child = &(current_node->_pointers.front());
            _path.push({next_child, 0});
            current_node = *next_child;
        }
        _index = 0;
    }
    return *this;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator
B_tree<tkey, tvalue, compare, t>::btree_iterator::operator++(int)
{
    auto temp = *this;
    ++(*this);
    return temp;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator&
B_tree<tkey, tvalue, compare, t>::btree_iterator::operator--()
{
    if (_path.empty()) return *this;
    btree_node *current_node = *(_path.top().first);

    if (current_node->_pointers.empty()) {
        if (_index > 0) {
            --_index;
        } else {
            while (!_path.empty()) {
                _path.pop();
                if (!_path.empty() && _path.top().second > 0) {
                    _index = _path.top().second - 1;
                    break;
                }
            }
        }
    } else {
        _path.top().second = _index;
        btree_node **prev_child = &(current_node->_pointers[_index]);
        current_node = *prev_child;
        _path.push({prev_child, current_node->_keys.size()});

        while (!current_node->_pointers.empty()) {
            prev_child = &(current_node->_pointers.back());
            current_node = *prev_child;
            _path.push({prev_child, current_node->_keys.size()});
        }
        _index = current_node->_keys.size() - 1;
    }
    return *this;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator
B_tree<tkey, tvalue, compare, t>::btree_iterator::operator--(int)
{
    auto temp = *this;
    --(*this);
    return temp;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::btree_iterator::operator==(const self& other) const noexcept
{
    if (_path.empty() || other._path.empty()) return _path.empty() == other._path.empty();
    return _path.top().first == other._path.top().first && _index == other._index;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::btree_iterator::operator!=(const self& other) const noexcept
{
    return !(*this == other);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
size_t B_tree<tkey, tvalue, compare, t>::btree_iterator::depth() const noexcept
{
    return _path.empty() ? 0 : _path.size() - 1;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
size_t B_tree<tkey, tvalue, compare, t>::btree_iterator::current_node_keys_count() const noexcept
{
    return _path.empty() ? 0 : (*(_path.top().first))->_keys.size();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::btree_iterator::is_terminate_node() const noexcept
{
    return _path.empty();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
size_t B_tree<tkey, tvalue, compare, t>::btree_iterator::index() const noexcept
{
    return _index;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>::btree_const_iterator::btree_const_iterator(
        const std::stack<std::pair<btree_node* const*, size_t>>& path, size_t index) : _path(path), _index(index) {}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>::btree_const_iterator::btree_const_iterator(
        const btree_iterator& it) noexcept : _index(it._index)
{
    auto trace_cp = it._path;
    std::vector<std::pair<btree_node *const *, size_t>> reversed_trace;
    while (!trace_cp.empty()) {
        reversed_trace.push_back({trace_cp.top().first, trace_cp.top().second});
        trace_cp.pop();
    }
    for (auto iter = reversed_trace.rbegin(); iter != reversed_trace.rend(); ++iter) {
        _path.push(*iter);
    }
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_iterator::reference
B_tree<tkey, tvalue, compare, t>::btree_const_iterator::operator*() const noexcept
{
    return reinterpret_cast<reference>((*(_path.top().first))->_keys[_index]);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_iterator::pointer
B_tree<tkey, tvalue, compare, t>::btree_const_iterator::operator->() const noexcept
{
    return &((*this).operator*());
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_iterator&
B_tree<tkey, tvalue, compare, t>::btree_const_iterator::operator++()
{
    if (_path.empty()) return *this;
    const btree_node *current_node = *(_path.top().first);

    if (current_node->_pointers.empty()) {
        ++_index;
        while (!_path.empty() && _index >= (*(_path.top().first))->_keys.size()) {
            _path.pop();
            if (!_path.empty()) _index = _path.top().second;
        }
    } else {
        _path.top().second = _index + 1;
        btree_node *const *next_child = &(current_node->_pointers[_index + 1]);
        _path.push({next_child, 0});
        current_node = *next_child;

        while (!current_node->_pointers.empty()) {
            next_child = &(current_node->_pointers.front());
            _path.push({next_child, 0});
            current_node = *next_child;
        }
        _index = 0;
    }
    return *this;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_iterator
B_tree<tkey, tvalue, compare, t>::btree_const_iterator::operator++(int)
{
    auto temp = *this;
    ++(*this);
    return temp;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_iterator&
B_tree<tkey, tvalue, compare, t>::btree_const_iterator::operator--()
{
    if (_path.empty()) return *this;
    const btree_node *current_node = *(_path.top().first);

    if (current_node->_pointers.empty()) {
        if (_index > 0) {
            --_index;
        } else {
            while (!_path.empty()) {
                _path.pop();
                if (!_path.empty() && _path.top().second > 0) {
                    _index = _path.top().second - 1;
                    break;
                }
            }
        }
    } else {
        _path.top().second = _index;
        btree_node *const *prev_child = &(current_node->_pointers[_index]);
        current_node = *prev_child;
        _path.push({prev_child, current_node->_keys.size()});

        while (!current_node->_pointers.empty()) {
            prev_child = &(current_node->_pointers.back());
            current_node = *prev_child;
            _path.push({prev_child, current_node->_keys.size()});
        }
        _index = current_node->_keys.size() - 1;
    }
    return *this;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_iterator
B_tree<tkey, tvalue, compare, t>::btree_const_iterator::operator--(int)
{
    auto temp = *this;
    --(*this);
    return temp;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::btree_const_iterator::operator==(const self& other) const noexcept
{
    if (_path.empty() || other._path.empty()) return _path.empty() == other._path.empty();
    return _path.top().first == other._path.top().first && _index == other._index;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::btree_const_iterator::operator!=(const self& other) const noexcept
{
    return !(*this == other);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
size_t B_tree<tkey, tvalue, compare, t>::btree_const_iterator::depth() const noexcept
{
    return _path.empty() ? 0 : _path.size() - 1;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
size_t B_tree<tkey, tvalue, compare, t>::btree_const_iterator::current_node_keys_count() const noexcept
{
    return _path.empty() ? 0 : (*(_path.top().first))->_keys.size();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::btree_const_iterator::is_terminate_node() const noexcept
{
    return _path.empty();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
size_t B_tree<tkey, tvalue, compare, t>::btree_const_iterator::index() const noexcept
{
    return _index;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::btree_reverse_iterator(
        const std::stack<std::pair<btree_node**, size_t>>& path, size_t index) : _path(path), _index(index) {}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::btree_reverse_iterator(
        const btree_iterator& it) noexcept : _path(it._path), _index(it._index) {}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::operator B_tree<tkey, tvalue, compare, t>::btree_iterator() const noexcept
{
    return btree_iterator(_path, _index);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::reference
B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::operator*() const noexcept
{
    return reinterpret_cast<reference>((*(_path.top().first))->_keys[_index]);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::pointer
B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::operator->() const noexcept
{
    return &((*this).operator*());
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator&
B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::operator++()
{
    btree_iterator forward_it(_path, _index);
    --forward_it;
    _path = forward_it._path;
    _index = forward_it._index;
    return *this;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator
B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::operator++(int)
{
    auto temp = *this;
    ++(*this);
    return temp;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator&
B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::operator--()
{
    btree_iterator forward_it(_path, _index);
    ++forward_it;
    _path = forward_it._path;
    _index = forward_it._index;
    return *this;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator
B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::operator--(int)
{
    auto temp = *this;
    --(*this);
    return temp;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::operator==(const self& other) const noexcept
{
    if (_path.empty() || other._path.empty()) return _path.empty() == other._path.empty();
    return _path.top().first == other._path.top().first && _index == other._index;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::operator!=(const self& other) const noexcept
{
    return !(*this == other);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
size_t B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::depth() const noexcept
{
    return _path.empty() ? 0 : _path.size() - 1;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
size_t B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::current_node_keys_count() const noexcept
{
    return _path.empty() ? 0 : (*(_path.top().first))->_keys.size();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::is_terminate_node() const noexcept
{
    return _path.empty();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
size_t B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::index() const noexcept
{
    return _index;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::btree_const_reverse_iterator(
        const std::stack<std::pair<btree_node* const*, size_t>>& path, size_t index) : _path(path), _index(index) {}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::btree_const_reverse_iterator(
        const btree_reverse_iterator& it) noexcept : _index(it._index)
{
    auto trace_cp = it._path;
    std::vector<std::pair<btree_node *const *, size_t>> reversed_trace;
    while (!trace_cp.empty()) {
        reversed_trace.push_back({trace_cp.top().first, trace_cp.top().second});
        trace_cp.pop();
    }
    for (auto iter = reversed_trace.rbegin(); iter != reversed_trace.rend(); ++iter) {
        _path.push(*iter);
    }
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::operator B_tree<tkey, tvalue, compare, t>::btree_const_iterator() const noexcept
{
    return btree_const_iterator(_path, _index);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::reference
B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::operator*() const noexcept
{
    return reinterpret_cast<reference>((*(_path.top().first))->_keys[_index]);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::pointer
B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::operator->() const noexcept
{
    return &((*this).operator*());
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator&
B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::operator++()
{
    btree_const_iterator forward_it(_path, _index);
    --forward_it;
    _path = forward_it._path;
    _index = forward_it._index;
    return *this;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator
B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::operator++(int)
{
    auto temp = *this;
    ++(*this);
    return temp;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator&
B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::operator--()
{
    btree_const_iterator forward_it(_path, _index);
    ++forward_it;
    _path = forward_it._path;
    _index = forward_it._index;
    return *this;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator
B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::operator--(int)
{
    auto temp = *this;
    --(*this);
    return temp;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::operator==(const self& other) const noexcept
{
    if (_path.empty() || other._path.empty()) return _path.empty() == other._path.empty();
    return _path.top().first == other._path.top().first && _index == other._index;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::operator!=(const self& other) const noexcept
{
    return !(*this == other);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
size_t B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::depth() const noexcept
{
    return _path.empty() ? 0 : _path.size() - 1;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
size_t B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::current_node_keys_count() const noexcept
{
    return _path.empty() ? 0 : (*(_path.top().first))->_keys.size();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::is_terminate_node() const noexcept
{
    return _path.empty();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
size_t B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::index() const noexcept
{
    return _index;
}

// endregion iterators implementation

// region element access implementation

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
tvalue& B_tree<tkey, tvalue, compare, t>::at(const tkey& key)
{
    auto it = find(key);
    if (it == end()) throw key_not_found();
    return it->second;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
const tvalue& B_tree<tkey, tvalue, compare, t>::at(const tkey& key) const
{
    auto it = find(key);
    if (it == end()) throw key_not_found();
    return it->second;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
tvalue& B_tree<tkey, tvalue, compare, t>::operator[](const tkey& key)
{
    return insert({key, tvalue()}).first->second;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
tvalue& B_tree<tkey, tvalue, compare, t>::operator[](tkey&& key)
{
    return insert({std::move(key), tvalue()}).first->second;
}

// endregion element access implementation

// region iterator begins implementation

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator B_tree<tkey, tvalue, compare, t>::begin()
{
    if (!_root) return end();
    std::stack<std::pair<btree_node **, size_t>> init_path;
    btree_node **current = &_root;

    while (current && *current) {
        init_path.push({current, 0});
        if ((*current)->_pointers.empty()) break;
        current = &((*current)->_pointers.front());
    }
    return btree_iterator(init_path, 0);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator B_tree<tkey, tvalue, compare, t>::end()
{
    return btree_iterator();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_iterator B_tree<tkey, tvalue, compare, t>::begin() const
{
    return cbegin();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_iterator B_tree<tkey, tvalue, compare, t>::end() const
{
    return btree_iterator();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_iterator B_tree<tkey, tvalue, compare, t>::cbegin() const
{
    if (!_root) return cend();
    std::stack<std::pair<btree_node *const *, size_t>> init_path;
    btree_node *const *current = &_root;

    while (current && *current) {
        init_path.push({current, 0});
        if ((*current)->_pointers.empty()) break;
        current = &((*current)->_pointers.front());
    }
    return btree_const_iterator(init_path, 0);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_iterator B_tree<tkey, tvalue, compare, t>::cend() const
{
    return btree_const_iterator();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator B_tree<tkey, tvalue, compare, t>::rbegin()
{
    if (!_root) return rend();
    std::stack<std::pair<btree_node **, size_t>> trace;
    btree_node **current = &_root;
    while (current && *current) {
        trace.push({current, (*current)->_keys.size()});
        if ((*current)->_pointers.empty()) {
            trace.top().second--;
            break;
        }
        current = &((*current)->_pointers.back());
    }
    return btree_reverse_iterator(trace, trace.top().second);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator B_tree<tkey, tvalue, compare, t>::rend()
{
    return btree_reverse_iterator();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator B_tree<tkey, tvalue, compare, t>::rbegin() const
{
    return crbegin();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator B_tree<tkey, tvalue, compare, t>::rend() const
{
    return crend();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator B_tree<tkey, tvalue, compare, t>::crbegin() const
{
    if (!_root) return crend();
    std::stack<std::pair<btree_node *const *, size_t>> trace;
    btree_node *const *current = &_root;
    while (current && *current) {
        trace.push({current, (*current)->_keys.size()});
        if ((*current)->_pointers.empty()) {
            trace.top().second--;
            break;
        }
        current = &((*current)->_pointers.back());
    }
    return btree_const_reverse_iterator(trace, trace.top().second);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator B_tree<tkey, tvalue, compare, t>::crend() const
{
    return btree_const_reverse_iterator();
}

// endregion iterator begins implementation

// region lookup implementation

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
size_t B_tree<tkey, tvalue, compare, t>::size() const noexcept
{
    return _size;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::empty() const noexcept
{
    return _size == 0;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator B_tree<tkey, tvalue, compare, t>::find(const tkey& key)
{
    std::stack<std::pair<btree_node **, size_t>> trace;
    btree_node **current = &_root;

    while (*current) {
        size_t key_index = 0;
        const auto &items = (*current)->_keys;
        
        for (; key_index < items.size(); ++key_index) {
            if (!compare_keys(items[key_index].first, key)) break;
        }

        if (key_index < items.size() && !compare_keys(items[key_index].first, key) && !compare_keys(key, items[key_index].first)) {
            trace.push({current, key_index});
            return btree_iterator(trace, key_index);
        }
        if ((*current)->_pointers.empty()) return end();

        trace.push({current, key_index});
        current = &((*current)->_pointers[key_index]);
    }
    return end();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_iterator B_tree<tkey, tvalue, compare, t>::find(const tkey& key) const
{
    std::stack<std::pair<btree_node *const *, size_t>> trace;
    btree_node *const *current = &_root;

    while (*current) {
        size_t key_index = 0;
        const auto &items = (*current)->_keys;
        
        for (; key_index < items.size(); ++key_index) {
            if (!compare_keys(items[key_index].first, key)) break;
        }

        if (key_index < items.size() && !compare_keys(items[key_index].first, key) && !compare_keys(key, items[key_index].first)) {
            trace.push({current, key_index});
            return btree_const_iterator(trace, key_index);
        }
        if ((*current)->_pointers.empty()) return end();

        trace.push({current, key_index});
        current = &((*current)->_pointers[key_index]);
    }
    return end();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator B_tree<tkey, tvalue, compare, t>::lower_bound(const tkey& key)
{
    std::stack<std::pair<btree_node **, size_t>> optimal_path;
    size_t optimal_index = 0;
    std::stack<std::pair<btree_node **, size_t>> current_path;
    btree_node **current = &_root;

    while (current && *current) {
        size_t pos = 0;
        const auto &items = (*current)->_keys;
        for (; pos < items.size(); ++pos) {
            if (!compare_keys(items[pos].first, key)) break;
        }

        if (pos < items.size()) {
            optimal_path = current_path;
            optimal_path.push({current, pos});
            optimal_index = pos;
        }
        if ((*current)->_pointers.empty()) break;
        
        current_path.push({current, pos});
        current = &((*current)->_pointers[pos]);
    }
    return optimal_path.empty() ? end() : btree_iterator(optimal_path, optimal_index);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_iterator B_tree<tkey, tvalue, compare, t>::lower_bound(const tkey& key) const
{
    std::stack<std::pair<btree_node *const *, size_t>> optimal_path;
    size_t optimal_index = 0;
    std::stack<std::pair<btree_node *const *, size_t>> current_path;
    btree_node *const *current = &_root;

    while (current && *current) {
        size_t pos = 0;
        const auto &items = (*current)->_keys;
        for (; pos < items.size(); ++pos) {
            if (!compare_keys(items[pos].first, key)) break;
        }

        if (pos < items.size()) {
            optimal_path = current_path;
            optimal_path.push({current, pos});
            optimal_index = pos;
        }
        if ((*current)->_pointers.empty()) break;
        
        current_path.push({current, pos});
        current = &((*current)->_pointers[pos]);
    }
    return optimal_path.empty() ? end() : btree_const_iterator(optimal_path, optimal_index);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator B_tree<tkey, tvalue, compare, t>::upper_bound(const tkey& key)
{
    std::stack<std::pair<btree_node **, size_t>> optimal_path;
    size_t optimal_index = 0;
    std::stack<std::pair<btree_node **, size_t>> current_path;
    btree_node **current = &_root;

    while (current && *current) {
        size_t pos = 0;
        const auto &items = (*current)->_keys;
        for (; pos < items.size(); ++pos) {
            if (compare_keys(key, items[pos].first)) break;
        }

        if (pos < items.size()) {
            optimal_path = current_path;
            optimal_path.push({current, pos});
            optimal_index = pos;
        }
        if ((*current)->_pointers.empty()) break;
        
        current_path.push({current, pos});
        current = &((*current)->_pointers[pos]);
    }
    return optimal_path.empty() ? end() : btree_iterator(optimal_path, optimal_index);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_iterator B_tree<tkey, tvalue, compare, t>::upper_bound(const tkey& key) const
{
    std::stack<std::pair<btree_node *const *, size_t>> optimal_path;
    size_t optimal_index = 0;
    std::stack<std::pair<btree_node *const *, size_t>> current_path;
    btree_node *const *current = &_root;

    while (current && *current) {
        size_t pos = 0;
        const auto &items = (*current)->_keys;
        for (; pos < items.size(); ++pos) {
            if (compare_keys(key, items[pos].first)) break;
        }

        if (pos < items.size()) {
            optimal_path = current_path;
            optimal_path.push({current, pos});
            optimal_index = pos;
        }
        if ((*current)->_pointers.empty()) break;
        
        current_path.push({current, pos});
        current = &((*current)->_pointers[pos]);
    }
    return optimal_path.empty() ? end() : btree_const_iterator(optimal_path, optimal_index);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::contains(const tkey& key) const
{
    btree_node *current = _root;
    while (current) {
        size_t pos = 0;
        for (; pos < current->_keys.size(); ++pos) {
            if (!compare_keys(current->_keys[pos].first, key)) break;
        }

        if (pos < current->_keys.size() && !compare_keys(current->_keys[pos].first, key) && !compare_keys(key, current->_keys[pos].first)) {
            return true;
        }
        if (current->_pointers.empty()) return false;
        current = current->_pointers[pos];
    }
    return false;
}

// endregion lookup implementation

// region modifiers implementation

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
void B_tree<tkey, tvalue, compare, t>::clear() noexcept
{
    clear_nodes_recursively(_root);
    _root = nullptr;
    _size = 0;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
std::pair<typename B_tree<tkey, tvalue, compare, t>::btree_iterator, bool>
B_tree<tkey, tvalue, compare, t>::insert(const tree_data_type& data)
{
    return insert(tree_data_type(data));
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
std::pair<typename B_tree<tkey, tvalue, compare, t>::btree_iterator, bool>
B_tree<tkey, tvalue, compare, t>::insert(tree_data_type&& data)
{
    if (auto existing_pos = find(data.first); existing_pos != end()) {
        return std::make_pair(existing_pos, false);
    }

    tkey inserted_key = data.first;

    if (!_root) {
        _root = allocate_node();
        _root->_keys.push_back(std::move(data));
        ++_size;
        return std::make_pair(find(inserted_key), true);
    }

    std::stack<btree_node *> descent_path;
    btree_node *node_ptr = _root;

    while (node_ptr) {
        descent_path.push(node_ptr);
        if (node_ptr->_pointers.empty()) break;
        
        auto loc = std::lower_bound(node_ptr->_keys.begin(), node_ptr->_keys.end(), data.first,
                         [this](const tree_data_type &p, const tkey &k) { return compare_keys(p.first, k); });
        node_ptr = node_ptr->_pointers[std::distance(node_ptr->_keys.begin(), loc)];
    }

    btree_node *current_target = descent_path.top();
    insert_into_target(current_target, std::move(data));
    ++_size;

    tree_data_type promoted_elem;
    btree_node *new_right = nullptr;

    while (!descent_path.empty() && current_target->_keys.size() > maximum_keys_in_node) {
        current_target = descent_path.top();
        descent_path.pop();

        new_right = divide_full_node(current_target, promoted_elem);

        if (descent_path.empty()) {
            btree_node *fresh_root = allocate_node();
            fresh_root->_keys.push_back(std::move(promoted_elem));
            fresh_root->_pointers.push_back(current_target);
            fresh_root->_pointers.push_back(new_right);
            _root = fresh_root;
        } else {
            btree_node *parent = descent_path.top();
            insert_into_target(parent, std::move(promoted_elem), new_right);
            current_target = parent;
        }
    }

    return std::make_pair(find(inserted_key), true);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
template <typename ...Args>
std::pair<typename B_tree<tkey, tvalue, compare, t>::btree_iterator, bool>
B_tree<tkey, tvalue, compare, t>::emplace(Args&&... args)
{
    return insert(tree_data_type(std::forward<Args>(args)...));
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator
B_tree<tkey, tvalue, compare, t>::insert_or_assign(const tree_data_type& data)
{
    if (auto it = find(data.first); it != end()) {
        it->second = data.second;
        return it;
    }
    return insert(data).first;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator
B_tree<tkey, tvalue, compare, t>::insert_or_assign(tree_data_type&& data)
{
    if (auto it = find(data.first); it != end()) {
        it->second = std::move(data.second);
        return it;
    }
    return insert(std::move(data)).first;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
template <typename ...Args>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator
B_tree<tkey, tvalue, compare, t>::emplace_or_assign(Args&&... args)
{
    return insert_or_assign(tree_data_type(std::forward<Args>(args)...));
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator
B_tree<tkey, tvalue, compare, t>::erase(btree_iterator pos)
{
    if (pos == end()) return end();
    auto successor = pos;
    ++successor;
    tkey target = pos->first;
    erase(target);
    return find(successor != end() ? successor->first : target);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator
B_tree<tkey, tvalue, compare, t>::erase(btree_const_iterator pos)
{
    if (pos == cend()) return end();
    erase(pos->first);
    return end();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator
B_tree<tkey, tvalue, compare, t>::erase(btree_iterator beg, btree_iterator en)
{
    while (beg != en) {
        beg = erase(beg);
    }
    return en;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator
B_tree<tkey, tvalue, compare, t>::erase(btree_const_iterator beg, btree_const_iterator en)
{
    boost::container::static_vector<tkey, 100> targets_to_erase;
    for (auto it = beg; it != en; ++it) {
        targets_to_erase.push_back(it->first);
    }
    for (const auto &k : targets_to_erase) {
        erase(k);
    }
    return end();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator
B_tree<tkey, tvalue, compare, t>::erase(const tkey& key)
{
    if (!_root) return end();
    remove_key_internal(_root, key);

    if (_root->_keys.empty()) {
        btree_node *old_root = _root;
        _root = _root->_pointers.empty() ? nullptr : _root->_pointers.front();
        free_single_node(old_root);
    }
    --_size;
    return end();
}

template <typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
void B_tree<tkey, tvalue, compare, t>::remove_key_internal(btree_node *node, const tkey &key) {
    if (!node) return;

    size_t pos = 0;
    for (; pos < node->_keys.size(); ++pos) {
        if (!compare_keys(node->_keys[pos].first, key)) break;
    }

    if (pos < node->_keys.size() && !compare_keys(node->_keys[pos].first, key) && !compare_keys(key, node->_keys[pos].first)) {
        if (node->_pointers.empty()) {
            delete_in_leaf(node, pos);
        } else {
            delete_in_branch(node, pos);
        }
    } else {
        if (node->_pointers.empty()) return;
        bool checked_last = (pos == node->_keys.size());

        if (node->_pointers[pos]->_keys.size() < t) {
            replenish_keys(node, pos);
        }

        if (checked_last && pos > node->_keys.size()) {
            remove_key_internal(node->_pointers[pos - 1], key);
        } else {
            remove_key_internal(node->_pointers[pos], key);
        }
    }
}

template <typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
void B_tree<tkey, tvalue, compare, t>::delete_in_leaf(btree_node *node, size_t pos) {
    node->_keys.erase(std::next(node->_keys.begin(), pos));
}

template <typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
void B_tree<tkey, tvalue, compare, t>::delete_in_branch(btree_node *node, size_t pos) {
    tree_data_type target_item = node->_keys[pos];

    if (node->_pointers[pos]->_keys.size() >= t) {
        tree_data_type pre_item = fetch_predecessor(node, pos);
        node->_keys[pos] = pre_item;
        remove_key_internal(node->_pointers[pos], pre_item.first);
    } else if (node->_pointers[pos + 1]->_keys.size() >= t) {
        tree_data_type suc_item = fetch_successor(node, pos);
        node->_keys[pos] = suc_item;
        remove_key_internal(node->_pointers[pos + 1], suc_item.first);
    } else {
        fuse_siblings(node, pos);
        remove_key_internal(node->_pointers[pos], target_item.first);
    }
}

template <typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::tree_data_type B_tree<tkey, tvalue, compare, t>::fetch_predecessor(btree_node *node, size_t pos) {
    btree_node *trace_node = node->_pointers[pos];
    while (!trace_node->_pointers.empty()) {
        trace_node = trace_node->_pointers.back();
    }
    return trace_node->_keys.back();
}

template <typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::tree_data_type B_tree<tkey, tvalue, compare, t>::fetch_successor(btree_node *node, size_t pos) {
    btree_node *trace_node = node->_pointers[pos + 1];
    while (!trace_node->_pointers.empty()) {
        trace_node = trace_node->_pointers.front();
    }
    return trace_node->_keys.front();
}

template <typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
void B_tree<tkey, tvalue, compare, t>::replenish_keys(btree_node *node, size_t pos) {
    if (pos > 0 && node->_pointers[pos - 1]->_keys.size() >= t) {
        pull_from_left_sibling(node, pos);
    } else if (pos < node->_keys.size() && node->_pointers[pos + 1]->_keys.size() >= t) {
        pull_from_right_sibling(node, pos);
    } else {
        if (pos < node->_keys.size()) {
            fuse_siblings(node, pos);
        } else {
            fuse_siblings(node, pos - 1);
        }
    }
}

template <typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
void B_tree<tkey, tvalue, compare, t>::pull_from_left_sibling(btree_node *node, size_t pos) {
    btree_node *target_child = node->_pointers[pos];
    btree_node *left_bro = node->_pointers[pos - 1];

    target_child->_keys.insert(target_child->_keys.begin(), std::move(node->_keys[pos - 1]));

    if (!target_child->_pointers.empty()) {
        target_child->_pointers.insert(target_child->_pointers.begin(), left_bro->_pointers.back());
        left_bro->_pointers.pop_back();
    }

    node->_keys[pos - 1] = std::move(left_bro->_keys.back());
    left_bro->_keys.pop_back();
}

template <typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
void B_tree<tkey, tvalue, compare, t>::pull_from_right_sibling(btree_node *node, size_t pos) {
    btree_node *target_child = node->_pointers[pos];
    btree_node *right_bro = node->_pointers[pos + 1];

    target_child->_keys.push_back(std::move(node->_keys[pos]));

    if (!target_child->_pointers.empty()) {
        target_child->_pointers.push_back(right_bro->_pointers.front());
        right_bro->_pointers.erase(right_bro->_pointers.begin());
    }

    node->_keys[pos] = std::move(right_bro->_keys.front());
    right_bro->_keys.erase(right_bro->_keys.begin());
}

template <typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
void B_tree<tkey, tvalue, compare, t>::fuse_siblings(btree_node *node, size_t pos) {
    btree_node *main_child = node->_pointers[pos];
    btree_node *right_child = node->_pointers[pos + 1];

    main_child->_keys.push_back(std::move(node->_keys[pos]));

    main_child->_keys.insert(main_child->_keys.end(),
        std::make_move_iterator(right_child->_keys.begin()),
        std::make_move_iterator(right_child->_keys.end()));

    if (!main_child->_pointers.empty()) {
        main_child->_pointers.insert(main_child->_pointers.end(),
            std::make_move_iterator(right_child->_pointers.begin()),
            std::make_move_iterator(right_child->_pointers.end()));
    }

    node->_keys.erase(std::next(node->_keys.begin(), pos));
    node->_pointers.erase(std::next(node->_pointers.begin(), pos + 1));

    free_single_node(right_child);
}

// endregion modifiers implementation

#endif