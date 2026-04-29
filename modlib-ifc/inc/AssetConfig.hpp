#pragma once

#include <string_view>

namespace modlib::asset_config {

// Default texture paths used by gameplay modules.
// Projects can override usage at module level if needed.
inline constexpr std::string_view kPersonUnitTexturePath = "assets/units/person.png";

} // namespace modlib::asset_config

