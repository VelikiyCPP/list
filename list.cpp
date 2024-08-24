#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstddef>
#include <initializer_list>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <thread>
#include <type_traits>

template <typename T>
using is_random_access_iterator = std::is_base_of<std::random_access_iterator_tag,
    typename std::iterator_traits<T>::iterator_category>;

template< class T>
class List {

    using value_type = T;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using reference = value_type&;
    using const_reference = const value_type&;
    using pointer = value_type*;
    using const_pointer = const value_type*;

    struct Base_node {
        Base_node* prev = nullptr;
        Base_node* next = nullptr;

        virtual ~Base_node() = default;
    };

    struct Node final : public Base_node {
        value_type value{};

        Node(const Node&) = delete;
        Node(Node&) = delete;

        Node& operator=(Node&) = delete;
        Node& operator=(const Node&) = delete;

        Node() = delete;

        Node(const_reference value) : value(value) {}

        virtual ~Node() = default;
    };

    Base_node base_node_;
    size_type size_{};

    template <bool is_const>
    class base_iterator {
    public:
        using iterator_category = std::bidirectional_iterator_tag;
        using value_type = T;
        using difference_type = std::ptrdiff_t;
        using pointer = typename std::conditional_t<is_const, const Node*, Node*>;
        using reference = typename std::conditional_t<is_const, const T&, T&>;

        pointer ptr_ = nullptr;

        base_iterator() : ptr_(nullptr) {}

        base_iterator(pointer ptr) : ptr_(ptr) {}

        base_iterator(const base_iterator& other) : ptr_(other.ptr_) {}

        base_iterator& operator=(const base_iterator& other) {
            ptr_ = other.ptr_;
            return *this;
        }

        reference operator*() const {
            return ptr_->value;
        }

        pointer operator->() const {
            return ptr_;
        }

        base_iterator& operator++() {
            ptr_ = static_cast<pointer>(ptr_->next);
            return *this;
        }

        base_iterator& operator--() {
            ptr_ = static_cast<pointer>(ptr_->prev);
            return *this;
        }

        base_iterator operator++(int) {
            base_iterator temp = *this;
            ++(*this);
            return temp;
        }

        base_iterator operator--(int) {
            base_iterator temp = *this;
            --(*this);
            return temp;
        }

        bool operator==(const base_iterator& other) const {
            return ptr_ == other.ptr_;
        }

        bool operator!=(const base_iterator& other) const {
            return ptr_ != other.ptr_;
        }

        operator base_iterator<true>() const {
            return base_iterator<true>(ptr_);
        }

        ~base_iterator() = default;
    };
public:
    using iterator = base_iterator<false>;
    using const_iterator = base_iterator<true>;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    List() {
        base_node_.next = &base_node_;
        base_node_.prev = &base_node_;
    }

    explicit List(size_type count, const_reference value = value_type()) : List() {
        while (count--) {
            push_back(value);
        }
    }

    template<class InputIt>
    List(InputIt first, InputIt last) : List() {
        for (auto it = first; it != last; ++it) {
            push_back(*it);
        }
    }

    explicit List(const List& other) : List() {
        for (const auto& elem : other) {
            push_back(elem);
        }
    }

    List& operator=(const List& other) {
        if (this != &other) {
            clear();
            for (const auto& elem : other) {
                push_back(elem);
            }
        }
        return *this;
    }

    // Modifiers

    void push_back(const_reference value) noexcept {
        Node* new_node = new Node(value);

        if (base_node_.next == &base_node_) {
            base_node_.next = new_node;
            base_node_.prev = new_node;
            new_node->next = &base_node_;
            new_node->prev = &base_node_;
        }
        else {
            base_node_.prev->next = new_node;
            new_node->prev = base_node_.prev;
            new_node->next = &base_node_;
            base_node_.prev = new_node;
        }

        ++size_;
    }


    void push_front(const_reference value) noexcept {
        Node* new_node = new Node(value);

        if (base_node_.next == &base_node_) {
            base_node_.next = new_node;
            base_node_.prev = new_node;
            new_node->next = &base_node_;
            new_node->prev = &base_node_;
        }
        else {
            base_node_.next->prev = new_node;
            new_node->next = base_node_.next;
            new_node->prev = &base_node_;
            base_node_.next = new_node;
        }

        ++size_;
    }

    void pop_back() noexcept {
        if (base_node_.prev != &base_node_) {
            Node* front_node = static_cast<Node*>(base_node_.prev);

            base_node_.prev = front_node->prev;
            front_node->prev->next = &base_node_;

            delete front_node;

            --size_;
        }
    }

    void pop_front() noexcept {
        if (base_node_.next != &base_node_) {
            Node* front_node = static_cast<Node*>(base_node_.next);

            base_node_.next = front_node->next;
            front_node->next->prev = &base_node_;

            delete front_node;

            --size_;
        }
    }

    // Capacity
    bool empty() const
    {
        return size_ == 0;
    }

    size_type size() const
    {
        return size_;
    }

    void resize(size_type count)
    {
        if (count < size_) {
            while (size_ > count) {
                pop_back();
            }
        }
        else if (count > size_) {
            while (size_ < count) {
                push_back(T{});
            }
        }

        size_ = count;
    }
    void resize(const size_type count, const value_type& value) {
        if (count < size_) {
            resize(count);
        }
        else if (count > size_) {
            while (size_ < count) {
                push_back(T(value));
            }
        }

        size_ = count;
    }

    // Iterators
    iterator begin() {
        return iterator(static_cast<Node*>(base_node_.next));
    }

    const_iterator cbegin() const {
        return const_iterator(static_cast<const Node*>(base_node_.next));
    }

    iterator end() {
        return iterator(static_cast<Node*>(&base_node_));
    }

    const_iterator cend() const {
        return const_iterator(static_cast<const Node*>(&base_node_));
    }

    reverse_iterator rbegin() {
        return reverse_iterator(end());
    }

    const_reverse_iterator crbegin() const {
        return const_reverse_iterator(cend());
    }

    reverse_iterator rend() {
        return reverse_iterator(begin());
    }

    const_reverse_iterator crend() const {
        return const_reverse_iterator(cbegin());
    }

    // Element access
    reference front() {
        return static_cast<Node*>(base_node_.next)->value;
    }
    const_reference front() const {
        return static_cast<Node*>(base_node_.next)->value;
    }
    reference back() {
        return static_cast<Node*>(base_node_.prev)->value;
    }
    const_reference back() const {
        return static_cast<Node*>(base_node_.prev)->value;
    }

    iterator insert(const_iterator pos, const_reference value) {
        Node* new_node = new Node(value);
        Node* current_node = const_cast<Node*>(pos.ptr_);

        new_node->next = current_node;
        new_node->prev = current_node->prev;
        current_node->prev->next = new_node; 
        current_node->prev = new_node;           ++size_;
        
        return iterator(new_node);
    }

    iterator insert(const_iterator pos, size_type count, const_reference value) { // test
        iterator iter(const_cast<Node*>(pos.ptr_));
        
        for (size_type i = 0; i < count; ++i) {
            iter = insert(iter, value);
        }
        
        return iter;
    }

    template<std::input_iterator InputIt>
    iterator insert(const_iterator pos, InputIt first, InputIt last) {
        iterator ret_iter(const_cast<Node*>(pos.ptr_));
        
        for (auto it = std::prev(last); it != std::prev(first); --it) {
            ret_iter = insert(ret_iter, *it);
        }
        return ret_iter;
    }

    iterator erase(const_iterator pos) {
        if (pos.ptr_ == &base_node_) {
            throw std::logic_error("Cannot erase from the base node");
        } else if (empty()) {
            throw std::out_of_range("Cannot erase from an empty list.");
        }
        
        Node* current_node = const_cast<Node*>(pos.ptr_);

        current_node->prev->next = current_node->next;
        current_node->next->prev = current_node->prev;
        
        Node* next_node = static_cast<Node*>(current_node->next);
    
        delete current_node;                             --size_;
    
        return iterator(next_node);
    }

    iterator erase(const_iterator first, const_iterator last) {
        
        if (first.ptr_ == &base_node_ || last.ptr_ == &base_node_) {
            throw std::logic_error("Cannot erase from the base node");
        } else if (first == last) {
            return iterator(const_cast<Node*>(last.ptr_));
        }
    
        Node* first_node = const_cast<Node*>(first.ptr_);
        Node* last_node = const_cast<Node*>(last.ptr_);
    
        first_node->prev->next = last_node;
        last_node->prev = first_node->prev;
        
        while (first_node != last_node) {
            Node* next_node = static_cast<Node*>(first_node->next);
            
            delete first_node;
            
            first_node = next_node;    --size_;
        }
        
        return iterator(last_node);
    }

    void clear() {
        if (base_node_.next == &base_node_) {
            return;
        }
    
        Node* current = static_cast<Node*>(base_node_.next);
    
        while (current != &base_node_) {
            Node* next_node = static_cast<Node*>(current->next);
            
            current->~Node();
            delete current;
            
            current = next_node;
        }
    
        base_node_.next = &base_node_;
        base_node_.prev = &base_node_;
        
        size_ = 0;
    }

    void sort() {
        if (size_ > 1){
            bool swapped;
            
            do {
                swapped = false;
                for (auto it = begin(); std::next(it) != end(); ++it) {
                    auto next_it = std::next(it);
                    if (*next_it < *it) {
                        std::iter_swap(it, next_it);
                        swapped = true;
                    }
                }
            } while (swapped);
        }
    }

    void swap(List& other) {
        using std::swap;
        swap(base_node_, other.base_node_);
        swap(size_, other.size_);
    }

    void unique() {
        if (size_ > 1) {
            for (auto it = begin(), next_it = std::next(it); next_it != end(); ) {
                if (*it == *next_it) {
                    next_it = erase(next_it);
                    continue;
                }
                
                (++it,++next_it);
            }
        }
    }

    // Please note: This code was generated using artificial intelligence without testing.
    void merge(List& other) {
        auto it1 = begin();
        auto it2 = other.begin();

        while (it2 != other.end()) {
            if (it1 == end() || *it2 < *it1) {
                insert(it1, *it2);
                it2 = other.erase(it2);
            }
            else {
                ++it1;
            }
        }
    }

    void reverse() noexcept {
        
        iterator left = begin();
        iterator right = std::prev(end());
        
        while (left != right && left.ptr_ != right.ptr_->next) {
            std::iter_swap(left, right);
            ++left;
            --right;
        }
    }
    
    void assign(size_type count, const_reference value) {
        clear();

        if (count != 0) {

            Node* current = new Node(value);

            base_node_.next = current;
            current->prev = static_cast<Node*>(&base_node_);

            for (size_type i = 1; i < count; ++i) {
                current->next = new Node(value);
                current->next->prev = current;
                current = static_cast<Node*>(current->next);
            }

            current->next = static_cast<Node*>(&base_node_);
            base_node_.prev = current;

            size_ = count;
        }
    }
    
    template <std::input_iterator InputIt>
    void assign(InputIt first, InputIt last) {
        clear();
    
        for (auto it = first; it != last; ++it) {
            push_back(*it);
        }
    }

    // Please note: This code was generated using artificial intelligence without testing.
    void splice(const_iterator pos, List& other) {
        if (this == &other || other.empty()) return;

        auto first = other.begin();
        auto last = other.end();
        splice(pos, other, first, last);
    }

    // Please note: This code was generated using artificial intelligence without testing.
    void splice(const_iterator pos, List& other, const_iterator it) {
        auto next_it = std::next(it);
        splice(pos, other, it, next_it);
    }

    // Please note: This code was generated using artificial intelligence without testing.
    void splice(const_iterator pos, List& other, const_iterator first, const_iterator last) {
        if (first == last) return;

        auto prev_first = first.ptr_->prev;
        auto next_last = last.ptr_;

        prev_first->next = next_last;
        next_last->prev = prev_first;

        first.ptr_->prev = const_cast<Base_node*>(pos.ptr_->prev);
        last.ptr_->prev->next = const_cast<Base_node*>(pos.ptr_);
        pos.ptr_->prev->next = first.ptr_;
        pos.ptr_->prev = last.ptr_->prev;

        size_ += std::distance(first, last);
        other.size_ -= std::distance(first, last);
    }

    // Please note: This code was generated using artificial intelligence without testing.
    void remove(const_reference value) {
        auto it = begin();
        while (it != end()) {
            if (*it == value) {
                it = erase(it);
            }
            else {
                ++it;
            }
        }
    }

    ~List() {
        clear(); 
    }
};

#include <vector>
#include <list>

int main() {
    std::vector<int> vec{};
    List<int> list;

    list.insert(list.cbegin(), vec.begin(), vec.end());

    //list.erase(list.begin());

    list.push_back(100);
    list.push_front(500);

    list.assign(vec.begin(), vec.end());
    
    //list.sort();
    
    list.reverse();
    list.push_back(100);
    list.push_back(500);
    list.push_back(500);
    list.push_back(500);
    list.push_back(500);
    list.push_back(100);
    list.push_back(100);
    list.push_back(100);
    list.reverse();
    list.push_back(500);
    list.unique();

    for(const auto& i : list){
        std::cout << i << "\n";
    }

    return 0;
}