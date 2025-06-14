#pragma once

#include "storage.hpp"
#include "policy.hpp"
#include "string_hash.hpp"

namespace dipp
{
    template<service_policy_type StoragePolicyTy,
             service_storage_memory_type SingletonPolicyTy,
             service_storage_memory_type ScopedPolicyTy>
    class service_scope
    {
    public:
        using storage_type = service_storage<StoragePolicyTy>;
        using singleton_storage_type = SingletonPolicyTy;
        using scoped_storage_type = ScopedPolicyTy;

    public:
        service_scope(storage_type* storage, singleton_storage_type* singleton_storage)
            : m_Storage(storage)
            , m_SingletonStorage(singleton_storage)
        {
        }

        service_scope(storage_type* storage,
                      singleton_storage_type* singleton_storage,
                      service_scope&& scope)
            : m_Storage(storage)
            , m_SingletonStorage(singleton_storage)
            , m_LocalStorage(std::move(scope.m_LocalStorage))
        {
        }

        service_scope(const service_scope&) = delete;
        service_scope& operator=(const service_scope&) = delete;

        service_scope(service_scope&&) = default;
        service_scope& operator=(service_scope&&) = default;

        ~service_scope() = default;

    public:
        /// <summary>
        /// Get a service from the storage.
        /// </summary>
        template<base_injected_type InjectableTy>
        [[nodiscard]] auto get() -> result<InjectableTy>
        {
            return m_Storage->template get_service<InjectableTy>(
                *this, *m_SingletonStorage, m_LocalStorage);
        }

    public:
        /// <summary>
        /// Check if a service is registered in the storage.
        /// </summary>
        template<base_injected_type InjectableTy>
        [[nodiscard]] bool has() const noexcept
        {
            using descriptor_type = typename InjectableTy::descriptor_type;
            return m_Storage->template has_service<descriptor_type>(InjectableTy::key);
        }

    public:
        /// <summary>
        /// Count the number of services registered in the storage.
        /// </summary>
        template<base_injected_type InjectableTy>
        [[nodiscard]] size_t count() const noexcept
        {
            using descriptor_type = typename InjectableTy::descriptor_type;
            return m_Storage->template count<descriptor_type>(InjectableTy::key);
        }

    public:
        /// <summary>
        /// Count the number of services registered in the storage.
        /// </summary>
        template<base_injected_type InjectableTy>
        [[nodiscard]] size_t count_all() const noexcept
        {
            using descriptor_type = typename InjectableTy::descriptor_type;
            return m_Storage->template count_all<descriptor_type>();
        }

    public:
        /// <summary>
        /// Get a service from the storage.
        /// </summary>
        template<base_injected_type InjectableTy, typename FuncTy>
        [[nodiscard]] void for_each(FuncTy&& func)
        {
            using descriptor_type = typename InjectableTy::descriptor_type;
            m_Storage->template for_each<descriptor_type>(std::forward<FuncTy>(func),
                                                          *this,
                                                          *m_SingletonStorage,
                                                          m_LocalStorage,
                                                          InjectableTy::key);
        }

    public:
        /// <summary>
        /// Get a service from the storage.
        /// </summary>
        template<base_injected_type InjectableTy, typename FuncTy>
        [[nodiscard]] void for_each_all(FuncTy&& func)
        {
            using descriptor_type = typename InjectableTy::descriptor_type;
            m_Storage->template for_each_all<descriptor_type>(
                std::forward<FuncTy>(func), *this, *m_SingletonStorage, m_LocalStorage);
        }

    private:
        storage_type* m_Storage;
        singleton_storage_type* m_SingletonStorage;
        scoped_storage_type m_LocalStorage;
    };

    using default_service_scope = service_scope<default_service_policy,
                                                default_service_storage_memory_type,
                                                default_service_storage_memory_type>;

    static_assert(service_scope_type<default_service_scope>,
                  "default_service_scope is not a service_scope_type");
} // namespace dipp