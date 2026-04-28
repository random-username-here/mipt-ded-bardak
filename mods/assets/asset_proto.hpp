#pragma once
#include "libpan_cxx_macros.hpp"

// Hand-written protocol for asset manager module.
// Uses libpan macros for fixed-size messages and a custom encoder for binary payloads.

PAN_GH_DEFS_BEGIN

// Client asks server to send tile image bytes for given tile type id.
PAN_GH_MSG(PAN_GH_CLIENT, ast, tile, "ast", "tile",
    uint32_t typeId;
)
PAN_GH_DECODE(PAN_GH_CLIENT, ast, tile, "ast", "tile",
    PAN_GH_READ(&self->typeId, sizeof(self->typeId));
)
PAN_GH_ENCODE(PAN_GH_CLIENT, ast, tile, "ast", "tile",
    uint16_t len = 0;
    len += sizeof(self->typeId);
    PAN_GH_HEADER("ast", "tile", len);
    PAN_GH_WRITE(&self->typeId, sizeof(self->typeId));
)

// Client asks server to send unit image bytes for given unit type id.
PAN_GH_MSG(PAN_GH_CLIENT, ast, unit, "ast", "unit",
    uint32_t typeId;
)
PAN_GH_DECODE(PAN_GH_CLIENT, ast, unit, "ast", "unit",
    PAN_GH_READ(&self->typeId, sizeof(self->typeId));
)
PAN_GH_ENCODE(PAN_GH_CLIENT, ast, unit, "ast", "unit",
    uint16_t len = 0;
    len += sizeof(self->typeId);
    PAN_GH_HEADER("ast", "unit", len);
    PAN_GH_WRITE(&self->typeId, sizeof(self->typeId));
)

// Server tells client requested asset is missing on server.
PAN_GH_MSG(PAN_GH_SERVER, ast, missing, "ast", "missing",
    uint8_t  kind;   // 0=tile, 1=unit
    uint32_t typeId;
)
PAN_GH_DECODE(PAN_GH_SERVER, ast, missing, "ast", "missing",
    PAN_GH_READ(&self->kind, sizeof(self->kind));
    PAN_GH_READ(&self->typeId, sizeof(self->typeId));
)
PAN_GH_ENCODE(PAN_GH_SERVER, ast, missing, "ast", "missing",
    uint16_t len = 0;
    len += sizeof(self->kind);
    len += sizeof(self->typeId);
    PAN_GH_HEADER("ast", "missing", len);
    PAN_GH_WRITE(&self->kind, sizeof(self->kind));
    PAN_GH_WRITE(&self->typeId, sizeof(self->typeId));
)

PAN_GH_DEFS_END

