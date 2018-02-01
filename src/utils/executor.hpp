/*
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

Product name: redemption, a FLOSS RDP proxy
Copyright (C) Wallix 2018
Author(s): Jonathan Poelen
*/

#pragma once

#include "cxx/cxx.hpp"
#include "cxx/diagnostic.hpp"

#include <vector>
#include <type_traits>
#include <utility>
#include <functional> // std::reference_wrapper
#include <chrono>
#include <cassert>
#include <memory>

namespace detail
{
    template<class... Ts>
    struct tuple;

    template<class Ints, class... Ts>
    struct tuple_impl;

    template<class T, class... Ts>
    struct emplace_type
    {
        tuple<Ts...> t;

        template<class... Us>
        auto operator()(Us&&... xs)
        {
            static_assert(0 == sizeof...(Ts));
            return emplace_type<T, Us&&...>{{xs...}};
        }
    };

    template<size_t, class T>
    struct tuple_elem
    {
        T x;

        template<std::size_t... ints, class... Ts>
        constexpr tuple_elem(int, tuple_impl<std::integer_sequence<size_t, ints...>, Ts...>& t)
          : x{static_cast<Ts&&>(static_cast<tuple_elem<ints, Ts>&>(t).x)...}
        {}

        template<class... Ts>
        constexpr tuple_elem(emplace_type<T, Ts...> e)
          : tuple_elem(1, e.t)
        {}

        template<class U>
        constexpr tuple_elem(U&& x)
          : x(static_cast<U&&>(x))
        {}
    };

    template<std::size_t... ints, class... Ts>
    struct tuple_impl<std::integer_sequence<size_t, ints...>, Ts...>
    : tuple_elem<ints, Ts>...
    {
        template<class F, class... Args>
        decltype(auto) invoke(F && f, Args&&... args)
        {
            return f(
                static_cast<Args&&>(args)...,
                static_cast<tuple_elem<ints, Ts>&>(*this).x...
            );
        }
    };

    template<class... Ts>
    struct tuple : tuple_impl<std::make_index_sequence<sizeof...(Ts)>, Ts...>
    {};

    template<class T> struct decay_and_strip { using type = T; };
    template<class T> struct decay_and_strip<T&> : decay_and_strip<T>{};
    template<class T> struct decay_and_strip<T const> : decay_and_strip<T>{};
    template<class T> struct decay_and_strip<std::reference_wrapper<T>> { using type = T&; };
    template<class T, class... Ts> struct decay_and_strip<emplace_type<T, Ts...>> { using type = T; };

    template<class... Args>
    using ctx_arg_type = detail::tuple<typename decay_and_strip<Args>::type...>;
}

template<class T>
constexpr auto emplace = detail::emplace_type<T>{};


// #define CXX_WARN_UNUSED_RESULT __attribute__((warn_unused_result))
#define CXX_WARN_UNUSED_RESULT [[nodiscard]]

class ExecutorBase;
class Executor;
template<class Ctx>
class ExecutorActionContext;

enum class REDEMPTION_CXX_NODISCARD ExecutorResult : uint8_t
{
    Nothing,
    ExitSuccess,
    ExitFailure,
    Terminate,
};

enum class ExitStatus { Error, Success, };

#ifdef IN_IDE_PARSER
struct SubExecutorBuilderConcept_
{
    template<class F> SubExecutorBuilderConcept_ on_action(F&&) && { return *this; }
    template<class F> SubExecutorBuilderConcept_ on_exit  (F&&) && { return *this; }

    template<class T> SubExecutorBuilderConcept_(T const&) noexcept;
};

struct ExecutorActionContextConcept_
{
    ExecutorResult retry();
    ExecutorResult exit(ExitStatus status);
    ExecutorResult exit_on_error();
    ExecutorResult exit_on_success();
    template<class F> ExecutorResult next_action(F);
    template<class F> ExecutorResult exec_action(F);
    template<class F1, class F2> ExecutorResult exec_action2(F1, F2);
    template<class... Args> SubExecutorBuilderConcept_ sub_executor(Args&&... args);
    template<class F> ExecutorActionContextConcept_ set_exit_action(F f);
};
#endif


namespace
{
    template<class F>
    F make_lambda() noexcept
    {
        static_assert(
            std::is_empty<F>::value,
            "F must be an empty class or a lambda expression convertible to pointer of function");
        // big hack for a lambda not default constructible before C++20 :)
        alignas(F) char const f[sizeof(F)]{}; // same as `char f`
        return reinterpret_cast<F const&>(f);
    }
}

class BasicExecutor;

enum class ExecutorError : uint8_t
{
    NoError,
    ActionError,
    Terminate,
    ExternalExit,
};


struct BasicExecutor
{
    ExecutorResult exec_action()
    {
        return this->on_action(*this);
    }

    ExecutorResult exec_exit(ExecutorError error)
    {
        return this->on_exit(*this, error);
    }

    void delete_self()
    {
        return this->deleter(this);
    }

    void terminate();

    bool exec();

    void exec_all()
    {
        while (this->exec()) {
        }
    }

    bool exit_with(ExecutorError);

protected:
    using OnActionPtrFunc = ExecutorResult(*)(BasicExecutor&);
    using OnExitPtrFunc = ExecutorResult(*)(BasicExecutor&, ExecutorError error);

    OnActionPtrFunc on_action = [](BasicExecutor&){ return ExecutorResult::Nothing; };
    OnExitPtrFunc on_exit = [](BasicExecutor&, ExecutorError){ return ExecutorResult::Nothing; };
    BasicExecutor* current = this;
    BasicExecutor* prev = nullptr;
    void (*deleter) (void*) = [](void*){};

    void set_next_executor(BasicExecutor& other) noexcept
    {
        other.current = this->current;
        other.prev = this;
        this->current->current = &other;
    }

    BasicExecutor() = default;
};

namespace detail
{
    struct GetExecutor
    {
        template<class T>
        auto& operator()(T& x) const
        { return x.executor; }
    };

    constexpr GetExecutor get_executor {};
}


template<class T, class U>
struct is_context_convertible;

template<class T, class U>
struct check_is_context_arg_convertible
{
    static constexpr bool value = (typename std::is_convertible<T, U>::type{} = std::true_type{});
};

template<class... Ts, class... Us>
struct is_context_convertible<detail::tuple<Ts...>, detail::tuple<Us...>>
{
    static constexpr bool value = (..., (check_is_context_arg_convertible<Ts, Us>::value));
};


template<class... Ts>
struct Executor2Impl;
template<class... Ts>
struct TopExecutorImpl;
template<class... Ts>
struct SubExecutor2Impl;
template<class... Ts>
struct SubAction2Impl;

class Reactor;
class TopExecutorBase;

namespace detail { namespace
{
    enum ExecutorType
    {
        Normal,
        Sub,
        Exec
    };

    template<class Executor, ExecutorType type, int Mask = 0>
    struct REDEMPTION_CXX_NODISCARD ExecutorBuilder
    {
        friend detail::GetExecutor;

        template<int Mask2>
        decltype(auto) select_return()
        {
            if constexpr (Mask == (~Mask2 & 0b111)) {
                if constexpr (ExecutorType::Sub == type) {
                    return ExecutorResult::Nothing;
                }
                else if constexpr (ExecutorType::Exec == type) {
                    return this->executor.exec_action();
                }
                else {
                    return this->executor;
                }
            }
            else {
                return ExecutorBuilder<Executor, type, Mask | Mask2>{this->executor};
            }
        }

        template<class F>
        decltype(auto) on_action(F f) && noexcept
        {
            static_assert(!(Mask & 0b001), "on_action already set");
            this->executor.set_on_action(f);
            return select_return<0b001>();
        }

        template<class F>
        decltype(auto) on_exit(F f) && noexcept
        {
            static_assert(!(Mask & 0b010), "on_exit already set");
            this->executor.set_on_exit(f);
            return select_return<0b010>();
        }

        template<class F>
        decltype(auto) on_timeout(std::chrono::milliseconds ms, F f) && noexcept
        {
            static_assert(!(Mask & 0b100), "on_timeout already set");
            this->executor.set_timeout(ms);
            this->executor.set_on_timeout(f);
            return select_return<0b100>();
        }

        ExecutorBuilder(Executor& executor) noexcept
        : executor(executor)
        {}

    private:
        Executor& executor;
    };
} }

template<class... Args>
using Executor2 = Executor2Impl<typename detail::decay_and_strip<Args>::type...>;
template<class... Args>
using TopExecutor2 = TopExecutorImpl<typename detail::decay_and_strip<Args>::type...>;

template<class... Args>
using TopExecutorBuilder = detail::ExecutorBuilder<TopExecutor2<Args...>, detail::ExecutorType::Normal>;
template<class... Args>
using SubExecutorBuilder = detail::ExecutorBuilder<Executor2<Args...>, detail::ExecutorType::Sub, 0b100>;
template<class... Args>
using ExecExecutorBuilder = detail::ExecutorBuilder<Executor2<Args...>, detail::ExecutorType::Exec, 0b100>;


template<class... Ts>
struct Timer2Impl;

template<class... Ts>
struct REDEMPTION_CXX_NODISCARD Executor2TimerContext
{
    template<class... PreviousTs>
    Executor2TimerContext(Executor2TimerContext<PreviousTs...> const& other) noexcept
      : timer(reinterpret_cast<Timer2Impl<Ts...>&>(detail::get_executor(other)))
    {
        // TODO strip arguments support (PreviousTs=(int, int), Ts=(int))
        static_assert((true && ... && check_is_context_arg_convertible<PreviousTs, Ts>::value));
        static_assert(sizeof(Timer2Impl<Ts...>) == sizeof(detail::get_executor(other)));
    }

    explicit Executor2TimerContext(Timer2Impl<Ts...>& timer) noexcept
      : timer{timer}
    {}

    Executor2TimerContext(Executor2TimerContext const&) = default;
    Executor2TimerContext& operator=(Executor2TimerContext const&) = delete;

    friend detail::GetExecutor;

    ExecutorResult detach_timer() noexcept
    {
        return ExecutorResult::ExitSuccess;
    }

    ExecutorResult retry() noexcept
    {
        this->timer.reset_time();
        return ExecutorResult::Nothing;
    }

    ExecutorResult retry_until(std::chrono::milliseconds ms)
    {
        this->timer.update_time(ms);
        return ExecutorResult::Nothing;
    }

    ExecutorResult terminate() noexcept
    {
        return ExecutorResult::Terminate;
    }

    template<class F>
    ExecutorResult next_action(F f) noexcept
    {
        this->timer.set_on_action(f);
        return ExecutorResult::Nothing;
    }

    template<class F1, class F2>
    ExecutorResult exec_action2(F1 f1, F2 f2)
    {
        this->timer.set_on_action(f1);
        return this->timer.ctx.invoke(f2, Executor2TimerContext{this->timer});
    }

    template<class F>
    ExecutorResult exec_action(F f)
    {
        return this->exec_action2(f, f);
    }

    Executor2TimerContext set_time(std::chrono::milliseconds ms)
    {
        this->timer.update_time(ms);
        return *this;
    }

protected:
    Timer2Impl<Ts...>& timer;
};


template<class... Ts>
struct REDEMPTION_CXX_NODISCARD Executor2ActionContext
{
    friend detail::GetExecutor;

    template<class... PreviousTs>
    Executor2ActionContext(Executor2ActionContext<PreviousTs...> const& other) noexcept
      : executor(reinterpret_cast<Executor2Impl<Ts...>&>(detail::get_executor(other)))
    {
        // TODO strip arguments support (PreviousTs=(int, int), Ts=(int))
        static_assert((true && ... && check_is_context_arg_convertible<PreviousTs, Ts>::value));
        static_assert(sizeof(Executor2Impl<Ts...>) == sizeof(detail::get_executor(other)));
    }

    explicit Executor2ActionContext(Executor2Impl<Ts...>& executor) noexcept
      : executor{executor}
    {}

    Executor2ActionContext(Executor2ActionContext const&) = default;
    Executor2ActionContext& operator=(Executor2ActionContext const&) = delete;

    ExecutorResult retry() noexcept
    {
        return ExecutorResult::Nothing;
    }

    ExecutorResult terminate() noexcept
    {
        return ExecutorResult::Terminate;
    }

    ExecutorResult exit(ExitStatus status) noexcept
    {
        return (status == ExitStatus::Success) ? this->exit_on_success() : this->exit_on_error();
    }

    ExecutorResult exit_on_error() noexcept
    {
        return ExecutorResult::ExitFailure;
    }

    ExecutorResult exit_on_success() noexcept
    {
        return ExecutorResult::ExitSuccess;
    }

    template<class... Args>
    auto create_timer(Args&&... args)
    {
        return executor.top_executor.create_timer(static_cast<Args&&>(args)...);
    }

    template<class... Args>
    SubExecutorBuilder<Args...> create_sub_executor(Args&&... args)
    {
        return executor.create_sub_executor(static_cast<Args&&>(args)...);
    }

    BasicExecutor& get_basic_executor() noexcept
    {
        return this->executor;
    }

    template<class... Args>
    SubExecutorBuilder<Ts..., Args...> create_nested_executor(Args&&... args)
    {
        return executor.create_nested_executor(static_cast<Args&&>(args)...);
    }

    template<class... Args>
    ExecExecutorBuilder<Args...> exec_sub_executor(Args&&... args)
    {
        auto builder = executor.create_sub_executor(static_cast<Args&&>(args)...);
        auto& sub_executor = detail::get_executor(builder);
        return {sub_executor};
    }

    template<class... Args>
    ExecExecutorBuilder<Ts..., Args...> exec_nested_executor(Args&&... args)
    {
        auto builder = executor.create_nested_executor(static_cast<Args&&>(args)...);
        auto& sub_executor = detail::get_executor(builder);
        return {sub_executor};
    }

    template<class F>
    ExecutorResult next_action(F f) noexcept
    {
        executor.set_on_action(f);
        return ExecutorResult::Nothing;
    }

    template<class F1, class F2>
    ExecutorResult exec_action2(F1 f1, F2 f2)
    {
        executor.set_on_action(f1);
        return executor.ctx.invoke(f2, Executor2ActionContext{this->executor});
    }

    template<class F>
    ExecutorResult exec_action(F f)
    {
        return this->exec_action2(f, f);
    }

    template<class F>
    Executor2ActionContext set_exit_action(F f) noexcept
    {
        executor.set_on_exit(f);
        return *this;
    }

protected:
    Executor2Impl<Ts...>& executor;
};


template<class... Ts>
struct Executor2Impl : public BasicExecutor
{
    friend Executor2ActionContext<Ts...>;

    template<class F>
    void set_on_action(F) noexcept
    {
        this->on_action = [](BasicExecutor& executor) {
            auto& self = static_cast<Executor2Impl&>(executor);
            return self.ctx.invoke(make_lambda<F>(), Executor2ActionContext<Ts...>(self));
        };
    }

    template<class F>
    void set_on_exit(F) noexcept
    {
        this->on_exit = [](BasicExecutor& executor, ExecutorError error) {
            auto& self = static_cast<Executor2Impl&>(executor);
            // TODO ExecutorExitContext
            return self.ctx.invoke(make_lambda<F>(), Executor2ActionContext<Ts...>(self), error);
        };
    }

    template<class... Args>
    SubExecutorBuilder<Args...> create_sub_executor(Args&&... args)
    {
        auto* sub_executor = Executor2<Args...>::New(this->top_executor, static_cast<Args&&>(args)...);
        this->set_next_executor(*sub_executor);
        return {*sub_executor};
    }

    Executor2Impl(Executor2Impl const&) = delete;
    Executor2Impl& operator=(Executor2Impl const&) = delete;

    REDEMPTION_DIAGNOSTIC_PUSH
    REDEMPTION_DIAGNOSTIC_CLANG_IGNORE("-Wmissing-braces")
    template<class... Args>
    Executor2Impl(TopExecutorBase& top_executor, Args&&... args)
      : ctx{static_cast<Args&&>(args)...}
      , top_executor(top_executor)
    {}
    REDEMPTION_DIAGNOSTIC_POP

    BasicExecutor& base() noexcept
    {
        return *this;
    }

    template<class... Args>
    static Executor2Impl* New(TopExecutorBase& top_executor, Args&&... args)
    {
        auto* p = new Executor2Impl(top_executor, static_cast<Args&&>(args)...);
        p->deleter = [](void* p) { delete static_cast<Executor2Impl*>(p); };
        return p;
    }

protected:
    detail::tuple<Ts...> ctx;

private:
    TopExecutorBase& top_executor;

private:
    void *operator new(size_t n) { return ::operator new(n); }
};


template<class... Ts>
struct REDEMPTION_CXX_NODISCARD Executor2TimeoutContext : Executor2ActionContext<Ts...>
{
    using Executor2ActionContext<Ts...>::Executor2ActionContext;

    template<class F>
    Executor2ActionContext<Ts...> set_timeout_action(F f) noexcept
    {
        auto executor_action = static_cast<Executor2ActionContext<Ts...>*>(this);
        detail::get_executor(executor_action)->set_on_timeout(f);
        return *executor_action;
    }

    Executor2TimeoutContext set_timeout(std::chrono::milliseconds ms) noexcept
    {
        auto& executor = static_cast<TopExecutorImpl<Ts...>&>(this->executor);
        executor.set_timeout(ms);
        return *this;
    }
};

struct BasicTimer
{
    ExecutorResult exec_timer()
    {
        return this->on_timer(*this);
    }

    void delete_self()
    {
        return this->deleter(this);
    }

    void reset_time()
    {
        this->remaining_ms = this->ms;
    }

    std::chrono::milliseconds time()
    {
        return this->ms;
    }

    std::chrono::milliseconds remaining_time()
    {
        return this->remaining_ms;
    }

protected:
    friend class TopExecutorTimers;

    using OnTimerPtrFunc = ExecutorResult(*)(BasicTimer&);
    std::chrono::milliseconds ms;
    std::chrono::milliseconds remaining_ms;
    OnTimerPtrFunc on_timer = [](BasicTimer&){ return ExecutorResult::Nothing; };
    void (*deleter) (void*) = [](void*){};

    void set_time(std::chrono::milliseconds ms)
    {
        this->ms = ms;
        this->remaining_ms = ms;
    }

    BasicTimer() = default;
};

template<class Base>
struct DeleteSelf
{
    void operator()(Base* p) const
    {
        p->delete_self();
    }
};

template<class Base, class T = Base>
using UniquePtr = std::unique_ptr<T, DeleteSelf<Base>>;

template<class Base>
struct Container
{
    template<class T, class... Args>
    T& emplace_back(Args&&... args)
    {
        auto* p = T::New(static_cast<Args&&>(args)...);
        this->xs.emplace_back(&p->base());
        return *p;
    }

    std::vector<UniquePtr<Base>> xs;
};

namespace detail
{
    template<class TimerPtr>
    struct REDEMPTION_CXX_NODISCARD TimerBuilder
    {
        template<class F>
        TimerPtr on_action(std::chrono::milliseconds ms, F f) && noexcept
        {
            this->timer_ptr->set_on_action(f);
            this->timer_ptr->update_time(ms);
            return static_cast<TimerPtr&&>(this->timer_ptr);
        }

        TimerBuilder(TimerPtr&& timer_ptr) noexcept
        : timer_ptr(static_cast<TimerPtr&&>(timer_ptr))
        {}

    private:
        TimerPtr timer_ptr;
    };
}

// template<class... Ts>
// struct TimedExecutor : Executor2Impl<Ts...>, BasicTimer
// {
//     BasicExecutor& base() noexcept
//     {
//         return this->executor;
//     }
//
//     template<class... Args>
//     static TimedExecutor* New(TopExecutorBase& top_executor, Args&&... args)
//     {
//         auto* p = new TimedExecutor{top_executor, static_cast<Args&&>(args)...};
//         p->deleter = [](void* p) { delete static_cast<TimedExecutor<Ts...>*>(p); };
//         return p;
//     }
// };

template<class... Args>
using Timer2 = Timer2Impl<typename detail::decay_and_strip<Args>::type...>;

template<class... Args>
using TimerBuilder = detail::TimerBuilder<UniquePtr<BasicTimer, Timer2<Args...>>>;

class TopExecutorBase;

struct TopExecutorTimers
{
    template<class... Args>
    TimerBuilder<Args...> create_timer(Args&&... args)
    {
        using TimerType = Timer2<Args...>;
        using UniqueTimerPtr = UniquePtr<BasicTimer, TimerType>;
        UniqueTimerPtr uptr(TimerType::New(
            *reinterpret_cast<TopExecutorBase*>(this),
            static_cast<Args&&>(args)...));
        this->timers.emplace_back(uptr.get());
        return std::move(uptr);
    }

//     template<class... Args>
//     TimedExecutorBuilder<Args...> create_timed_executor(Args&&... args)
//     {}

    void update_time(BasicTimer& timer, std::chrono::milliseconds ms)
    {
        (void)timer;
        (void)ms;
    }

    void detach_timer(BasicTimer& timer)
    {
        this->timers.erase(
            std::find_if(this->timers.begin(), this->timers.end(), [&timer](auto* p){
                return p == &timer;
            }),
            this->timers.end()
        );
    }

    void set_timeout(std::chrono::milliseconds ms)
    {
        this->ms = ms;
        this->remaining_ms = ms;
    }

    std::chrono::milliseconds get_next_timeout() const noexcept
    {
        std::chrono::milliseconds r = this->remaining_ms;
        for (auto& timer : this->timers) {
            r = std::min(r, timer->remaining_ms);
        }
        return r;
    }

    void exec_timeout();

protected:
    using OnTimeoutPtrFunc = ExecutorResult(*)(BasicExecutor&);
    OnTimeoutPtrFunc on_timeout;

private:
    std::chrono::milliseconds ms;
    std::chrono::milliseconds remaining_ms;
    std::vector<BasicTimer*> timers;
    // std::chrono::milliseconds next_timeout;
};

struct TopExecutorBase : TopExecutorTimers
{
    BasicExecutor base_executor;

    void delete_self()
    {
        this->base_executor.delete_self();
    }
};

void TopExecutorTimers::exec_timeout()
{
    auto ms = this->get_next_timeout();
    for (std::size_t i = 0; i < this->timers.size(); ) {
        auto* timer = this->timers[i];
        if (timer->remaining_ms <= ms) {
            switch (timer->exec_timer()) {
                case ExecutorResult::ExitSuccess:
                case ExecutorResult::ExitFailure:
                    this->timers.erase(this->timers.begin() + i);
                case ExecutorResult::Terminate:
                    break;
                case ExecutorResult::Nothing:
                    ++i;
                    break;
            }
        }
        else {
            ++i;
            timer->remaining_ms -= ms;
        }
    }
    if (this->remaining_ms <= ms) {
        this->remaining_ms = this->ms;
        this->on_timeout(reinterpret_cast<TopExecutorBase*>(this)->base_executor);
    }
    else {
        this->remaining_ms -= ms;
    }
}

template<class... Ts>
struct Timer2Impl : BasicTimer
{
    template<class F>
    void set_on_action(F) noexcept
    {
        this->on_timer = [](BasicTimer& timer) {
            auto& self = static_cast<Timer2Impl&>(timer);
            return self.ctx.invoke(make_lambda<F>(), Executor2TimerContext<Ts...>(self));
        };
    }

    void detach_timer()
    {
        this->top_executor.detach_timer(*this);
    }

    void update_time(std::chrono::milliseconds ms)
    {
        this->set_time(ms);
        this->top_executor.update_time(*this, ms);
    }

    Timer2Impl(Timer2Impl const&) = delete;
    Timer2Impl& operator=(Timer2Impl const&) = delete;

    REDEMPTION_DIAGNOSTIC_PUSH
    REDEMPTION_DIAGNOSTIC_CLANG_IGNORE("-Wmissing-braces")
    template<class... Args>
    Timer2Impl(TopExecutorBase& top_executor, Args&&... args)
      : ctx{static_cast<Args&&>(args)...}
      , top_executor(top_executor)
    {}
    REDEMPTION_DIAGNOSTIC_POP

    BasicTimer& base() noexcept
    {
        return *this;
    }

    template<class... Args>
    static Timer2Impl* New(TopExecutorBase& top_executor, Args&&... args)
    {
        auto* p = new Timer2Impl(top_executor, static_cast<Args&&>(args)...);
        p->deleter = [](void* p) {
            auto* timer_ptr = static_cast<Timer2Impl*>(p);
            timer_ptr->top_executor.detach_timer(*timer_ptr);
            delete timer_ptr;
        };
        return p;
    }

protected:
    detail::tuple<Ts...> ctx;

private:
    TopExecutorBase& top_executor;

private:
    void *operator new(size_t n) { return ::operator new(n); }
};

template<class... Ts>
struct TopExecutorImpl : TopExecutorTimers, Executor2Impl<Ts...>
{
    using Executor2Impl<Ts...>::Executor2Impl;

    ExecutorResult exec_timeout()
    {
        return this->timeout.on_timeout(*this);
    }

    template<class... Args>
    SubExecutorBuilder<Args...> create_sub_executor(BasicExecutor& from, Args&&... args)
    {
        auto* sub_executor = Executor2<Args...>::New(*this, static_cast<Args&&>(args)...);
        sub_executor->current = from.current;
        sub_executor->prev = &from;
        from.current->current = sub_executor;
        return {*sub_executor};
    }

    template<class F>
    void set_on_timeout(F) noexcept
    {
        this->on_timeout = [](BasicExecutor& executor) {
            auto& self = static_cast<TopExecutorImpl&>(executor);
            // TODO ExecutorTimeoutContext
            return self.ctx.invoke(make_lambda<F>(), Executor2TimeoutContext<Ts...>(self));
        };
    }

    TopExecutorImpl(TopExecutorImpl const&) = delete;
    TopExecutorImpl& operator=(TopExecutorImpl const&) = delete;

    template<class... Args>
    TopExecutorImpl(Reactor& /*reactor*/, Args&&... args)
      : Executor2Impl<Ts...>(this->base(), static_cast<Args&&>(args)...)
    {}

    TopExecutorBase& base() noexcept
    {
        return *reinterpret_cast<TopExecutorBase*>(this);
    }

    static TopExecutorImpl* get_top_executor_from_executor(Executor2Impl<Ts...>* p) noexcept
    {
        constexpr auto pad = sizeof(TopExecutorTimers) % alignof(decltype(*p));
        void* d = reinterpret_cast<uint8_t*>(p) - pad - sizeof(TopExecutorTimers);
        return static_cast<TopExecutorImpl*>(d);
    }

    template<class... Args>
    static TopExecutorImpl* New(Reactor& reactor, Args&&... args)
    {
        auto* p = new TopExecutorImpl{reactor, static_cast<Args&&>(args)...};
        p->deleter = [](void* p) {
            delete get_top_executor_from_executor(static_cast<Executor2Impl<Ts...>*>(p));
        };
        assert(get_top_executor_from_executor(static_cast<Executor2Impl<Ts...>*>(p)) == p);

        return p;
    }

private:
    void *operator new(size_t n) { return ::operator new(n); }
};

struct Reactor
{
    template<class... Args>
    TopExecutorBuilder<Args...> create_executor(int /*fd*/, Args&&... args)
    {
        return {this->executors.emplace_back<TopExecutor2<Args...>>(
            *this, static_cast<Args&&>(args)...)};
    }

private:
    Container<TopExecutorBase> executors;
    Container<BasicTimer> timers;
};


namespace detail { namespace {
    template<class... Ts>
    static ExecutorResult terminate_callee(Ts...)
    {
        assert("call a executor marked 'Terminate'");
        return ExecutorResult::Terminate;
    }
} }

bool BasicExecutor::exec()
{
    switch (this->current->exec_action()) {
        case ExecutorResult::ExitSuccess:
            return this->exit_with(ExecutorError::NoError);
        case ExecutorResult::ExitFailure:
            return this->exit_with(ExecutorError::ActionError);
        case ExecutorResult::Terminate:
            this->terminate();
            return false;
            break;
        case ExecutorResult::Nothing:
            break;
    }

    return this->current;
}

void BasicExecutor::terminate()
{
    while (this->current != this) {
        (void)this->current->exec_exit(ExecutorError::Terminate);
        std::exchange(this->current, this->current->prev)->delete_self();
    }
    (void)this->current->exec_exit(ExecutorError::Terminate);
    this->on_action = detail::terminate_callee;
    this->on_exit = detail::terminate_callee;
    //TODO this->on_timeout = detail::terminate_callee;
}

bool BasicExecutor::exit_with(ExecutorError error)
{
    do {
        switch (this->current->exec_exit(error)) {
            case ExecutorResult::ExitSuccess:
                if (this->current == this) {
                    this->on_action = detail::terminate_callee;
                    this->on_exit = detail::terminate_callee;
                    return false;
                }
                std::exchange(this->current, this->current->prev)->delete_self();
                error = ExecutorError::NoError;
                break;
            case ExecutorResult::ExitFailure:
                if (this->current == this) {
                    this->on_action = detail::terminate_callee;
                    this->on_exit = detail::terminate_callee;
                    return false;
                }
                std::exchange(this->current, this->current->prev)->delete_self();
                error = ExecutorError::ActionError;
                break;
            case ExecutorResult::Terminate:
                this->terminate();
                return false;
                break;
            case ExecutorResult::Nothing:
                return true;
        }
    } while (this->current);
    return false;
}
