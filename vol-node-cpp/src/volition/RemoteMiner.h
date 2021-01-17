// Copyright (c) 2017-2018 Cryptogogue, Inc. All Rights Reserved.
// http://cryptogogue.com

#ifndef VOLITION_REMOTEMINER_H
#define VOLITION_REMOTEMINER_H

#include <volition/common.h>
#include <volition/AbstractBlockTree.h>
#include <volition/Accessors.h>
#include <volition/CryptoKey.h>
#include <volition/Ledger.h>
#include <volition/TransactionQueue.h>
#include <volition/serialization/AbstractSerializable.h>
#include <volition/Singleton.h>
#include <volition/TransactionStatus.h>

namespace Volition {

//================================================================//
// RemoteMiner
//================================================================//
class RemoteMiner {
private:

    string                      mMinerID;

public:

    enum NetworkState {
        STATE_NEW,
        STATE_TIMEOUT,
        STATE_ONLINE,
        STATE_ERROR,
    };

    string                      mURL;
    BlockTreeTag                mTag;
    BlockTreeTag                mImproved;
    NetworkState                mNetworkState;
    string                      mMessage;

    bool                        mIsBusy;

    size_t                                              mHeight;
    bool                                                mForward;
    map < size_t, shared_ptr < const BlockHeader >>     mHeaderQueue;

    GET ( string,       MinerID,        mMinerID )
    GET ( string,       URL,            mURL )

    //----------------------------------------------------------------//
    bool            canFetchInfo            () const;
    bool            canFetchHeaders         () const;
                    RemoteMiner             ();
                    ~RemoteMiner            ();
    void            reset                   ();
    void            setError                ( string message = "" );
    void            setMinerID              ( string minerID );
    void            updateHeaders           ( AbstractBlockTree& blockTree );
};

} // namespace Volition
#endif
