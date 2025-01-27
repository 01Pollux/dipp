#pragma once

#include "concepts.hpp"
#include "dependency.hpp"

namespace dipp
{
    template<typename Ty, service_lifetime Lifetime, dependency_container_type DepsTy> class base_service_descriptor
    {
    public:
        using service_type =
            std::conditional_t<Lifetime == service_lifetime::singleton || Lifetime == service_lifetime::scoped,
                               std::reference_wrapper<std::remove_reference_t<Ty>>, Ty>;

        using dependency_types = typename DepsTy::types;
    };

    //

    template<typename Ty, service_lifetime Lifetime, service_scope_type ScopeTy,
             dependency_container_type DepsTy = dependency<>>
    class functor_service_descriptor : public base_service_descriptor<Ty, Lifetime, DepsTy>
    {
    public:
        using value_type = Ty;
        using scope_type = ScopeTy;
#if _HAS_CXX23
        using functor_type = std::move_only_function<value_type(scope_type& scope)>;
#else
        using functor_type = std::function<value_type(scope_type& scope)>;
#endif

        static constexpr service_lifetime lifetime = Lifetime;

        template<typename FnTy>
            requires std::is_invocable_v<FnTy, scope_type&>
        constexpr functor_service_descriptor(FnTy functor) noexcept(
            std::is_nothrow_move_constructible_v<functor_type>) : m_Functor(std::move(functor))
        {
        }

        constexpr value_type load(scope_type& scope) noexcept(std::is_nothrow_invocable_v<functor_type, scope_type&>)
        {
            return m_Functor(scope);
        }

    private:
        functor_type m_Functor{};
    };

    //

    template<typename Ty, service_scope_type ScopeTy> class extern_service_descriptor
    {
    public:
        using value_type   = std::reference_wrapper<Ty>;
        using service_type = std::reference_wrapper<Ty>;
        using scope_type   = ScopeTy;

        static constexpr service_lifetime lifetime = service_lifetime::singleton;

        constexpr extern_service_descriptor(Ty& service) : m_Service(service)
        {
        }

        constexpr value_type load(scope_type&) noexcept
        {
            return m_Service;
        }

    private:
        value_type m_Service;
    };

    //

    template<typename Ty, service_scope_type ScopeTy> class const_extern_service_descriptor
    {
    public:
        using value_type   = std::reference_wrapper<const Ty>;
        using service_type = std::reference_wrapper<const Ty>;
        using scope_type   = ScopeTy;

        static constexpr service_lifetime lifetime = service_lifetime::singleton;

        constexpr const_extern_service_descriptor(const Ty& service) : m_Service(service)
        {
        }

        constexpr value_type load(scope_type&) noexcept
        {
            return m_Service;
        }

    private:
        value_type m_Service{};
    };

    //

    template<typename Ty, service_scope_type ScopeTy> class extern_shared_service_descriptor
    {
    public:
        using value_type   = std::shared_ptr<Ty>;
        using service_type = std::shared_ptr<Ty>;
        using scope_type   = ScopeTy;

        static constexpr service_lifetime lifetime = service_lifetime::singleton;

        constexpr extern_shared_service_descriptor(value_type service) : m_Service(service)
        {
        }

        value_type load(scope_type&) noexcept
        {
            return m_Service;
        }

    private:
        value_type m_Service;
    };

    //

    template<typename Ty, service_lifetime Lifetime, service_scope_type ScopeTy,
             dependency_container_type DepsTy = dependency<>>
    class unique_service_descriptor : public functor_service_descriptor<std::unique_ptr<Ty>, Lifetime, ScopeTy, DepsTy>
    {
    public:
        using base_class = functor_service_descriptor<std::unique_ptr<Ty>, Lifetime, ScopeTy, DepsTy>;

        template<typename FnTy>
            requires std::is_invocable_v<FnTy, ScopeTy&>
        constexpr unique_service_descriptor(FnTy functor) noexcept(std::is_nothrow_move_constructible_v<FnTy>) :
            base_class(std::move(functor))
        {
        }

        template<typename... ArgsTy>
            requires(!std::is_abstract_v<Ty>)
        constexpr unique_service_descriptor(ArgsTy&&... args) :
            base_class(
                [args = std::make_tuple(std::forward<ArgsTy>(args)...)](ScopeTy& scope) mutable ->
                typename base_class::value_type
                {
                    if constexpr (std::tuple_size_v<typename DepsTy::types> == 0)
                    {
                        (void)scope;
                        return std::apply([](auto&&... args)
                                          { return std::make_unique<Ty>(std::forward<decltype(args)>(args)...); },
                                          std::move(args));
                    }
                    else
                    {
                        return std::apply(
                            [](auto&&... args) { return std::make_unique<Ty>(std::forward<decltype(args)>(args)...); },
                            std::tuple_cat(get_tuple_from_scope<ScopeTy, DepsTy>(scope), std::move(args)));
                    }
                })
        {
        }
    };

    //

    template<typename Ty, service_lifetime Lifetime, service_scope_type ScopeTy,
             dependency_container_type DepsTy = dependency<>>
    class shared_service_descriptor : public functor_service_descriptor<std::shared_ptr<Ty>, Lifetime, ScopeTy, DepsTy>
    {
    public:
        using base_class = functor_service_descriptor<std::shared_ptr<Ty>, Lifetime, ScopeTy, DepsTy>;

        template<typename FnTy>
            requires std::is_invocable_v<FnTy, ScopeTy&>
        constexpr shared_service_descriptor(FnTy functor) noexcept(std::is_nothrow_move_constructible_v<FnTy>) :
            base_class(std::move(functor))
        {
        }

        template<typename... ArgsTy>
            requires(!std::is_abstract_v<Ty>)
        constexpr shared_service_descriptor(ArgsTy&&... args) :
            base_class(
                [args = std::make_tuple(std::forward<ArgsTy>(args)...)](ScopeTy& scope) mutable ->
                typename base_class::value_type
                {
                    if constexpr (std::tuple_size_v<typename DepsTy::types> == 0)
                    {
                        (void)scope;
                        return std::apply([](auto&&... args)
                                          { return std::make_shared<Ty>(std::forward<decltype(args)>(args)...); },
                                          std::move(args));
                    }
                    else
                    {
                        return std::apply(
                            [](auto&&... args) { return std::make_shared<Ty>(std::forward<decltype(args)>(args)...); },
                            std::tuple_cat(get_tuple_from_scope<ScopeTy, DepsTy>(scope), std::move(args)));
                    }
                })
        {
        }
    };

    //

    template<typename Ty, service_lifetime Lifetime, service_scope_type ScopeTy,
             dependency_container_type DepsTy = dependency<>>
    class local_service_descriptor : public functor_service_descriptor<Ty, Lifetime, ScopeTy, DepsTy>
    {
    public:
        using base_class = functor_service_descriptor<Ty, Lifetime, ScopeTy, DepsTy>;

        template<typename FnTy>
            requires std::is_invocable_v<FnTy, ScopeTy&>
        constexpr local_service_descriptor(FnTy functor) noexcept(std::is_nothrow_move_constructible_v<FnTy>) :
            base_class(std::move(functor))
        {
        }

        template<typename... ArgsTy>
            requires(!std::is_abstract_v<Ty>)
        constexpr local_service_descriptor(ArgsTy&&... args) :
            base_class(
                [args = std::make_tuple(std::forward<ArgsTy>(args)...)](ScopeTy& scope) mutable ->
                typename base_class::value_type
                {
                    if constexpr (std::tuple_size_v<typename DepsTy::types> == 0)
                    {
                        (void)scope;
                        return std::apply(
                            [](auto&&... args) { return Ty(std::forward<decltype(args)>(args)...); }, std::move(args));
                    }
                    else
                    {
                        return std::apply(
                            [](auto&&... args) { return Ty(std::forward<decltype(args)>(args)...); },
                            std::tuple_cat(get_tuple_from_scope<ScopeTy, DepsTy>(scope), std::move(args)));
                    }
                })
        {
        }
    };
} // namespace dipp