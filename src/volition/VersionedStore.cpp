// Copyright (c) 2017-2018 Cryptogogue, Inc. All Rights Reserved.
// http://cryptogogue.com

#include <volition/State.h>
#include <volition/VersionedStore.h>

namespace Volition {

//================================================================//
// AbstractVersionedValue
//================================================================//

//----------------------------------------------------------------//
AbstractVersionedValue::AbstractVersionedValue () :
    mTypeID ( typeid ( void ).hash_code ()) {
}

//----------------------------------------------------------------//
AbstractVersionedValue::~AbstractVersionedValue () {
}

//----------------------------------------------------------------//
const void* AbstractVersionedValue::getRaw () const {
    return this->AbstractVersionedValue_getRaw ();
}

//----------------------------------------------------------------//
bool AbstractVersionedValue::isEmpty () const {
    return this->AbstractVersionedValue_isEmpty ();
}

//----------------------------------------------------------------//
unique_ptr < AbstractVersionedValue > AbstractVersionedValue::makeEmptyCopy () const {
    return this->AbstractVersionedValue_makeEmptyCopy ();
}

//----------------------------------------------------------------//
void AbstractVersionedValue::pop () {
    this->AbstractVersionedValue_pop ();
}

//----------------------------------------------------------------//
void AbstractVersionedValue::pushBackRaw ( const void* value ) {
    this->AbstractVersionedValue_pushBackRaw ( value );
}

//----------------------------------------------------------------//
void AbstractVersionedValue::pushFrontRaw ( const void* value ) {
    this->AbstractVersionedValue_pushFrontRaw ( value );
}

//================================================================//
// VersionedStoreEpoch
//================================================================//

//----------------------------------------------------------------//
void VersionedStoreEpoch::affirmLayer () {

    if ( this->mLayers.size () == 0 ) {
        this->pushLayer ();
    }
}

//----------------------------------------------------------------//
const AbstractVersionedValue* VersionedStoreEpoch::findVersionedValue ( string key ) const {

    // descend through epochs until we find the value. if we don't find it, return NULL.
    map < string, unique_ptr < AbstractVersionedValue >>::const_iterator valueIt = this->mMap.find ( key );
    if ( valueIt != this->mMap.cend ()) {
        return valueIt->second.get ();
    }
    return this->mParent ? this->mParent->findVersionedValue ( key ) : NULL;
}

//----------------------------------------------------------------//
void VersionedStoreEpoch::moveChildrenTo ( VersionedStoreEpoch& epoch ) {

    set < VersionedStoreEpoch* >::iterator childIt = this->mChildren.begin ();
    while ( childIt != this->mChildren.end()) {
        epoch.mChildren.insert ( *childIt );
        childIt = this->mChildren.erase ( childIt );
    }
}

//----------------------------------------------------------------//
void VersionedStoreEpoch::moveClientTo ( VersionedStore& client, VersionedStoreEpoch* epoch ) {

    assert ( client.mEpoch.get () == this );
    assert ( this->mClients.find ( &client ) != this->mClients.end ());

    // make sure we're not deleted until done
    shared_ptr < VersionedStoreEpoch > scopedSelf = this->shared_from_this ();

    this->mClients.erase ( &client );
    client.mEpoch = NULL;

    if ( epoch ) {
        epoch->mClients.insert ( &client );
        client.mEpoch = epoch->shared_from_this ();
    }
}

//----------------------------------------------------------------//
void VersionedStoreEpoch::moveClientsTo ( VersionedStoreEpoch& epoch, const VersionedStore* except ) {

    // make sure we're not deleted until done
    shared_ptr < VersionedStoreEpoch > scopedSelf = this->shared_from_this ();

    // move all the other clients to the new epoch
    set < VersionedStore* >::iterator clientIt = this->mClients.begin ();
    while ( clientIt != this->mClients.end ()) {
    
        VersionedStore* client = *clientIt;
        
        if ( client == except ) {
            // client is this, so do nothing and skip to the next client.
            ++clientIt;
        }
        else {
            // point the client at the new epoch and add it to that epoch's client set.
            client->mEpoch = epoch.shared_from_this ();
            epoch.mClients.insert ( client );
            clientIt = this->mClients.erase ( clientIt ); // remove it from the current epoch's set.
        }
    }
}

//----------------------------------------------------------------//
void VersionedStoreEpoch::moveTopLayerTo ( VersionedStoreEpoch& epoch ) {

    VersionedStoreLayer* topLayer = this->mLayers.back ().get ();
    VersionedStoreLayer* bottomLayer = epoch.mLayers [ 0 ].get ();
    
    assert ( topLayer && bottomLayer );

    set < string >::const_iterator keyIt = topLayer->mKeys.cbegin ();
    for ( ; keyIt != topLayer->mKeys.cend (); ++keyIt ) {
        const string& key = *keyIt;
        
        map < string, unique_ptr < AbstractVersionedValue >>::const_iterator valueIt = this->mMap.find ( key );
        assert ( valueIt != this->mMap.cend ());
        
        // from value
        AbstractVersionedValue* fromValue = valueIt->second.get ();
        assert ( fromValue );
        assert ( !fromValue->isEmpty ());
        
        // affirm to value
        unique_ptr < AbstractVersionedValue >& toValuePtr = epoch.mMap [ key ];
        if ( !toValuePtr ) {
            toValuePtr = fromValue->makeEmptyCopy ();
        }
        
        toValuePtr->pushFrontRaw ( fromValue->getRaw ());
        fromValue->pop ();
        
        if ( fromValue->isEmpty ()) {
            this->mMap.erase ( key );
        }
    }
    this->popLayer ();
}

//----------------------------------------------------------------//
void VersionedStoreEpoch::popLayer () {

    // if we're not popping the bottom layer, we need to pop any values it contains.
    // we'll also need to erase any values that no longer have version history.
    if ( this->mLayers.size () > 1 ) {

        const VersionedStoreLayer* layer = this->mLayers.back ().get ();
        set < string >::const_iterator keyIt = layer->mKeys.cbegin ();
        for ( ; keyIt != layer->mKeys.cend (); ++keyIt ) {
            const string& key = *keyIt;
            
            AbstractVersionedValue* value = this->mMap [ key ].get ();
            assert ( value );
            
            value->pop (); // pop the value.
            
            // if it's empty now (no more versions), remove it.
            if ( value->isEmpty ()) {
                this->mMap.erase ( key );
            }
        }
    }
    this->mLayers.pop_back ();
}

//----------------------------------------------------------------//
void VersionedStoreEpoch::pushLayer () {

    this->mLayers.push_back ( make_unique < VersionedStoreLayer >());
}

//----------------------------------------------------------------//
shared_ptr < VersionedStoreEpoch > VersionedStoreEpoch::spawnChildEpoch () {

    shared_ptr < VersionedStoreEpoch > epoch = make_shared < VersionedStoreEpoch >();
    epoch->mParent = shared_from_this ();
    this->mChildren.insert ( epoch.get ());
    return epoch;
}

//----------------------------------------------------------------//
VersionedStoreEpoch::VersionedStoreEpoch () {
}

//----------------------------------------------------------------//
VersionedStoreEpoch::~VersionedStoreEpoch () {

    if ( this->mParent ) {
        this->mParent->mChildren.erase ( this );
    }
}

//================================================================//
// VersionedStore
//================================================================//

//----------------------------------------------------------------//
void VersionedStore::clear () {

    if ( this->mEpoch ) {
        this->mEpoch->mClients.erase ( this );
        this->mEpoch = NULL;
    }
}

//----------------------------------------------------------------//
const void* VersionedStore::getRaw ( string key, size_t typeID ) const {

    assert ( this->mEpoch );
    const AbstractVersionedValue* value = this->mEpoch->findVersionedValue ( key );
    
    if ( value ) {
        assert ( value->mTypeID == typeID );
        return value->getRaw ();
    }
    return NULL;
}

//----------------------------------------------------------------//
void VersionedStore::setRaw ( string key, size_t typeID, const void* value ) {

    assert ( this->mEpoch );

    AbstractVersionedValue* versionedValue = this->mEpoch->mMap [ key ].get ();
    assert ( versionedValue );

    // if this epoch has children, then we can't add the value here. we have to spawn a new
    // epoch and move this client to it.

    if ( this->mEpoch->mChildren.size () > 0 ) {
    
        shared_ptr < VersionedStoreEpoch > epoch = this->mEpoch->spawnChildEpoch ();
        this->mEpoch->moveClientTo ( *this, epoch.get ());
        
        unique_ptr < AbstractVersionedValue > versionedValueCopy = versionedValue->makeEmptyCopy ();
        versionedValue = versionedValueCopy.get ();
        
        this->mEpoch->mMap [ key ] = move ( versionedValueCopy );
    }

    versionedValue->pushBackRaw ( value );
    
    this->mEpoch->affirmLayer ();
    this->mEpoch->mLayers.back ()->mKeys.insert ( key );
}

//----------------------------------------------------------------//
void VersionedStore::popVersion () {

    assert ( this->mEpoch );

    // if more clients, can't change this epoch - gotta make a new one
    if ( this->mEpoch->mClients.size () > 1 ) {
    
        // unlike pushVersion (), we have to make the new epoch then update all the *other*
        // clients to point to it. we also have to re-root any values held in the current
        // epoch's top layer.
    
        // make a new epoch to hold all the other client's we're leaving behind
        shared_ptr < VersionedStoreEpoch > epoch = this->mEpoch->spawnChildEpoch ();

        // and move all the other clients over
        this->mEpoch->moveClientsTo ( *epoch, this );
        
        // and now the fun begins. since we're going to remove a layer, but want the new epoch (representing the
        // original state of the store) to keep any values specified in this layer, we have to move any values
        // named in our current layer in the base layer of the new epoch.
        
        this->mEpoch->moveTopLayerTo ( *epoch );
    }
    else {
        this->mEpoch->popLayer ();
    }
    
    // if the epoch is but has a parent then we need to erase it. which should be as simple as
    // moving all its children and clients to its parent.
    if (( this->mEpoch->mLayers.size () == 0 ) && this->mEpoch->mParent ) {
        this->mEpoch->moveChildrenTo ( *this->mEpoch->mParent );
        this->mEpoch->moveClientsTo ( *this->mEpoch->mParent ); // delete the original epoch and point this->mEpoch to the parent
    }
}

//----------------------------------------------------------------//
void VersionedStore::pushVersion () {

    assert ( this->mEpoch );
    
    // if more clients, can't change this epoch - gotta make a new one
    if ( this->mEpoch->mClients.size () > 1 ) {
        
        // nice and easy - don't have to modify any other clients. just make and set the new epoch.
        shared_ptr < VersionedStoreEpoch > epoch = this->mEpoch->spawnChildEpoch ();
        this->mEpoch->moveClientTo ( *this, epoch.get ());
    }
    
    // make the new layer
    this->mEpoch->pushLayer ();
}

//----------------------------------------------------------------//
void VersionedStore::takeSnapshot ( VersionedStore& other ) {

    this->mEpoch->moveClientTo ( *this, other.mEpoch.get ());
}

//----------------------------------------------------------------//
VersionedStore::VersionedStore () {

    this->mEpoch = make_shared < VersionedStoreEpoch >();
    this->mEpoch->mClients.insert ( this );
}

//----------------------------------------------------------------//
VersionedStore::VersionedStore ( VersionedStore& other ) {

    this->takeSnapshot ( other );
}

//----------------------------------------------------------------//
VersionedStore::VersionedStore ( const VersionedStore& other ) {
    assert ( false );
}

//----------------------------------------------------------------//
VersionedStore::~VersionedStore () {

    this->mEpoch->moveClientTo ( *this, NULL );
}

} // namespace Volition