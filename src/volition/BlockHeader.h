// Copyright (c) 2017-2018 Cryptogogue, Inc. All Rights Reserved.
// http://cryptogogue.com

#ifndef VOLITION_BLOCKHEADER_H
#define VOLITION_BLOCKHEADER_H

#include <volition/common.h>

#include <volition/Accessors.h>
#include <volition/HasBlockHeaderFields.h>

namespace Volition {

class BlockHeader;
class Ledger;

//================================================================//
// BlockHeader
//================================================================//
class BlockHeader :
    public BlockHeaderFields,
    public HasBlockHeaderFields {
protected:

    //----------------------------------------------------------------//
    void                applyEntropy                        ( Ledger& ledger ) const;
    
    //----------------------------------------------------------------//
    void                            AbstractSerializable_serializeFrom      ( const AbstractSerializerFrom& serializer ) override;
    void                            AbstractSerializable_serializeTo        ( AbstractSerializerTo& serializer ) const override;
    const BlockHeaderFields&        HasBlockHeader_getFields                () const override;

public:

    SET ( time_t,               BlockDelayInSeconds,    mBlockDelay )
    SET ( const Digest&,        Charm,                  mCharm )
    SET ( const Digest&,        Digest,                 mDigest )
    SET ( string,               MinerID,                mMinerID )
    SET ( time_t,               RewriteWindow,          mRewriteWindow )

    //----------------------------------------------------------------//
                        BlockHeader                         ();
                        ~BlockHeader                        ();
    void                initialize                          ( string minerID, const Digest& visage, time_t now, const BlockHeader* prevBlock, const CryptoKeyPair& key );
};

} // namespace Volition
#endif
