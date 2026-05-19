#pragma once

#include <cstdint>

// Int protocol between adapter and script (comp-lang via sock_send_int / sock_recv_int).
// Adapter contains no gameplay logic — only translation and I/O.

namespace brain_proto {

constexpr int32_t kDefaultPort = 17771;

// adapter -> script
constexpr int32_t kEvtTick         = 1;
constexpr int32_t kEvtHp           = 2;
constexpr int32_t kEvtAt           = 3;
constexpr int32_t kEvtRoot         = 4;
constexpr int32_t kEvtEnemy        = 5;
constexpr int32_t kEvtWall         = 6;
constexpr int32_t kEvtAbilitySlash = 7;

// script -> adapter
constexpr int32_t kActNone  = 0;
constexpr int32_t kActMove  = 1;
constexpr int32_t kActUse   = 2;
constexpr int32_t kActStop  = 3;

} // namespace brain_proto
