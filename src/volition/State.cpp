//
//  main.cpp
// Copyright (c) 2017-2018 Cryptogogue, Inc. All Rights Reserved.
// http://cryptogogue.com

#include <volition/State.h>

namespace Volition {

//================================================================//
// State
//================================================================//

//----------------------------------------------------------------//
const MinerInfo* State::getMinerInfo ( string minerID ) const {

    map < string, MinerInfo >::const_iterator minerInfoIt = this->mMinerInfo.find ( minerID );
    if ( minerInfoIt != this->mMinerInfo.end ()) {
        return &minerInfoIt->second;
    }
    return NULL;
}

//----------------------------------------------------------------//
map < string, string > State::getMinerURLs () const {

    return this->mMinerURLs;
}

//----------------------------------------------------------------//
void State::registerMiner ( const MinerInfo& minerInfo ) {

    this->mMinerInfo.insert ( pair < string, MinerInfo >( minerInfo.getMinerID (), minerInfo ));
    this->mMinerURLs [ minerInfo.getMinerID ()] = minerInfo.getURL ();
}

//----------------------------------------------------------------//
State::State () {
}

//----------------------------------------------------------------//
//State::State ( const State* prevState ) {
//
//    if ( prevState ) {
//        *this = *prevState;
//    }
//}

//----------------------------------------------------------------//
State::~State () {
}

//================================================================//
// overrides
//================================================================//

//----------------------------------------------------------------//

} // namespace Volition