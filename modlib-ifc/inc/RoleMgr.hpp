#pragma once

#include "BmServerModule.hpp"

#include <functional>
#include <string>
#include <string_view>

namespace modlib {

struct ClientRoleInfo {
    std::string id;
    std::string name;
    std::string prefix;
};

class RoleMgr : public BmServerModule {
public:
    using OnSelected = std::function<void(BmClient*)>;
    using RoleVisitor = std::function<void(const ClientRoleInfo&)>;

    virtual bool registerRole(
        std::string_view id,
        std::string_view name,
        std::string_view prefix,
        OnSelected onSelected
    ) = 0;

    virtual std::string_view roleOf(BmClient* client) const = 0;
    virtual bool clientHasRole(BmClient* client, std::string_view roleId) const = 0;
    virtual bool selectRole(BmClient* client, std::string_view roleId) = 0;
    virtual void forEachRole(const RoleVisitor& visitor) const = 0;

    ~RoleMgr() override = default;
};

} // namespace modlib
