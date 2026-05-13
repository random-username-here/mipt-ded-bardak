#pragma once

#include <algorithm>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace modlib {

struct AbilityDef {
    std::string id;

    explicit AbilityDef(std::string id_)
        : id(std::move(id_))
    {}
};

struct ItemDef {
    std::string id;
    std::vector<AbilityDef> abilities;

    ItemDef(std::string id_, std::vector<AbilityDef> abilities_)
        : id(std::move(id_))
        , abilities(std::move(abilities_))
    {}
};

class Inventory {
    std::vector<ItemDef> m_items;

public:
    void addItem(ItemDef item)
    {
        m_items.push_back(std::move(item));
    }

    const std::vector<ItemDef> &items() const
    {
        return m_items;
    }

    bool hasAbility(std::string_view ability) const
    {
        for (const auto &item : m_items) {
            for (const auto &owned : item.abilities) {
                if (owned.id == ability) {
                    return true;
                }
            }
        }

        return false;
    }

    std::vector<AbilityDef> abilities() const
    {
        std::vector<AbilityDef> out;

        for (const auto &item : m_items) {
            for (const auto &ability : item.abilities) {
                const auto already = std::find_if(
                    out.begin(),
                    out.end(),
                    [&](const AbilityDef &known) {
                        return known.id == ability.id;
                    }
                );

                if (already == out.end()) {
                    out.push_back(ability);
                }
            }
        }

        return out;
    }
};

} // namespace modlib
