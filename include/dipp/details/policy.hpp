#pragma once

#include <concepts>
#include <any>
#include <map>

#include "concepts.hpp"
#include "type_key_pair.hpp"
#include "move_only_any.hpp"

#include "service_info.hpp"
#include "instance_info.hpp"

namespace dipp::details
{
    struct default_service_policy
    {
        using service_map_type = std::map<type_key_pair, service_info>;
    };
    static_assert(service_policy_type<default_service_policy>,
                  "default_service_policy is not a service_policy_type");

    //

    struct default_service_storage_memory_type
    {
    public:
        using instance_map_type = std::map<type_key_pair, instance_info>;

    public:
        default_service_storage_memory_type() = default;

        default_service_storage_memory_type(const default_service_storage_memory_type&) = delete;
        default_service_storage_memory_type& operator=(const default_service_storage_memory_type&) =
            delete;

        default_service_storage_memory_type(default_service_storage_memory_type&&) = default;
        default_service_storage_memory_type& operator=(default_service_storage_memory_type&&) =
            default;

        ~default_service_storage_memory_type()
        {
            // destroy all instances in reverse order
            for (auto it = m_Instances.rbegin(); it != m_Instances.rend(); ++it)
            {
                it->reset();
            }
        }

    public:
        /// <summary>
        /// Finds the instance by the handle.
        /// </summary>
        [[nodiscard]]
        auto find(const type_key_pair& handle)
        {
            auto it = m_InstanceRefs.find(handle);
            return it != m_InstanceRefs.end() ? it->second : nullptr;
        }

        /// <summary>
        /// Emplace a new instance into the storage.
        /// </summary>
        template<service_descriptor_type DescTy, service_scope_type ScopeTy>
        [[nodiscard]]
        auto emplace(const type_key_pair& handle, DescTy& descriptor, ScopeTy& scope)
        {
            auto instance = std::make_unique<instance_info>(descriptor, scope);
            auto instance_ptr = instance.get();

            m_Instances.emplace_back(std::move(instance));
            auto it = m_InstanceRefs.emplace(handle, instance_ptr).first;

            return it->second;
        }

    private:
        std::vector<std::unique_ptr<instance_info>> m_Instances;
        std::map<type_key_pair, instance_info*> m_InstanceRefs;
    };
    static_assert(service_storage_memory_type<default_service_storage_memory_type>,
                  "default_service_storage_memory_type is not a service_storage_memory_type");
}