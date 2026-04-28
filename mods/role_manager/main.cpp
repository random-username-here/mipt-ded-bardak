#include "RoleMgr.hpp"
#include "binmsg.hpp"
#include "modlib_manager.hpp"
#include "role_proto.hpp"

#include <exception>
#include <iostream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>

class RoleMgrImpl final : public modlib::RoleMgr {
public:
    std::string_view id() const override { return "sevsol.bardak.role_mgr"; }
    std::string_view brief() const override { return "Client role selection manager"; }
    ModVersion version() const override { return ModVersion(0, 1, 0); }

    bool registerRole(
        std::string_view id,
        std::string_view name,
        std::string_view prefix,
        OnSelected onSelected
    ) override {
        if (id.empty() || id.size() > 64 || roles_.count(std::string(id)) != 0) {
            return false;
        }

        RegisteredRole role;
        role.info.id = std::string(id);
        role.info.name = std::string(name);
        role.info.prefix = std::string(prefix);
        role.onSelected = std::move(onSelected);
        roles_.emplace(role.info.id, std::move(role));
        return true;
    }

    std::string_view roleOf(modlib::BmClient* client) const override {
        const auto selected = selected_.find(client->id());
        if (selected == selected_.end()) {
            return {};
        }

        return selected->second;
    }

    bool clientHasRole(modlib::BmClient* client, std::string_view roleId) const override {
        return roleOf(client) == roleId;
    }

    bool selectRole(modlib::BmClient* client, std::string_view roleId) override {
        const auto role = roles_.find(std::string(roleId));
        if (role == roles_.end() || selected_.count(client->id()) != 0) {
            return false;
        }

        selected_[client->id()] = role->second.info.id;
        try {
            role->second.onSelected(client);
        } catch (...) {
            selected_.erase(client->id());
            throw;
        }

        return true;
    }

    void forEachRole(const RoleVisitor& visitor) const override {
        for (const auto& [_, role] : roles_) {
            visitor(role.info);
        }
    }

    void onSetup(modlib::BmServer* server) override {
        if (!server->registerPrefix("role", this)) {
            throw ModManager::Error("failed to register role prefix");
        }
    }

    void onConnect(modlib::BmClient* client) override {
        sendRoleOptions(client);
    }

    void onDisconnect(modlib::BmClient* client) override {
        selected_.erase(client->id());
    }

    void onMessage(modlib::BmClient* client, bmsg::RawMessage msg) override {
        if (!msg.isCorrect() || msg.header()->type != "choose") {
            return;
        }

        const auto choose = bmsg::CL_role_choose::decode(msg);
        if (!choose) {
				std::cerr << "[ROLE MANAGER] REJECT - " << "bad choose body" << std::endl;
            sendReject(client, "bad choose body");
            return;
        }

		std::cerr << "[ROLE MANAGER] CHOOSE REQUEST - " << choose->id << std::endl;

        try {
            if (!selectRole(client, choose->id)) {
				std::cerr << "[ROLE MANAGER] REJECT - " << "role unavailable" << std::endl;
                sendReject(client, "role unavailable");
                return;
            }
        } catch (const std::exception& err) {
			std::cerr << "[ROLE MANAGER]: REJECT - " << err.what() << std::endl;
            sendReject(client, err.what());
            return;
        }

        client->send(bmsg::SV_role_chosen { choose->id });
    }

private:
    struct RegisteredRole {
        modlib::ClientRoleInfo info;
        OnSelected onSelected;
    };

    std::unordered_map<std::string, RegisteredRole> roles_;
    std::unordered_map<size_t, std::string> selected_;

    void sendRoleOptions(modlib::BmClient* client) const {
        forEachRole([client](const modlib::ClientRoleInfo& role) {
            client->send(bmsg::SV_role_option { role.id, role.name, role.prefix });
        });
    }

    void sendReject(modlib::BmClient* client, std::string_view reason) const {
        client->send(bmsg::SV_role_reject { reason });
    }
};

extern "C" Mod* modlib_create(ModManager*) {
    return new RoleMgrImpl();
}
