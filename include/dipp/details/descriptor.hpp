#pragma once

#include <functional>
#include "apply.hpp"

namespace dipp
{
    template<typename Ty, service_lifetime Lifetime, dependency_container_type DepsTy>
    struct base_service_descriptor
    {
    public:
        using service_type = std::conditional_t<Lifetime == service_lifetime::singleton ||
                                                    Lifetime == service_lifetime::scoped,
                                                std::reference_wrapper<std::remove_reference_t<Ty>>,
                                                Ty>;

        using dependency_type = DepsTy;
    };

    //

    template<typename Ty,
             service_lifetime Lifetime,
             service_scope_type ScopeTy,
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

    public:
        constexpr functor_service_descriptor(functor_type functor) noexcept(
            std::is_nothrow_move_constructible_v<functor_type>)
            : m_Functor(std::move(functor))
        {
        }

    public:
        constexpr move_only_any load(scope_type& scope) noexcept(
            std::is_nothrow_invocable_v<functor_type, scope_type&>)
        {
            return m_Functor(scope);
        }

    private:
        functor_type m_Functor{};
    };

    //

    template<typename Ty,
             service_lifetime Lifetime,
             service_scope_type ScopeTy,
             dependency_container_type DepsTy = dependency<>>
    class unique_service_descriptor
        : public functor_service_descriptor<std::unique_ptr<Ty>, Lifetime, ScopeTy, DepsTy>
    {
    public:
        using base_class =
            functor_service_descriptor<std::unique_ptr<Ty>, Lifetime, ScopeTy, DepsTy>;
        using value_type = typename base_class::value_type;
        using functor_type = typename base_class::functor_type;

        constexpr unique_service_descriptor(functor_type functor) noexcept(
            std::is_nothrow_move_constructible_v<functor_type>)
            : base_class(std::move(functor))
        {
        }

    public:
        template<typename ImplTy = Ty, typename... ArgsTy>
            requires(!std::is_abstract_v<ImplTy> && std::derived_from<ImplTy, Ty>)
        [[nodiscard]] static auto factory(ArgsTy&&... args)
        {
            return unique_service_descriptor(
                [args = std::make_tuple(std::forward<ArgsTy>(args)...)](ScopeTy& scope) mutable
                {
                    return dipp::apply<DepsTy>(
                        scope,
                        [](auto&&... params) mutable
                        {
                            return dipp::make_any<std::unique_ptr<Ty>>(std::make_unique<ImplTy>(
                                std::forward<decltype(params)>(params)...));
                        },
                        std::move(args));
                });
        }

        template<service_descriptor_type DescTy, typename... ArgsTy>
        [[nodiscard]] static auto factory(ArgsTy&&... args)
        {
            using implementation_type = typename DescTy::value_type;
            using implementation_dependency_type = typename DescTy::dependency_type;

            return unique_service_descriptor(
                [args = std::make_tuple(std::forward<ArgsTy>(args)...)](ScopeTy& scope) mutable
                {
                    return dipp::apply<implementation_dependency_type>(
                        scope,
                        [](auto&&... params) mutable
                        {
                            return dipp::make_any<std::unique_ptr<Ty>>(
                                std::make_unique<implementation_type>(
                                    std::forward<decltype(params)>(params)...));
                        },
                        std::move(args));
                });
        }
    };

    //

    template<typename Ty,
             service_lifetime Lifetime,
             service_scope_type ScopeTy,
             dependency_container_type DepsTy = dependency<>>
    class shared_service_descriptor
        : public functor_service_descriptor<std::shared_ptr<Ty>, Lifetime, ScopeTy, DepsTy>
    {
    public:
        using base_class =
            functor_service_descriptor<std::shared_ptr<Ty>, Lifetime, ScopeTy, DepsTy>;
        using value_type = typename base_class::value_type;
        using functor_type = typename base_class::functor_type;

        constexpr shared_service_descriptor(functor_type functor) noexcept(
            std::is_nothrow_move_constructible_v<functor_type>)
            : base_class(std::move(functor))
        {
        }

    public:
        template<typename ImplTy = Ty, typename... ArgsTy>
            requires(!std::is_abstract_v<ImplTy> && std::derived_from<ImplTy, Ty>)
        [[nodiscard]] static auto factory(ArgsTy&&... args)
        {
            return shared_service_descriptor(
                [args = std::make_tuple(std::forward<ArgsTy>(args)...)](ScopeTy& scope) mutable
                {
                    return dipp::apply<DepsTy>(
                        scope,
                        [](auto&&... params) mutable
                        {
                            return dipp::make_any<std::shared_ptr<Ty>>(std::make_shared<ImplTy>(
                                std::forward<decltype(params)>(params)...));
                        },
                        std::move(args));
                });
        }

        template<service_descriptor_type DescTy, typename... ArgsTy>
        [[nodiscard]] static auto factory(ArgsTy&&... args)
        {
            using implementation_type = typename DescTy::value_type;
            using implementation_dependency_type = typename DescTy::dependency_type;

            return shared_service_descriptor(
                [args = std::make_tuple(std::forward<ArgsTy>(args)...)](ScopeTy& scope) mutable
                {
                    return dipp::apply<implementation_dependency_type>(
                        scope,
                        [](auto&&... params) mutable
                        {
                            return dipp::make_any<std::shared_ptr<Ty>>(
                                std::make_shared<implementation_type>(
                                    std::forward<decltype(params)>(params)...));
                        },
                        std::move(args));
                });
        }
    };

    //

    template<typename Ty,
             service_lifetime Lifetime,
             service_scope_type ScopeTy,
             dependency_container_type DepsTy = dependency<>>
    class local_service_descriptor
        : public functor_service_descriptor<Ty, Lifetime, ScopeTy, DepsTy>
    {
    public:
        using base_class = functor_service_descriptor<Ty, Lifetime, ScopeTy, DepsTy>;
        using value_type = typename base_class::value_type;
        using functor_type = typename base_class::functor_type;

        constexpr local_service_descriptor(functor_type functor) noexcept(
            std::is_nothrow_move_constructible_v<functor_type>)
            : base_class(std::move(functor))
        {
        }

    public:
        template<typename ImplTy = Ty, typename... ArgsTy>
            requires(!std::is_abstract_v<ImplTy>)
        [[nodiscard]] static auto factory(ArgsTy&&... args)
        {
            return local_service_descriptor(
                [args = std::make_tuple(std::forward<ArgsTy>(args)...)](ScopeTy& scope) mutable
                {
                    return dipp::apply<DepsTy>(
                        scope,
                        [](auto&&... params) mutable
                        { return dipp::make_any<Ty>(std::forward<decltype(params)>(params)...); },
                        std::move(args));
                });
        }
    };
} // namespace dipp