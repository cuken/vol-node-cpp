// Copyright (c) 2017-2018 Cryptogogue, Inc. All Rights Reserved.
// http://cryptogogue.com

#include <volition/Block.h>
#include <volition/Chain.h>
#include <volition/ChainMetadata.h>
#include <volition/Format.h>

namespace Volition {

//================================================================//
// Chain
//================================================================//

//----------------------------------------------------------------//
bool Chain::canPush ( string minerID, bool force ) const {

    return ( force || ( !this->isInCycle ( this->getTopCycle (), minerID )));
}

//----------------------------------------------------------------//
Chain::Chain () {
}

//----------------------------------------------------------------//
Chain::~Chain () {
}

//----------------------------------------------------------------//
size_t Chain::countBlocks () const {

    return this->getVersion ();
}

//----------------------------------------------------------------//
size_t Chain::countBlocks ( size_t cycleIdx ) const {

    VersionedValueIterator < string > cycleIt ( *this, CYCLE_KEY );
    size_t top = this->getVersion ();

    for ( ; cycleIt; cycleIt.prev ()) {
        shared_ptr < Cycle > cycle = Ledger::getJSONSerializableObject < Cycle >( cycleIt, CYCLE_KEY );
        assert ( cycle );
        if ( cycle->mCycleID == cycleIdx ) {
            return top - cycle->mBase;
        }
        top = cycle->mBase;
    }
    return top - 0;
}

//----------------------------------------------------------------//
size_t Chain::countCycles () const {

    if ( this->hasValue ( CYCLE_KEY )) {
        const Cycle& cycle = this->getTopCycle ();
        return cycle.mCycleID + 1;
    }
    return 0;
}

//----------------------------------------------------------------//
ChainPlacement Chain::findNextCycle ( ChainMetadata& metaData, string minerID ) const {

    if ( !this->hasValue ( CYCLE_KEY )) {
        return ChainPlacement ( Cycle (), true );
    }

    // first, seek back to find the earliest cycle we could change.
    // we're only allowed change if next cycle ratio is below the threshold.
    // important to measure the threshold *after* the proposed change.
    size_t nCycles = this->countCycles ();

    // if only one cycle, that is the genesis cycle. skip it and add a new cycle.
    if ( nCycles > 1 ) {

        // the 'best' cycle is the earliest cycle in the chain that can be edited and will be 'improved.'
        // 'improved' means the miner will be added to the pool and/or chain.
        bool foundCycle = false;
        Cycle bestCycle; // default is to start a new cycle.

        // start at the last cycle and count backward until we find a the best cycle.
        // the most recent cycle can always be edited, though may not be 'improved.'
        VersionedValueIterator < string > cycleIt ( *this, CYCLE_KEY );
        
        for ( ; cycleIt; cycleIt.prev ()) {
        
            shared_ptr < Cycle > cycle = Ledger::getJSONSerializableObject < Cycle >( cycleIt, CYCLE_KEY );
            assert ( cycle );
        
            if ( metaData.canEdit ( cycle->mCycleID, minerID )) {
            
                if ( this->willImprove ( metaData, *cycle, minerID )) {
                    bestCycle = *cycle;
                    foundCycle = true;
                }
            }
            else {
                break;
            }
        }
        
        if ( foundCycle ) {
            return ChainPlacement ( bestCycle, false );
        }
    }
    return ChainPlacement ( this->getTopCycle (), true );
}

//----------------------------------------------------------------//
Cycle Chain::getTopCycle () const {

    shared_ptr < Cycle > cycle = this->getJSONSerializableObject < Cycle >( CYCLE_KEY );
    assert ( cycle );
    return *cycle;
}

//----------------------------------------------------------------//
bool Chain::isInCycle ( const Cycle& cycle, string minerID ) const {

    VersionedStoreIterator chainIt ( *this, cycle.mBase );
    // TODO: rewrite this loop
    for ( ; chainIt && ( Ledger::getJSONSerializableObject < Cycle >( chainIt, CYCLE_KEY )->mCycleID == cycle.mCycleID ); chainIt.next ()) {
        shared_ptr < Block > block = Ledger::getJSONSerializableObject < Block >( chainIt, BLOCK_KEY );
        assert ( block );
        if ( block->mMinerID == minerID ) return true;
    }
    return false;
}

//----------------------------------------------------------------//
void Chain::newCycle () {

    Cycle cycle;
    cycle.mBase = this->getVersion ();
    if ( cycle.mBase > 0 ) {
        shared_ptr < Cycle > prevCycle = this->getJSONSerializableObject < Cycle >( CYCLE_KEY );
        assert ( prevCycle );
        cycle.mCycleID = prevCycle->mCycleID + 1;
    }
    this->setJSONSerializableObject < Cycle >( CYCLE_KEY, cycle );
}

//----------------------------------------------------------------//
void Chain::prepareForPush ( ChainMetadata& metaData, const ChainPlacement& placement, Block& block ) {

    if ( placement.mNewCycle ) {
        this->newCycle ();
    }
    else {
        const Cycle& cycle = placement.mCycle;
        
        size_t top = this->getVersion ();
        size_t truncate = cycle.mBase;
        size_t score = block.getScore ();
        VersionedStoreIterator chainIt ( *this, truncate );
        for ( ; chainIt && ( !chainIt.isCurrent ()) && ( truncate < top ); chainIt.next (), ++truncate ) {
            // TODO: rewrite this
            shared_ptr < Block > block = Ledger::getJSONSerializableObject < Block >( chainIt, BLOCK_KEY );
            assert ( block );
            
            if (( block->mCycleID > cycle.mCycleID ) || ( block->getScore () > score )) break;
        }
        this->revert ( truncate );
        this->clearVersion ();
        
        metaData.mCycleMetadata.resize ( cycle.mCycleID + 1 );
    }
    
    size_t version = this->getVersion ();
    if ( version > 0 ) {
        VersionedStoreIterator prevVersion ( *this, version - 1 );
        shared_ptr < Block > prevBlock = Ledger::getJSONSerializableObject < Block >( prevVersion, BLOCK_KEY );
        assert ( prevBlock );
        block.setPreviousBlock ( *prevBlock );
    }
}

//----------------------------------------------------------------//
string Chain::print ( const char* pre, const char* post ) const {
    
    return this->print ( NULL, pre, post );
}

//----------------------------------------------------------------//
string Chain::print ( const ChainMetadata& metaData, const char* pre, const char* post ) const {

    return this->print ( &metaData, pre, post );
}

//----------------------------------------------------------------//
string Chain::print ( const ChainMetadata* metaData, const char* pre, const char* post ) const {

    string str;

    if ( pre ) {
        Format::write ( str, "%s", pre );
    }

    size_t cycleCount = 0;
    size_t blockCount = 0;
    
    VersionedStoreIterator chainIt ( *this, 0 );
    
    shared_ptr < Cycle > prevCycle = Ledger::getJSONSerializableObject < Cycle >( chainIt, CYCLE_KEY );
    assert ( prevCycle );
    
    for ( ; chainIt && ( !chainIt.isCurrent ()); chainIt.next ()) {
        shared_ptr < Cycle > cycle = Ledger::getJSONSerializableObject < Cycle >( chainIt, CYCLE_KEY );
        assert ( cycle );
        
        if (( cycleCount == 0 ) || ( cycle->mCycleID != prevCycle->mCycleID )) {
        
            if ( cycleCount > 0 ) {
                if ( cycleCount > 1 ) {
                    Format::write ( str, " (%d)]", ( int )( metaData ? metaData->countParticipants ( cycleCount - 1 ) : blockCount ));
                }
                else {
                    Format::write ( str, "]" );
                }
            }
            Format::write ( str, "[" );
            cycleCount++;
            blockCount = 0;
        }
        
        shared_ptr < Block > block = Ledger::getJSONSerializableObject < Block >( chainIt, BLOCK_KEY );
        assert ( block );
    
        if ( blockCount > 0 ) {
            Format::write ( str, "," );
        }
        
        Format::write ( str, "%s", block->mHeight == 0  ? "." : block->getMinerID ().c_str ());
        blockCount++;
        *prevCycle = *cycle;
    }
    
    if ( cycleCount > 1 ) {
        Format::write ( str, " (%d)]", ( int )( metaData ? metaData->countParticipants ( cycleCount - 1 ) : blockCount ));
    }
    else {
        Format::write ( str, "]" );
    }

    if ( post ) {
        Format::write ( str, "%s", post );
    }
    
    return str;
}

//----------------------------------------------------------------//
bool Chain::pushBlock ( const Block& block ) {

    Ledger ledger ( *this );
    
    if ( !ledger.hasValue ( CYCLE_KEY )) {
        assert ( ledger.getVersion () == 0 );
        ledger.setJSONSerializableObject < Cycle >( CYCLE_KEY, Cycle ());
    }

    shared_ptr < Cycle > baseCycle = ledger.getJSONSerializableObject < Cycle >( CYCLE_KEY );
    assert ( baseCycle );
    size_t baseCycleID = baseCycle->mCycleID;
    
    if ( block.mCycleID != baseCycleID ) {
        assert ( block.mCycleID == ( baseCycleID + 1 ));
        ledger.setJSONSerializableObject < Cycle >( CYCLE_KEY, Cycle ( block.mCycleID, ledger.getVersion ()));
    }
    
    bool result = block.apply ( ledger );

    if ( result ) {
        ledger.pushVersion ();
        this->takeSnapshot ( ledger );
    }
    return result;
}

//----------------------------------------------------------------//
void Chain::reset () {

    this->Ledger::reset ();
}

//----------------------------------------------------------------//
size_t Chain::size () const {

    return this->VersionedStore::getVersion ();
}

//----------------------------------------------------------------//
Chain::UpdateResult Chain::update ( ChainMetadata& metaData, const Block& block ) {

    // rewind to block height.
    VersionedStoreIterator chainIt ( *this, block.mHeight );

    shared_ptr < Block > original = Ledger::getJSONSerializableObject < Block >( chainIt, BLOCK_KEY );
    assert ( original );

    // if the block matches... do nothing. next block.
    if ( block == *original ) return UpdateResult::UPDATE_EQUALS;
    
    // check to see if the previous block matches
    if ( block.mPrevDigest == original->mPrevDigest ) {
        
        // it's preceded by a match (hashes match), so let's see if it's a better block.
        int compare = Block::compare ( *original, block );
        assert ( compare != 0 );
        
        if (( compare < 0 ) || ( !metaData.canEdit ( original->mCycleID, block.mMinerID ))) {
        
            // rejected or can't edit, so try again
            return UpdateResult::UPDATE_RETRY;
        }
        
        // new block is an improvement *and* we can edit. so, truncate the chain and add the new block.
        
        // roll back to the point of divergence
        Chain ledger;
        ledger.takeSnapshot ( *this );
        ledger.revert ( block.mHeight );
        ledger.clearVersion ();
        
        bool result = ledger.pushBlock ( block );
        if ( !result ) return UpdateResult::UPDATE_RETRY; // block was rejected

        // truncate the chain and grabs the revised ledger.
        this->takeSnapshot ( ledger );
        
        // truncate the metadata. we're keeping base cycle so we get the full
        // set of all active contributors known to the chain prior to truncation.
        metaData.mCycleMetadata.resize ( block.mCycleID + 1 );

        // affirm the block as a participant in the current cycle.
        metaData.affirmParticipant ( block.mCycleID, block.mMinerID );
    }

    // previous blocks don't match. need to rewind.
    return UpdateResult::UPDATE_REWIND;
}

//----------------------------------------------------------------//
void Chain::update ( ChainMetadata& metaData, const Chain& other ) {

    if ( other.getVersion () == 0 ) return;
    
    VersionedStoreIterator chainIt0 ( *this, 0 );
    VersionedStoreIterator chainIt1 ( other, 0 );
    
    bool diverged = false;
    size_t diverge = 0;
    bool overwrite = false;
    
    for ( ; chainIt1 && ( !chainIt1.isCurrent ()); chainIt0.next (), chainIt1.next ()) {
        
        size_t height = chainIt1.getVersion ();
        
        if ( chainIt0 && ( !chainIt0.isCurrent ())) {
            
            shared_ptr < Block > block0 = Ledger::getJSONSerializableObject < Block >( chainIt0, BLOCK_KEY );
            shared_ptr < Block > block1 = Ledger::getJSONSerializableObject < Block >( chainIt1, BLOCK_KEY );
        
            assert ( block0 );
            assert ( block1 );
        
            if ( diverged == false ) {
                diverge = height;
                diverged = ( block0 != block1 );
            }
            
            int compare = Block::compare ( *block0, *block1 );
            
            if ( compare != 0 ) {
                overwrite = (( compare > 0 ) && ( metaData.canEdit ( block0->mCycleID, block1->mMinerID )));
                break;
            }
        }
        else {
            diverge = height;
            overwrite = true;
            break;
        }
    }
    
    if ( overwrite ) {
        
        // roll back to the point of divergence
        Chain ledger;
        ledger.takeSnapshot ( *this );
        ledger.revert ( diverge );
        ledger.clearVersion ();
        
        // apply all the other chain's blocks
        VersionedStoreIterator fromIt ( other, diverge );
        for ( ; fromIt && ( !fromIt.isCurrent ()); fromIt.next ()) {
            
            shared_ptr < Block > block = Ledger::getJSONSerializableObject < Block >( fromIt, BLOCK_KEY );
            assert ( block );
            
            bool result = ledger.pushBlock ( *block );
            if ( !result ) return;
        }
        
        // if we're here, then all the blocks were valid. go ahead and truncate
        // the chain and rebuild the metadata.
        
        // this truncates the chain and grabs the revised ledger.
        this->takeSnapshot ( ledger );
        
        // rewind back to diverge
        VersionedStoreIterator chainIt ( *this, diverge );
        
        // save the base cycle ID for later
        shared_ptr < Cycle > baseCycle = Ledger::getJSONSerializableObject < Cycle >( chainIt, CYCLE_KEY );
        size_t baseCycleID = baseCycle ? baseCycle->mCycleID : 0;
        
        // truncate the metadata. we're keeping base cycle so we get the full
        // set of all active contributors known to the chain prior to truncation.
        metaData.mCycleMetadata.resize ( baseCycleID + 1 );
        
        for ( ; chainIt && ( !chainIt.isCurrent ()); chainIt.next ()) {
        
            shared_ptr < Block > block = Ledger::getJSONSerializableObject < Block >( chainIt, BLOCK_KEY );
            assert ( block );
            metaData.affirmParticipant ( block->mCycleID, block->mMinerID );
        }
    }
}

//----------------------------------------------------------------//
bool Chain::willImprove ( ChainMetadata& metaData, const Cycle& cycle, string minerID ) const {

    // cycle will improve if miner is missing from either the participant set or the chain itself
    return (( cycle.mCycleID > 0 ) && (( metaData.isParticipant ( cycle.mCycleID, minerID ) && this->isInCycle ( cycle, minerID )) == false ));
}

//================================================================//
// overrides
//================================================================//

//----------------------------------------------------------------//
void Chain::AbstractSerializable_serializeFrom ( const AbstractSerializerFrom& serializer ) {

    this->reset ();
    
    SerializableVector < Block > blocks;
    serializer.serialize ( "blocks", blocks );

    size_t size = blocks.size ();
    for ( size_t i = 0; i < size; ++i ) {
    
        Block block = blocks [ i ];
        this->pushBlock ( block );
    }
}

//----------------------------------------------------------------//
void Chain::AbstractSerializable_serializeTo ( AbstractSerializerTo& serializer ) const {

    SerializableVector < Block > blocks;
    
    size_t top = this->getVersion ();
    VersionedStoreIterator chainIt ( *this, 0 );
    for ( ; chainIt && ( chainIt.getVersion () < top ); chainIt.next ()) {
        shared_ptr < Block > block = Ledger::getJSONSerializableObject < Block >( chainIt, BLOCK_KEY );
        assert ( block );
        blocks.push_back ( *block );
    }
    serializer.serialize ( "blocks", blocks );
}

} // namespace Volition
