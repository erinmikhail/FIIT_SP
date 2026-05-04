#include <not_implemented.h>
#include "../include/allocator_boundary_tags.h"
#include <new>
#include <stdexcept>

allocator_boundary_tags::~allocator_boundary_tags()
{
    if (_trusted_memory == nullptr) return;

    char* base = reinterpret_cast<char*>(_trusted_memory);
    auto parent_allocator = *reinterpret_cast<std::pmr::memory_resource**>(base);
    auto space_size = *reinterpret_cast<size_t*>(base + sizeof(std::pmr::memory_resource*) + sizeof(allocator_with_fit_mode::fit_mode));
    
    auto mut = reinterpret_cast<std::mutex*>(base + sizeof(std::pmr::memory_resource*) + sizeof(allocator_with_fit_mode::fit_mode) + sizeof(size_t));
    mut->~mutex();

    std::pmr::memory_resource* resource = (parent_allocator != nullptr) 
        ? parent_allocator 
        : std::pmr::get_default_resource();

    resource->deallocate(_trusted_memory, space_size);
}

allocator_boundary_tags::allocator_boundary_tags(
    allocator_boundary_tags &&other) noexcept
{
    other._trusted_memory = nullptr;
}

allocator_boundary_tags &allocator_boundary_tags::operator=(
    allocator_boundary_tags &&other) noexcept
{
    if (this == &other) return *this;
    this->~allocator_boundary_tags();
    _trusted_memory = other._trusted_memory;
    other._trusted_memory = nullptr;
    return *this;
}


/** If parent_allocator* == nullptr you should use std::pmr::get_default_resource()
 */
allocator_boundary_tags::allocator_boundary_tags(
        size_t space_size,
        std::pmr::memory_resource *parent_allocator,
        allocator_with_fit_mode::fit_mode allocate_fit_mode)
{
    size_t min_size = allocator_metadata_size + occupied_block_metadata_size;
    if (space_size < min_size) {
        throw std::bad_alloc();
    }

    std::pmr::memory_resource* resource = (parent_allocator != nullptr) 
        ? parent_allocator 
        : std::pmr::get_default_resource();
    
    _trusted_memory = resource->allocate(space_size);
    char* ptr = reinterpret_cast<char*>(_trusted_memory);

    *reinterpret_cast<std::pmr::memory_resource**>(ptr) = parent_allocator;
    ptr += sizeof(std::pmr::memory_resource*);
    *reinterpret_cast<allocator_with_fit_mode::fit_mode*>(ptr) = allocate_fit_mode;
    ptr += sizeof(allocator_with_fit_mode::fit_mode);
    *reinterpret_cast<size_t*>(ptr) = space_size;
    ptr += sizeof(size_t);
    new (ptr) std::mutex();
    ptr += sizeof(std::mutex);

    void** first_block_ptr_loc = reinterpret_cast<void**>(ptr); // смещение на начало первого блока (после метаданных аллокатора)
    ptr += sizeof(void*);
    *first_block_ptr_loc = reinterpret_cast<void*>(ptr);

    *reinterpret_cast<size_t*>(ptr) = space_size - allocator_metadata_size - occupied_block_metadata_size; 
    *reinterpret_cast<void**>(ptr + sizeof(size_t)) = nullptr; 
    *reinterpret_cast<void**>(ptr + sizeof(size_t) + sizeof(void*)) = nullptr;
    *reinterpret_cast<void**>(ptr + sizeof(size_t) + 2 * sizeof(void*)) = nullptr;
}

[[nodiscard]] void *allocator_boundary_tags::do_allocate_sm(
    size_t size)
{
    if (_trusted_memory == nullptr) throw std::bad_alloc();

    char* base = reinterpret_cast<char*>(_trusted_memory);
    auto mut = reinterpret_cast<std::mutex*>(base + sizeof(std::pmr::memory_resource*) + sizeof(allocator_with_fit_mode::fit_mode) + sizeof(size_t));
    std::lock_guard<std::mutex> lock(*mut);

    auto mode = *reinterpret_cast<allocator_with_fit_mode::fit_mode*>(base + sizeof(std::pmr::memory_resource*));

    boundary_iterator target_it = end();
    size_t target_size = 0;

    for (auto it = begin(); it != end(); ++it)
    {
        if (!it.occupied() && it.size() >= size)
        {
            if (mode == allocator_with_fit_mode::fit_mode::first_fit) {
                target_it = it; target_size = it.size(); break;
            } else if (mode == allocator_with_fit_mode::fit_mode::the_best_fit) {
                if (target_it == end() || it.size() < target_size) {
                    target_it = it; target_size = it.size();
                }
            } else if (mode == allocator_with_fit_mode::fit_mode::the_worst_fit) {
                if (target_it == end() || it.size() > target_size) {
                    target_it = it; target_size = it.size();
                }
            }
        }
    }

    if (target_it == end()) throw std::bad_alloc();

    char* target_ptr = reinterpret_cast<char*>(target_it.get_ptr());
    void* old_next = *reinterpret_cast<void**>(target_ptr + sizeof(size_t) + 2 * sizeof(void*));

    if (target_size - size >= occupied_block_metadata_size)
    {
        size_t remainder_size = target_size - size - occupied_block_metadata_size;

        *reinterpret_cast<size_t*>(target_ptr) = size;

        *reinterpret_cast<void**>(target_ptr + sizeof(size_t)) = this;

        char* new_block_ptr = target_ptr + occupied_block_metadata_size + size;
        *reinterpret_cast<size_t*>(new_block_ptr) = remainder_size;
        *reinterpret_cast<void**>(new_block_ptr + sizeof(size_t)) = nullptr; // Свободен

        *reinterpret_cast<void**>(new_block_ptr + sizeof(size_t) + sizeof(void*)) = target_ptr;
        *reinterpret_cast<void**>(new_block_ptr + sizeof(size_t) + 2 * sizeof(void*)) = old_next;
        
        *reinterpret_cast<void**>(target_ptr + sizeof(size_t) + 2 * sizeof(void*)) = new_block_ptr;

        if (old_next != nullptr) {
            *reinterpret_cast<void**>(reinterpret_cast<char*>(old_next) + sizeof(size_t) + sizeof(void*)) = new_block_ptr;
        }
    }
    else
    {
        *reinterpret_cast<void**>(target_ptr + sizeof(size_t)) = this;
    }

    return target_ptr + occupied_block_metadata_size;
}

void allocator_boundary_tags::do_deallocate_sm(
    void *at)
{
    if (at == nullptr || _trusted_memory == nullptr) return;

    char* base = reinterpret_cast<char*>(_trusted_memory);
    auto mut = reinterpret_cast<std::mutex*>(base + sizeof(std::pmr::memory_resource*) + sizeof(allocator_with_fit_mode::fit_mode) + sizeof(size_t));
    std::lock_guard<std::mutex> lock(*mut);

    // Получаем начало метаданных возвращаемого блока
    char* target_ptr = reinterpret_cast<char*>(at) - occupied_block_metadata_size;

    //  Проверка принадлежности
    void* alloc_ptr = *reinterpret_cast<void**>(target_ptr + sizeof(size_t));
    if (alloc_ptr != this) {
        throw std::logic_error("allocator_boundary_tags: attempt to deallocate memory not belonging to this allocator");
    }

    *reinterpret_cast<void**>(target_ptr + sizeof(size_t)) = nullptr;

    void* prev_block = *reinterpret_cast<void**>(target_ptr + sizeof(size_t) + sizeof(void*));
    void* next_block = *reinterpret_cast<void**>(target_ptr + sizeof(size_t) + 2 * sizeof(void*));

    //  Склейка с ПРАВЫМ соседом
    if (next_block != nullptr) 
    {
        void* next_alloc_ptr = *reinterpret_cast<void**>(reinterpret_cast<char*>(next_block) + sizeof(size_t));
        
        if (next_alloc_ptr == nullptr) // Правый сосед свободен
        { 
            size_t target_size = *reinterpret_cast<size_t*>(target_ptr);
            size_t next_size = *reinterpret_cast<size_t*>(next_block);

            // Поглощаем правого соседа
            *reinterpret_cast<size_t*>(target_ptr) = target_size + occupied_block_metadata_size + next_size;

            void* next_next_block = *reinterpret_cast<void**>(reinterpret_cast<char*>(next_block) + sizeof(size_t) + 2 * sizeof(void*));
            *reinterpret_cast<void**>(target_ptr + sizeof(size_t) + 2 * sizeof(void*)) = next_next_block;

            // Если за правым соседом кто-то был, говорим ему, что теперь его левый сосед - это мы
            if (next_next_block != nullptr) {
                *reinterpret_cast<void**>(reinterpret_cast<char*>(next_next_block) + sizeof(size_t) + sizeof(void*)) = target_ptr;
            }
        }
    }

    //  Склейка с ЛЕВЫМ соседом (если он существует и свободен)
    if (prev_block != nullptr) 
    {
        void* prev_alloc_ptr = *reinterpret_cast<void**>(reinterpret_cast<char*>(prev_block) + sizeof(size_t));
        
        if (prev_alloc_ptr == nullptr) // Левый сосед свободен
        { 
            size_t prev_size = *reinterpret_cast<size_t*>(prev_block);
            size_t target_size = *reinterpret_cast<size_t*>(target_ptr); // Учитываем, что он мог вырасти при склейке с правым соседом

            // Левый сосед поглощает нас
            *reinterpret_cast<size_t*>(prev_block) = prev_size + occupied_block_metadata_size + target_size;

            // Левый сосед забирает наш next себе
            void* target_next_block = *reinterpret_cast<void**>(target_ptr + sizeof(size_t) + 2 * sizeof(void*));
            *reinterpret_cast<void**>(reinterpret_cast<char*>(prev_block) + sizeof(size_t) + 2 * sizeof(void*)) = target_next_block;

            // Если справа от нас кто-то остался, говорим ему, что его левый сосед теперь - наш левый сосед
            if (target_next_block != nullptr) {
                *reinterpret_cast<void**>(reinterpret_cast<char*>(target_next_block) + sizeof(size_t) + sizeof(void*)) = prev_block;
            }
        }
    }
}

inline void allocator_boundary_tags::set_fit_mode(
    allocator_with_fit_mode::fit_mode mode)
{
    if (_trusted_memory == nullptr) return;
    char* base = reinterpret_cast<char*>(_trusted_memory);
    auto mut = reinterpret_cast<std::mutex*>(base + sizeof(std::pmr::memory_resource*) + sizeof(allocator_with_fit_mode::fit_mode) + sizeof(size_t));
    std::lock_guard<std::mutex> lock(*mut);
    auto mode_ptr = reinterpret_cast<allocator_with_fit_mode::fit_mode*>(base + sizeof(std::pmr::memory_resource*));
    *mode_ptr = mode;
}


std::vector<allocator_test_utils::block_info> allocator_boundary_tags::get_blocks_info() const
{
    try {
        if (_trusted_memory == nullptr) return {};
        char* base = reinterpret_cast<char*>(_trusted_memory);
        auto mut = reinterpret_cast<std::mutex*>(base + sizeof(std::pmr::memory_resource*) + sizeof(allocator_with_fit_mode::fit_mode) + sizeof(size_t));
        std::lock_guard<std::mutex> lock(*mut);
        return get_blocks_info_inner();
    } catch (...) { return {}; }
}

allocator_boundary_tags::boundary_iterator allocator_boundary_tags::begin() const noexcept
{
    return boundary_iterator(_trusted_memory);
}

allocator_boundary_tags::boundary_iterator allocator_boundary_tags::end() const noexcept
{
    return boundary_iterator(nullptr);
}

std::vector<allocator_test_utils::block_info> allocator_boundary_tags::get_blocks_info_inner() const
{
    std::vector<allocator_test_utils::block_info> info;
    for (auto it = begin(); it != end(); ++it) {
        info.push_back({it.size() + occupied_block_metadata_size, it.occupied()});
    }

    if (!info.empty()) {
        info.back().block_size += allocator_metadata_size;
    }
    
    return info;
}

allocator_boundary_tags::allocator_boundary_tags(const allocator_boundary_tags &other)
{
    throw std::logic_error("allocator_boundary_tags: copying is forbidden");
}

allocator_boundary_tags &allocator_boundary_tags::operator=(const allocator_boundary_tags &other)
{
    throw std::logic_error("allocator_boundary_tags: copying is forbidden");
}

bool allocator_boundary_tags::do_is_equal(const std::pmr::memory_resource &other) const noexcept
{
    auto other_ptr = dynamic_cast<allocator_boundary_tags const *>(&other);
    if (other_ptr == nullptr) return false;
    return this->_trusted_memory == other_ptr->_trusted_memory;
}

bool allocator_boundary_tags::boundary_iterator::operator==(
        const allocator_boundary_tags::boundary_iterator &other) const noexcept
{
    return _occupied_ptr == other._occupied_ptr;
}

bool allocator_boundary_tags::boundary_iterator::operator!=(
        const allocator_boundary_tags::boundary_iterator & other) const noexcept
{
    return _occupied_ptr != other._occupied_ptr;
}

allocator_boundary_tags::boundary_iterator &allocator_boundary_tags::boundary_iterator::operator++() & noexcept
{
    if (_occupied_ptr != nullptr) 
    {
        char* metadata = reinterpret_cast<char*>(_occupied_ptr);
        _occupied_ptr = *reinterpret_cast<void**>(metadata + sizeof(size_t) + 2 * sizeof(void*));
    }
    return *this;
}

allocator_boundary_tags::boundary_iterator &allocator_boundary_tags::boundary_iterator::operator--() & noexcept
{
    if (_occupied_ptr != nullptr) 
    {
        char* metadata = reinterpret_cast<char*>(_occupied_ptr);
        _occupied_ptr = *reinterpret_cast<void**>(metadata + sizeof(size_t) + sizeof(void*));
    }
    return *this;
}

allocator_boundary_tags::boundary_iterator allocator_boundary_tags::boundary_iterator::operator++(int n)
{
    auto temp = *this; ++(*this); return temp;
}

allocator_boundary_tags::boundary_iterator allocator_boundary_tags::boundary_iterator::operator--(int n)
{
    auto temp = *this; --(*this); return temp;
}

size_t allocator_boundary_tags::boundary_iterator::size() const noexcept
{
    if (_occupied_ptr == nullptr) return 0;
    return *reinterpret_cast<size_t*>(_occupied_ptr);
}

bool allocator_boundary_tags::boundary_iterator::occupied() const noexcept
{
    if (_occupied_ptr == nullptr) return false;
    void* alloc_ptr = *reinterpret_cast<void**>(reinterpret_cast<char*>(_occupied_ptr) + sizeof(size_t));
    return alloc_ptr != nullptr;
}

void* allocator_boundary_tags::boundary_iterator::operator*() const noexcept
{
    if (_occupied_ptr == nullptr) return nullptr;
    return reinterpret_cast<char*>(_occupied_ptr) + occupied_block_metadata_size;
}

allocator_boundary_tags::boundary_iterator::boundary_iterator() :  _occupied_ptr(nullptr), _occupied(false), _trusted_memory(nullptr) {}

allocator_boundary_tags::boundary_iterator::boundary_iterator(void *trusted) : _trusted_memory(trusted)
{
    if (_trusted_memory != nullptr){
        char* base = reinterpret_cast<char*>(_trusted_memory);
        _occupied_ptr = *reinterpret_cast<void**>(base + allocator_metadata_size - sizeof(void*));
    } else {
        _occupied_ptr = nullptr;
    }
}

void *allocator_boundary_tags::boundary_iterator::get_ptr() const noexcept
{
    return _occupied_ptr;
}
