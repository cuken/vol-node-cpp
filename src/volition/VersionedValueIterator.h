// Copyright (c) 2017-2018 Cryptogogue, Inc. All Rights Reserved.
// http://cryptogogue.com

#ifndef VOLITION_VERSIONEDVALUEITERATOR_H
#define VOLITION_VERSIONEDVALUEITERATOR_H

#include <volition/common.h>
#include <volition/VersionedStore.h>

namespace Volition {

//================================================================//
// VersionedValueIterator
//================================================================//
template < typename TYPE >
class VersionedValueIterator :
    public VersionedStore {
protected:

    friend class VersionedStore;

    typedef typename map < size_t, TYPE >::const_iterator ValueIterator;

    enum {
        VALID,
        
        // these are only set *after* a call to prev() or next().
        // they are not meant to be exposed or for general use.
        NO_PREV,
        NO_NEXT,
    };

    VersionedStore      mAnchor;
    string              mKey;
    ValueIterator       mIterator;
    int                 mState;
    
    size_t              mFirstVersion;
    size_t              mLastVersion;

    //----------------------------------------------------------------//
    const ValueStack < TYPE >* getValueStack () {
        
        return this->getValueStack ( this->mEpoch );
    }

    //----------------------------------------------------------------//
    const ValueStack < TYPE >* getValueStack ( shared_ptr < VersionedStoreEpoch > epoch ) {
    
        if ( epoch ) {
            const AbstractValueStack* abstractValueStack = epoch->findValueStack ( this->mKey );
            if ( abstractValueStack ) {
                return dynamic_cast < const ValueStack < TYPE >* >( abstractValueStack );
            }
        }
        return NULL;
    }

    //----------------------------------------------------------------//
    void seekNext ( shared_ptr < VersionedStoreEpoch > prevEpoch ) {
        
        shared_ptr < VersionedStoreEpoch > epoch = this->mAnchor.mEpoch;
        size_t top = this->mAnchor.mVersion + 1;
        
        shared_ptr < VersionedStoreEpoch > bestEpoch;
        const ValueStack < TYPE >* bestValueStack = NULL;
        size_t bestTop = top;
        
        for ( ; epoch != prevEpoch; epoch = epoch->mParent ) {
            const ValueStack < TYPE >* valueStack = this->getValueStack ( epoch );
            if ( valueStack && valueStack->size ()) {
                bestEpoch = epoch;
                bestValueStack = valueStack;
                bestTop = top;
            }
            top = epoch->mBaseVersion;
        }
        
        if ( bestValueStack ) {
            this->setExtents ( *bestValueStack, bestTop - 1 );
            this->mIterator = bestValueStack->mValuesByVersion.begin ();
            this->setEpoch ( bestEpoch, this->mFirstVersion );
            this->mState = VALID;
        }
        else {
            this->mState = NO_NEXT;
        }
    }

    //----------------------------------------------------------------//
    void seekPrev ( shared_ptr < VersionedStoreEpoch > epoch, size_t top ) {
        
        for ( ; epoch; epoch = epoch->mParent ) {
        
            const ValueStack < TYPE >* valueStack = this->getValueStack ( epoch );
            
            if ( valueStack && valueStack->size ()) {
                this->setExtents ( *valueStack, top - 1 );
                this->mIterator = valueStack->mValuesByVersion.find ( this->mLastVersion );
                this->setEpoch ( epoch, this->mLastVersion );
                this->mState = VALID;
                return;
            }
            top = epoch->mBaseVersion;
        }
        this->mState = NO_PREV;
    }

    //----------------------------------------------------------------//
    void setExtents ( const ValueStack < TYPE >& valueStack, size_t top ) {
    
        assert ( valueStack.mValuesByVersion.size ());
        this->mFirstVersion     = valueStack.mValuesByVersion.begin ()->first;
        
        typename map < size_t, TYPE >::const_iterator valueIt = valueStack.mValuesByVersion.lower_bound ( top );
        
        if ( valueIt == valueStack.mValuesByVersion.cend ()) {
            this->mLastVersion = valueStack.mValuesByVersion.rbegin ()->first;
        }
        else {
            if ( valueIt->first > top ) {
                valueIt--;
            }
            this->mLastVersion = valueIt->first;
        }
    }

public:

    //----------------------------------------------------------------//
    operator bool () const {
        return this->isValid ();
    }

    //----------------------------------------------------------------//
    const TYPE& operator * () const {
        return this->value ();
    }

    //----------------------------------------------------------------//
    bool isValid () const {
        return ( this->mState == VALID );
    }
    
    //----------------------------------------------------------------//
    bool next () {
        
        if ( !this->mEpoch ) return false;
        
        if ( this->mState == NO_PREV ) {
            this->mState = VALID;
        }
        else if ( this->mState != NO_NEXT ) {
            
             if ( this->mIterator->first < this->mLastVersion ) {
                ++this->mIterator;
                this->mVersion = this->mIterator->first;
            }
            else {
                assert ( this->mIterator->first == this->mLastVersion );
                this->seekNext ( this->mEpoch );
            }
        }
        return ( this->mState != NO_NEXT );
    }
    
    //----------------------------------------------------------------//
    bool prev () {

        if ( !this->mEpoch ) return false;

        if ( this->mState == NO_NEXT ) {
            this->mState = VALID;
        }
        else if ( this->mState != NO_PREV ) {
                
            if ( this->mIterator->first > this->mFirstVersion ) {
                --this->mIterator;
                this->mVersion = this->mIterator->first;
            }
            else {
                assert ( this->mIterator->first == this->mFirstVersion );
                this->seekPrev ( this->mEpoch->mParent, this->mVersion );
            }
        }
        return ( this->mState != NO_PREV );
    }

    //----------------------------------------------------------------//
    const TYPE& value () const {
        assert ( this->isValid ());
        return this->mIterator->second;
    }
    
    //----------------------------------------------------------------//
    VersionedValueIterator ( VersionedStore& versionedStore, string key ) :
        mAnchor ( versionedStore ),
        mKey ( key ) {
        
        if ( this->mAnchor.mEpoch ) {
            this->seekPrev ( this->mAnchor.mEpoch, this->mAnchor.mVersion + 1 );
        }
    }
};

} // namespace Volition
#endif
