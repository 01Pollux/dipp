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

    public:
        template<typename ScopeTy, typename TupleTy>
        [[nodiscard]] static auto GetCombinedArgs(ScopeTy& scope, TupleTy&& args)
        {
            if constexpr (std::tuple_size_v<typename DepsTy::types> == 0)
            {
                (void)scope;
                return std::forward<TupleTy>(args);
            }
            else
            {
                return std::tuple_cat(get_tuple_from_scope<ScopeTy, DepsTy>(scope), std::forward<TupleTy>(args));
            }
        }

        template<typename ScopeTy, typename FactoryTy, typename TupleTy>
        [[nodiscard]] static auto ApplyFactory(ScopeTy& scope, FactoryTy&& factory, TupleTy&& args)
        {
            return std::apply(std::forward<FactoryTy>(factory), GetCombinedArgs(scope, std::forward<TupleTy>(args)));
        }
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
        using functor_type = std::move_only_function<move_only_any(scope_type& scope)>;
#else
        using functor_type = std::function<move_only_any(scope_type& scope)>;
#endif

        static constexpr service_lifetime lifetime = Lifetime;

        constexpr functor_service_descriptor(functor_type functor) noexcept(
            std::is_nothrow_move_constructible_v<functor_type>) : m_Functor(std::move(functor))
        {
        }

        constexpr move_only_any load(scope_type& scope) noexcept(std::is_nothrow_invocable_v<functor_type, scope_type&>)
        {
            return m_Functor(scope);
        }

    private:
        functor_type m_Functor{};
    };

    //

    template<typename Ty, service_lifetime Lifetime, service_scope_type ScopeTy,
             dependency_container_type DepsTy = dependency<>>
    class unique_service_descriptor : public functor_service_descriptor<std::unique_ptr<Ty>, Lifetime, ScopeTy, DepsTy>
    {
    public:
        using base_class   = functor_service_descriptor<std::unique_ptr<Ty>, Lifetime, ScopeTy, DepsTy>;
        using value_type   = typename base_class::value_type;
        using functor_type = typename base_class::functor_type;

        using base_class::ApplyFactory;

        explicit constexpr unique_service_descriptor(functor_type functor) noexcept(
            std::is_nothrow_move_constructible_v<functor_type>) : base_class(std::move(functor))
        {
        }

        template<typename... ArgsTy>
            requires(!std::is_abstract_v<Ty>)
        constexpr unique_service_descriptor(std::in_place_t, ArgsTy&&... args) :
            base_class(
                [args = std::forward_as_tuple(std::forward<ArgsTy>(args)...)](ScopeTy& scope) mutable
                {
                    return base_class::ApplyFactory(
                        scope,
                        [](auto&&... params) mutable
                        {
                            return make_any<std::unique_ptr<Ty>>(
                                std::make_unique<Ty>(std::forward<decltype(params)>(params)...));
                        },
                        std::move(args));
                })
        {
        }

        constexpr unique_service_descriptor() : unique_service_descriptor(std::in_place)
        {
        }
    };

    //

    template<typename Ty, service_lifetime Lifetime, service_scope_type ScopeTy,
             dependency_container_type DepsTy = dependency<>>
    class shared_service_descriptor : public functor_service_descriptor<std::shared_ptr<Ty>, Lifetime, ScopeTy, DepsTy>
    {
    public:
        using base_class   = functor_service_descriptor<std::shared_ptr<Ty>, Lifetime, ScopeTy, DepsTy>;
        using value_type   = typename base_class::value_type;
        using functor_type = typename base_class::functor_type;

        using base_class::ApplyFactory;

        constexpr shared_service_descriptor(functor_type functor) noexcept(
            std::is_nothrow_move_constructible_v<functor_type>) : base_class(std::move(functor))
        {
        }

        template<typename... ArgsTy>
            requires(!std::is_abstract_v<Ty>)
        constexpr shared_service_descriptor(std::in_place_t, ArgsTy&&... args) :
            base_class(
                [args = std::forward_as_tuple(std::forward<ArgsTy>(args)...)](ScopeTy& scope) mutable
                {
                    return base_class::ApplyFactory(
                        scope,
                        [](auto&&... params) mutable
                        {
                            return make_any<std::shared_ptr<Ty>>(
                                std::make_shared<Ty>(std::forward<decltype(params)>(params)...));
                        },
                        std::move(args));
                })
        {
        }

        constexpr shared_service_descriptor() : shared_service_descriptor(std::in_place)
        {
        }
    };

    //

    template<typename Ty, service_lifetime Lifetime, service_scope_type ScopeTy,
             dependency_container_type DepsTy = dependency<>>
    class local_service_descriptor : public functor_service_descriptor<Ty, Lifetime, ScopeTy, DepsTy>
    {
    public:
        using base_class   = functor_service_descriptor<Ty, Lifetime, ScopeTy, DepsTy>;
        using value_type   = typename base_class::value_type;
        using functor_type = typename base_class::functor_type;

        using base_class::ApplyFactory;

        constexpr local_service_descriptor(functor_type functor) noexcept(
            std::is_nothrow_move_constructible_v<functor_type>) : base_class(std::move(functor))
        {
        }

        template<typename... ArgsTy>
            requires(!std::is_abstract_v<Ty>)
        constexpr local_service_descriptor(std::in_place_t, ArgsTy&&... args) :
            base_class(
                [args = std::forward_as_tuple(std::forward<ArgsTy>(args)...)](ScopeTy& scope) mutable
                {
                    return base_class::ApplyFactory(
                        scope, [](auto&&... params) mutable
                        { return dipp::make_any<Ty>(std::forward<decltype(params)>(params)...); }, std::move(args));
                })
        {
        }

        constexpr local_service_descriptor() : local_service_descriptor(std::in_place)
        {
        }
    };
} // namespace dipp