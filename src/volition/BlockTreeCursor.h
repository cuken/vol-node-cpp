// Copyright (c) 2017-2018 Cryptogogue, Inc. All Rights Reserved.
// http://cryptogogue.com

#ifndef VOLITION_BLOCKTREECURSOR_H
#define VOLITION_BLOCKTREECURSOR_H

#include <volition/common.h>
#include <volition/Accessors.h>
#include <volition/Block.h>
#include <volition/BlockTreeEnums.h>

namespace Volition {

class AbstractBlockTree;

//================================================================//
// BlockTreeCursor
//================================================================//
class BlockTreeCursor :
    public HasBlockHeaderFields {
protected:
    
    friend class AbstractBlockTree;
    friend class BlockTreeTag;

    const AbstractBlockTree*            mTree;

    shared_ptr < const BlockHeader >    mHeader;
    
    kBlockTreeEntryStatus               mStatus;
    kBlockTreeEntryMeta                 mMeta;
    bool                                mHasBlock;

    //----------------------------------------------------------------//
    void                                log                         ( string& str, string prefix = "" ) const;
    void                                logBranchRecurse            ( string& str, size_t maxDepth ) const;

    //----------------------------------------------------------------//
    const BlockHeaderFields&            HasBlockHeader_getFields    () const override;

public:

    GET ( const AbstractBlockTree*,     Tree,       mTree )

    //----------------------------------------------------------------//
                                        BlockTreeCursor             ();
                                        ~BlockTreeCursor            ();
    bool                                checkStatus                 ( kBlockTreeEntryStatus status ) const;
    bool                                checkTree                   ( const AbstractBlockTree* tree ) const;
    static int                          compare                     ( const BlockTreeCursor& node0, const BlockTreeCursor& node1 );
    bool                                equals                      ( const BlockTreeCursor& rhs ) const;
    static BlockTreeCursor              findRoot                    ( const BlockTreeCursor& node0, const BlockTreeCursor& node1 );
    shared_ptr < const Block >          getBlock                    () const;
    string                              getHash                     () const;
    const BlockHeader&                  getHeader                   () const;
    BlockTreeCursor                     getParent                   () const;
    kBlockTreeEntryStatus               getStatus                   () const;
    bool                                hasBlock                    () const;
    bool                                hasHeader                   () const;
    bool                                hasParent                   () const;
    bool                                isAncestorOf                ( BlockTreeCursor tail ) const;
    bool                                isComplete                  () const;
    bool                                isInvalid                   () const;
    bool                                isMissing                   () const;
    bool                                isMissingOrInvalid          () const;
    bool                                isNew                       () const;
    BlockTreeCursor                     trim                        ( kBlockTreeEntryStatus status ) const;
    BlockTreeCursor                     trimInvalid                 () const;
    BlockTreeCursor                     trimMissing                 () const;
    BlockTreeCursor                     trimMissingOrInvalid        () const;
    string                              write                       () const;
    string                              writeBranch                 ( size_t maxDepth = 3 ) const;
};

} // namespace Volition
#endif
