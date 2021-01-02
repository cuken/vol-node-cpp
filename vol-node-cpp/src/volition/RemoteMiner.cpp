// Copyright (c) 2017-2018 Cryptogogue, Inc. All Rights Reserved.
// http://cryptogogue.com

#include <volition/Block.h>
#include <volition/Digest.h>
#include <volition/HTTPMiningMessenger.h>
#include <volition/RemoteMiner.h>
#include <volition/Transaction.h>
#include <volition/Transactions.h>
#include <volition/UnsecureRandom.h>

namespace Volition {

//================================================================//
// RemoteMiner
//================================================================//

//----------------------------------------------------------------//
RemoteMiner::RemoteMiner () :
    mNetworkState ( STATE_NEW ),
    mHeight ( 0 ),
    mForward ( true ) {
}

//----------------------------------------------------------------//
RemoteMiner::~RemoteMiner () {
}

//----------------------------------------------------------------//
void RemoteMiner::reset () {

    this->mHeight           = 0;
    this->mTag              = BlockTreeTag ();
    this->mImproved         = BlockTreeTag ();
    this->mForward          = true;
    
    this->mHeaderQueue.clear ();
}

//----------------------------------------------------------------//
void RemoteMiner::setError ( string message ) {

    this->mNetworkState     = STATE_ERROR;
    this->mMessage          = message;
    this->mHeight           = 0;
    this->mTag              = BlockTreeTag ();
    this->mImproved         = BlockTreeTag ();
    this->mForward          = true;
    
    this->mHeaderQueue.clear ();
}

//----------------------------------------------------------------//
void RemoteMiner::setMinerID ( string minerID ) {

    this->mMinerID = minerID;
    this->mTag.setName ( Format::write ( "~%s'", minerID.c_str ()));
    this->mImproved.setName ( Format::write ( "~%s'", minerID.c_str ()));
}

//----------------------------------------------------------------//
void RemoteMiner::updateHeaders ( AbstractBlockTree& blockTree ) {
    
    // process the queue
    size_t accepted = 0;
    if ( this->mHeaderQueue.size ()) {
        
        // if 'tag' gets overwritten, 'thumb' will hang on to any nodes we might need later.
//        InMemoryBlockTreeNode::ConstPtr thumb = this->mTag;
        
        // visit each header in the cache and try to apply it.
        while ( this->mHeaderQueue.size ()) {

            shared_ptr < const BlockHeader > header = this->mHeaderQueue.begin ()->second;
            
            AbstractBlockTree::CanAppend canAppend = blockTree.checkAppend ( *header );
            
            switch ( canAppend ) {
            
                case AbstractBlockTree::APPEND_OK:
                case AbstractBlockTree::ALREADY_EXISTS:
                    this->mTag = blockTree.affirmHeader ( this->mTag, header );
                    this->mHeaderQueue.erase ( this->mHeaderQueue.begin ());
                    accepted++;
                    break;
                
                case AbstractBlockTree::MISSING_PARENT:
                    if ( accepted == 0 ) {
                        // if there's anything left in the queue, back up and get an earlier batch of blocks.
                        this->mHeight = this->mHeaderQueue.begin ()->second->getHeight ();
                        this->mForward = false;
                    }
                    this->reset ();
                    return;
                
                case AbstractBlockTree::REFUSED:
                case AbstractBlockTree::TOO_SOON:
                    this->reset ();
                    break;
            }
        }
    }
    
    if ( this->mTag.hasCursor ()) {
        // nothing in the queue, so get the next batch of blocks.
        this->mHeight = this->mTag.getHeight () + 1; // this doesn't really matter.
        this->mForward = true;
    }
}

} // namespace Volition
