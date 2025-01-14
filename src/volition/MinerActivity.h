// Copyright (c) 2017-2018 Cryptogogue, Inc. All Rights Reserved.
// http://cryptogogue.com

#ifndef VOLITION_MINERACTIVITY_H
#define VOLITION_MINERACTIVITY_H

#include <volition/common.h>
#include <volition/Miner.h>

namespace Volition {

//================================================================//
// MinerActivity
//================================================================//
class MinerActivity :
    public Miner,
    public Poco::Activity < MinerActivity > {
private:

    u32                 mFixedUpdateDelayInMillis;
    u32                 mVariableUpdateDelayInMillis;
    Poco::Event         mShutdownEvent;

    //----------------------------------------------------------------//
    void                runActivity                 ();

    //----------------------------------------------------------------//
    void                Miner_reset                 () override;
    void                Miner_shutdown              ( bool kill ) override;

public:

    static const u32    DEFAULT_FIXED_UPDATE_MILLIS         = 1000;
    static const u32    DEFAULT_VARIABLE_UPDATE_MILLIS      = 10000;

    GET_SET ( u32,      FixedUpdateDelayInMillis,       mFixedUpdateDelayInMillis )
    GET_SET ( u32,      VariableUpdateDelayInMillis,    mVariableUpdateDelayInMillis )

    //----------------------------------------------------------------//
                        MinerActivity               ();
                        ~MinerActivity              ();
    void                waitForShutdown             ();
};

} // namespace Volition
#endif
