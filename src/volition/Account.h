// Copyright (c) 2017-2018 Cryptogogue, Inc. All Rights Reserved.
// http://cryptogogue.com

#ifndef VOLITION_ACCOUNT_H
#define VOLITION_ACCOUNT_H

#include <volition/common.h>
#include <volition/AccountID.h>
#include <volition/KeyAndPolicy.h>
#include <volition/serialization/Serialization.h>

namespace Volition {

//================================================================//
// Account
//================================================================//
class Account :
    public AbstractSerializable {
public:
    
    u64                                 mBalance;
    Policy                              mPolicy;
    SerializableSharedPtr < Policy >    mBequest;

    SerializableMap < string, KeyAndPolicy > mKeys;

    //----------------------------------------------------------------//
    operator const Policy& () const {
        return this->mPolicy;
    }

    //----------------------------------------------------------------//
    void AbstractSerializable_serializeFrom ( const AbstractSerializerFrom& serializer ) override {
    
        serializer.serialize ( "balance",           this->mBalance );
        serializer.serialize ( "keys",              this->mKeys );
        serializer.serialize ( "policy",            this->mPolicy );
        serializer.serialize ( "bequest",           this->mBequest );
    }
    
    //----------------------------------------------------------------//
    void AbstractSerializable_serializeTo ( AbstractSerializerTo& serializer ) const override {
    
        serializer.serialize ( "balance",           this->mBalance );
        serializer.serialize ( "keys",              this->mKeys );
        serializer.serialize ( "policy",            this->mPolicy );
        serializer.serialize ( "bequest",           this->mBequest );
    }

    //----------------------------------------------------------------//
    Account () :
        mBalance ( 0 ) {
    }

    //----------------------------------------------------------------//
    u64 getBalance () const {
        return this->mBalance;
    }

    //----------------------------------------------------------------//
    const Policy* getBequest () const {
        return this->mBequest.get ();
    }

    //----------------------------------------------------------------//
    KeyAndPolicy getKeyAndPolicy ( string keyName ) const {
        
        map < string, KeyAndPolicy >::const_iterator keyAndPolicyIt = this->mKeys.find ( keyName );
        if ( keyAndPolicyIt != this->mKeys.cend ()) {
            return keyAndPolicyIt->second;
        }
        return KeyAndPolicy ();
    }

    //----------------------------------------------------------------//
    void getKeys ( map < string, KeyAndPolicy >& keys ) const {
    
        map < string, KeyAndPolicy >::const_iterator keyIt = this->mKeys.cbegin ();
        for ( ; keyIt != this->mKeys.end (); ++keyIt ) {
            keys [ keyIt->first ] = keyIt->second;
        }
    }
    
    //----------------------------------------------------------------//
    const Policy& getPolicy () {
        return this->mPolicy;
    }
};

} // namespace Volition
#endif
