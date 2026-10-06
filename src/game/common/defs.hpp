#pragma once
#include "binmsg.hpp"

enum SyncvIdx : bmsg::Id {
    SYNCV_POS_X = 0,
    SYNCV_POS_Y = 1,
    SYNCV_POS_Z = 2,
    SYNCV_AZIMUTH = 3,
    SYNCV_ELEV = 4
};

