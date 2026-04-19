#pragma once

#include "concepts.hpp"
#include <range/v3/view/all.hpp>
#include <range/v3/view/view.hpp>
#include <utility>
#include <memory>

namespace rpp {

// ============================================================================
// Pipeable base class - enables range | adapter syntax
// ============================================================================

template<typename Derived>
struct pipeable_base {
    template<input_range R>
    friend constexpr auto operator|(R&& r, const Derived& self) {
        return self(std::forward<R>(r));
    }
    
    template<input_range R>
    friend constexpr auto operator|(R&& r, Derived&& self) {
        return std::move(self)(std::forward<R>(r));
    }
};

// ============================================================================
// Adapter composition - enables adapter1 | adapter2 syntax
// ============================================================================

template<typename A1, typename A2>
struct composed_adapter : pipeable_base<composed_adapter<A1, A2>> {
    A1 first;
    A2 second;
    
    constexpr composed_adapter(A1 a1, A2 a2) 
        : first(std::move(a1)), second(std::move(a2)) {}
    
    template<input_range R>
    constexpr auto operator()(R&& r) const {
        return second(first(std::forward<R>(r)));
    }
    
    template<input_range R>
    constexpr auto operator()(R&& r) {
        return second(first(std::forward<R>(r)));
    }
};

namespace detail {
    template<typename T>
    struct is_pipeable : std::false_type {};
    
    template<typename D>
    struct is_pipeable<pipeable_base<D>> : std::true_type {};
    
    template<typename T>
    concept pipeable_type = std::derived_from<T, pipeable_base<T>> ||
        requires { typename T::is_pipeable_tag; };
    
    // Detect range-v3 view closure
    template<typename T>
    concept range_v3_pipeable = requires {
        typename T::CPP_template_sfinae_guard_;
    } || requires(T t) {
        { t.base() };
    };
}

// Composition between rpp adapters
template<typename A1, typename A2>
    requires detail::pipeable_type<A1> && detail::pipeable_type<A2>
constexpr auto operator|(A1 a1, A2 a2) {
    return composed_adapter<A1, A2>{std::move(a1), std::move(a2)};
}

// ============================================================================
// Closure adapter - wraps parameterized adapter factories
// ============================================================================

template<typename Fn>
struct closure : pipeable_base<closure<Fn>> {
    Fn fn;
    
    constexpr explicit closure(Fn f) : fn(std::move(f)) {}
    
    template<input_range R>
    constexpr auto operator()(R&& r) const {
        return fn(std::forward<R>(r));
    }
    
    template<input_range R>
    constexpr auto operator()(R&& r) {
        return fn(std::forward<R>(r));
    }
    
    using is_pipeable_tag = void;
};

template<typename Fn>
closure(Fn) -> closure<Fn>;

// ============================================================================
// make_closure helper
// ============================================================================

template<typename Fn>
[[nodiscard]] constexpr auto make_closure(Fn&& fn) {
    return closure<std::decay_t<Fn>>{std::forward<Fn>(fn)};
}

// ============================================================================
// owning_view - view that owns a container, allowing continued pipeline ops
// Uses unique_ptr for stable iterator semantics across moves (no ref-count overhead).
// ============================================================================

template<typename Container>
class owning_view : public ::ranges::view_base {
    std::unique_ptr<Container> data_;
    
public:
    using iterator = typename Container::iterator;
    using const_iterator = typename Container::const_iterator;
    using value_type = typename Container::value_type;
    
    owning_view() = default;
    
    explicit owning_view(Container c) 
        : data_(std::make_unique<Container>(std::move(c))) {}
    
    owning_view(owning_view&&) noexcept = default;
    owning_view& operator=(owning_view&&) noexcept = default;
    
    // Deep copy for range-v3 semi-regular requirement
    owning_view(const owning_view& other) 
        : data_(other.data_ ? std::make_unique<Container>(*other.data_) : nullptr) {}
    owning_view& operator=(const owning_view& other) {
        if (this != &other) {
            data_ = other.data_ ? std::make_unique<Container>(*other.data_) : nullptr;
        }
        return *this;
    }
    
    auto begin() const { return data_->begin(); }
    auto end() const { return data_->end(); }
    auto begin() { return data_->begin(); }
    auto end() { return data_->end(); }
    
    auto size() const { return data_->size(); }
    bool empty() const { return !data_ || data_->empty(); }
    
    auto& operator[](std::size_t i) { return (*data_)[i]; }
    const auto& operator[](std::size_t i) const { return (*data_)[i]; }
    
    Container& container() { return *data_; }
    const Container& container() const { return *data_; }
};

template<typename Container>
owning_view(Container) -> owning_view<Container>;

// Helper: convert a container to owning_view
template<typename Container>
[[nodiscard]] auto as_view(Container&& c) {
    return owning_view<std::decay_t<Container>>{std::forward<Container>(c)};
}

} // namespace rpp
