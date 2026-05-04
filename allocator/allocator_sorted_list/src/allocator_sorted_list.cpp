#include <not_implemented.h>
#include "../include/allocator_sorted_list.h"
#include <new>

allocator_sorted_list::~allocator_sorted_list()
{
    if (_trusted_memory == nullptr) return;

    char* ptr = reinterpret_cast<char*>(_trusted_memory);

    auto parent_allocator = *reinterpret_cast<std::pmr::memory_resource**>(ptr);
    ptr += sizeof(std::pmr::memory_resource*);

    ptr += sizeof(allocator_with_fit_mode::fit_mode);

    auto space_size = *reinterpret_cast<size_t*>(ptr);
    ptr += sizeof(size_t);

    auto mut = reinterpret_cast<std::mutex*>(ptr);
    mut->~mutex();

    if (parent_allocator != nullptr){
        parent_allocator->deallocate(_trusted_memory, space_size);
    } else {
        ::operator delete(_trusted_memory, space_size);
    }
}

allocator_sorted_list::allocator_sorted_list(
    allocator_sorted_list &&other) noexcept
    : _trusted_memory(other._trusted_memory)
{
    other._trusted_memory = nullptr;
}

allocator_sorted_list &allocator_sorted_list::operator=(
    allocator_sorted_list &&other) noexcept
{
    if (this == &other) return *this;
    this->~allocator_sorted_list();

    _trusted_memory = other._trusted_memory; // освобождаем старый ресурс и забираем новый, чтобы не было утечки памяти
    other._trusted_memory = nullptr;
    return *this;
}

allocator_sorted_list::allocator_sorted_list(
        size_t space_size,
        std::pmr::memory_resource *parent_allocator,
        allocator_with_fit_mode::fit_mode allocate_fit_mode)
{
    size_t min_space_size = allocator_metadata_size + block_metadata_size;
    if (space_size < min_space_size) 
    {
        throw std::bad_alloc();
    }

    if (parent_allocator != nullptr)
    {
        _trusted_memory = parent_allocator->allocate(space_size);
    }
    else
    {
        _trusted_memory = ::operator new(space_size);
    }

    char* ptr = reinterpret_cast<char*>(_trusted_memory);

    *reinterpret_cast<std::pmr::memory_resource**>(ptr) = parent_allocator; // указатель на род. аллокатор
    ptr += sizeof(std::pmr::memory_resource*);

    *reinterpret_cast<allocator_with_fit_mode::fit_mode*>(ptr) = allocate_fit_mode; // стратегия выделения памяти
    ptr += sizeof(allocator_with_fit_mode::fit_mode);

    *reinterpret_cast<size_t*>(ptr) = space_size; // размер выделенной памяти
    ptr += sizeof(size_t);

    new (ptr) std::mutex(); // мьютекс для синхронизации доступа к аллокатору
    ptr += sizeof(std::mutex);

    auto first_free_ptr = reinterpret_cast<void**>(ptr);
    ptr += sizeof(void*);

    *first_free_ptr = ptr; // указатель на первый свободный блок, который идет сразу за метаданными аллокатора

    auto block_next = reinterpret_cast<void**>(ptr);
    *block_next = nullptr; 
    ptr += sizeof(void*);

    auto block_size = reinterpret_cast<size_t*>(ptr);
    *block_size = space_size - allocator_metadata_size - block_metadata_size;
}

[[nodiscard]] void *allocator_sorted_list::do_allocate_sm(
    size_t size)
{
    if (_trusted_memory == nullptr) throw std::bad_alloc();

    char* base = reinterpret_cast<char*>(_trusted_memory);
    auto mut = reinterpret_cast<std::mutex*>(base + sizeof(std::pmr::memory_resource*) + sizeof(allocator_with_fit_mode::fit_mode) + sizeof(size_t));
    std::lock_guard<std::mutex> lock(*mut);

    auto mode = *reinterpret_cast<allocator_with_fit_mode::fit_mode*>(base + sizeof(std::pmr::memory_resource*));
    void** first_free_ptr_loc = reinterpret_cast<void**>(base + allocator_metadata_size - sizeof(void*));

    void* current = *first_free_ptr_loc;
    void* prev = nullptr;

    void* target = nullptr;
    void* target_prev = nullptr;
    size_t target_size = 0;

    while (current != nullptr)
    {
        size_t current_size = *reinterpret_cast<size_t*>(reinterpret_cast<char*>(current) + sizeof(void*));

        if (current_size >= size)
        {
            if (mode == allocator_with_fit_mode::fit_mode::first_fit) {
                target = current; target_prev = prev; target_size = current_size;
                break;
            } else if (mode == allocator_with_fit_mode::fit_mode::the_best_fit) {
                if (target == nullptr || current_size < target_size) {
                    target = current; target_prev = prev; target_size = current_size;
                }
            } else if (mode == allocator_with_fit_mode::fit_mode::the_worst_fit) {
                if (target == nullptr || current_size > target_size) {
                    target = current; target_prev = prev; target_size = current_size;
                }
            }
        }
        prev = current;
        current = *reinterpret_cast<void**>(current);
    }

    if (target == nullptr) throw std::bad_alloc();

    if (target_size - size >= block_metadata_size)
    {
        size_t remainder_size = target_size - size - block_metadata_size;
        *reinterpret_cast<size_t*>(reinterpret_cast<char*>(target) + sizeof(void*)) = size;

        void* new_free_block = reinterpret_cast<char*>(target) + block_metadata_size + size;
        *reinterpret_cast<void**>(new_free_block) = *reinterpret_cast<void**>(target);
        *reinterpret_cast<size_t*>(reinterpret_cast<char*>(new_free_block) + sizeof(void*)) = remainder_size;

        if (target_prev == nullptr) {
            *first_free_ptr_loc = new_free_block;
        } else {
            *reinterpret_cast<void**>(target_prev) = new_free_block;
        }
    }
    else
    {
        if (target_prev == nullptr) {
            *first_free_ptr_loc = *reinterpret_cast<void**>(target);
        } else {
            *reinterpret_cast<void**>(target_prev) = *reinterpret_cast<void**>(target);
        }
    }

    return reinterpret_cast<char*>(target) + block_metadata_size;
}

allocator_sorted_list::allocator_sorted_list(const allocator_sorted_list &other)
{
    throw std::logic_error("allocator_sorted_list: copying is forbidden");
}

allocator_sorted_list &allocator_sorted_list::operator=(const allocator_sorted_list &other)
{
    throw std::logic_error("allocator_sorted_list: copying is forbidden");
}

bool allocator_sorted_list::do_is_equal(const std::pmr::memory_resource &other) const noexcept
{
    auto other_ptr = dynamic_cast<allocator_sorted_list const *>(&other);
    if (other_ptr == nullptr) return false;
    
    return this->_trusted_memory == other_ptr->_trusted_memory;
}

void allocator_sorted_list::do_deallocate_sm(
    void *at)
{
    if (at == nullptr || _trusted_memory == nullptr) return;

    char* base = reinterpret_cast<char*>(_trusted_memory);
    auto mut = reinterpret_cast<std::mutex*>(base + sizeof(std::pmr::memory_resource*) + sizeof(allocator_with_fit_mode::fit_mode) + sizeof(size_t));
    std::lock_guard<std::mutex> lock(*mut);

    // Адрес физического начала возвращаемого блока
    char* target_block = reinterpret_cast<char*>(at) - block_metadata_size;
    size_t target_size = *reinterpret_cast<size_t*>(target_block + sizeof(void*));

    // Проверка принадлежности блока к нашему аллокатору
    size_t total_space = *reinterpret_cast<size_t*>(base + sizeof(std::pmr::memory_resource*) + sizeof(allocator_with_fit_mode::fit_mode));
    if (target_block < base || target_block >= base + total_space) {
        throw std::logic_error("Attempt to deallocate memory not belonging to this allocator");
    }

    void** first_free_ptr_loc = reinterpret_cast<void**>(base + allocator_metadata_size - sizeof(void*));
    void* current = *first_free_ptr_loc;
    void* prev = nullptr;

    // Ищем место для вставки (сохраняем сортировку адресов)
    while (current != nullptr && current < target_block) {
        prev = current;
        current = *reinterpret_cast<void**>(current);
    }

    // 1. Склеиваем right
    bool merged_right = false;
    if (current != nullptr) {
        char* target_end = target_block + block_metadata_size + target_size;
        if (target_end == reinterpret_cast<char*>(current)) {
            size_t current_size = *reinterpret_cast<size_t*>(reinterpret_cast<char*>(current) + sizeof(void*));
            target_size += block_metadata_size + current_size;
            *reinterpret_cast<size_t*>(target_block + sizeof(void*)) = target_size;
            
            *reinterpret_cast<void**>(target_block) = *reinterpret_cast<void**>(current);
            merged_right = true;
        }
    }
    
    if (!merged_right) {
        *reinterpret_cast<void**>(target_block) = current;
    }

    // 2. Склеиваем left
    if (prev != nullptr) {
        size_t prev_size = *reinterpret_cast<size_t*>(reinterpret_cast<char*>(prev) + sizeof(void*));
        char* prev_end = reinterpret_cast<char*>(prev) + block_metadata_size + prev_size;
        
        if (prev_end == target_block) {
            prev_size += block_metadata_size + target_size;
            *reinterpret_cast<size_t*>(reinterpret_cast<char*>(prev) + sizeof(void*)) = prev_size;
            
            *reinterpret_cast<void**>(prev) = *reinterpret_cast<void**>(target_block);
        } else {
            *reinterpret_cast<void**>(prev) = target_block;
        }
    } else {
        // Если левого нет, наш блок становится новым началом списка
        *first_free_ptr_loc = target_block;
    }
}

inline void allocator_sorted_list::set_fit_mode(
    allocator_with_fit_mode::fit_mode mode)
{
    if (_trusted_memory == nullptr) return;
    
    char* base = reinterpret_cast<char*>(_trusted_memory);
    auto mut = reinterpret_cast<std::mutex*>(base + sizeof(std::pmr::memory_resource*) + sizeof(allocator_with_fit_mode::fit_mode) + sizeof(size_t));
    std::lock_guard<std::mutex> lock(*mut);

    auto mode_ptr = reinterpret_cast<allocator_with_fit_mode::fit_mode*>(base + sizeof(std::pmr::memory_resource*));
    *mode_ptr = mode;
}

std::vector<allocator_test_utils::block_info> allocator_sorted_list::get_blocks_info() const noexcept
{
    try 
    {
        if (_trusted_memory == nullptr) return {};

        char* base = reinterpret_cast<char*>(_trusted_memory);
        auto mut = reinterpret_cast<std::mutex*>(base + sizeof(std::pmr::memory_resource*) + sizeof(allocator_with_fit_mode::fit_mode) + sizeof(size_t));
        std::lock_guard<std::mutex> lock(*mut);

        return get_blocks_info_inner();
    } 
    catch (...) 
    {
        return {};
    }
}


std::vector<allocator_test_utils::block_info> allocator_sorted_list::get_blocks_info_inner() const
{
    std::vector<allocator_test_utils::block_info> info;

    for (auto it = begin(); it != end(); ++it) 
    {
        info.push_back({it.size(), it.occupied()});
    }
    
    return info;
}

allocator_sorted_list::sorted_free_iterator allocator_sorted_list::free_begin() const noexcept
{
    if (_trusted_memory == nullptr) return sorted_free_iterator();
    char *ptr = reinterpret_cast<char*>(_trusted_memory) + allocator_metadata_size - sizeof(void*);
    void* first_free = *reinterpret_cast<void**>(ptr);
    return sorted_free_iterator(first_free);

}

allocator_sorted_list::sorted_free_iterator allocator_sorted_list::free_end() const noexcept
{
    return sorted_free_iterator(nullptr);
}

allocator_sorted_list::sorted_iterator allocator_sorted_list::begin() const noexcept
{
    return sorted_iterator(_trusted_memory);
}

allocator_sorted_list::sorted_iterator allocator_sorted_list::end() const noexcept
{
    return sorted_iterator();
}


bool allocator_sorted_list::sorted_free_iterator::operator==(
        const allocator_sorted_list::sorted_free_iterator & other) const noexcept
{
    return _free_ptr == other._free_ptr;
}

bool allocator_sorted_list::sorted_free_iterator::operator!=(
        const allocator_sorted_list::sorted_free_iterator &other) const noexcept
{
    return _free_ptr != other._free_ptr;
}

allocator_sorted_list::sorted_free_iterator &allocator_sorted_list::sorted_free_iterator::operator++() & noexcept
{
    if (_free_ptr != nullptr){
        _free_ptr = *reinterpret_cast<void**>(_free_ptr);
    }

    return *this;
}

allocator_sorted_list::sorted_free_iterator allocator_sorted_list::sorted_free_iterator::operator++(int n)
{
    auto temp = *this;
    ++(*this);
    return temp;
}

size_t allocator_sorted_list::sorted_free_iterator::size() const noexcept
{
    if (_free_ptr == nullptr) return 0;
    char* ptr = reinterpret_cast<char*>(_free_ptr) + sizeof(void*);
    return *reinterpret_cast<size_t*>(ptr);
}

void *allocator_sorted_list::sorted_free_iterator::operator*() const noexcept
{
    if (_free_ptr == nullptr) return nullptr;
    return reinterpret_cast<char*>(_free_ptr) + allocator_sorted_list::block_metadata_size;
}

allocator_sorted_list::sorted_free_iterator::sorted_free_iterator() : _free_ptr(nullptr) {}

allocator_sorted_list::sorted_free_iterator::sorted_free_iterator(void *trusted) : _free_ptr(trusted) {}

bool allocator_sorted_list::sorted_iterator::operator==(const allocator_sorted_list::sorted_iterator & other) const noexcept
{
    return _current_ptr == other._current_ptr;
}

bool allocator_sorted_list::sorted_iterator::operator!=(const allocator_sorted_list::sorted_iterator &other) const noexcept
{
    return _current_ptr != other._current_ptr;
}

allocator_sorted_list::sorted_iterator &allocator_sorted_list::sorted_iterator::operator++() & noexcept
{
    if (_current_ptr != nullptr) {
        size_t current_block_size = size();

        if (!occupied() && _free_ptr != nullptr) {
            _free_ptr = *reinterpret_cast<void**>(_free_ptr);
        }

        _current_ptr = reinterpret_cast<char*>(_current_ptr) + allocator_sorted_list::block_metadata_size + current_block_size;
        char* base = reinterpret_cast<char*>(_trusted_memory);
        size_t total_space = *reinterpret_cast<size_t*>(base + sizeof(std::pmr::memory_resource*) + sizeof(allocator_with_fit_mode::fit_mode));

        if (reinterpret_cast<char*>(_current_ptr) >= base + total_space) {
            _current_ptr = nullptr; 
        }
    }

    return *this;
}

allocator_sorted_list::sorted_iterator allocator_sorted_list::sorted_iterator::operator++(int n)
{
    auto temp = *this;
    ++(*this);
    return temp;
}

size_t allocator_sorted_list::sorted_iterator::size() const noexcept
{
    if (_current_ptr == nullptr) return 0;
    char* ptr = reinterpret_cast<char*>(_current_ptr) + sizeof(void*);
    return *reinterpret_cast<size_t*>(ptr);
}

void *allocator_sorted_list::sorted_iterator::operator*() const noexcept
{
   if (_current_ptr == nullptr) return nullptr;
    return reinterpret_cast<char*>(_current_ptr) + block_metadata_size;
}

allocator_sorted_list::sorted_iterator::sorted_iterator() : _free_ptr(nullptr), _current_ptr(nullptr), _trusted_memory(nullptr) {}

allocator_sorted_list::sorted_iterator::sorted_iterator(void *trusted)
{
    _trusted_memory = trusted;
    if (_trusted_memory != nullptr) {
        _current_ptr = reinterpret_cast<char*>(_trusted_memory) + allocator_metadata_size; // Первый физ. блок
        
        char* first_free_ptr_loc = reinterpret_cast<char*>(_trusted_memory) + allocator_metadata_size - sizeof(void*);
        _free_ptr = *reinterpret_cast<void**>(first_free_ptr_loc);
    } else {
        _current_ptr = nullptr;
        _free_ptr = nullptr;
    }
}

bool allocator_sorted_list::sorted_iterator::occupied() const noexcept
{
    return _current_ptr != _free_ptr;
}
