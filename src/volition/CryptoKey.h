// Copyright (c) 2017-2018 Cryptogogue, Inc. All Rights Reserved.
// http://cryptogogue.com

#ifndef VOLITION_CRYPTOKEY_H
#define VOLITION_CRYPTOKEY_H

#include <volition/common.h>
#include <volition/Signature.h>
#include <volition/serialization/Serialization.h>

namespace Volition {

//================================================================//
// CryptoKey
//================================================================//
class CryptoKey :
    public AbstractSerializable {
private:

    shared_ptr < Poco::Crypto::KeyPair >      mKeyPair;
    
    //----------------------------------------------------------------//
    void            AbstractSerializable_serializeFrom      ( const AbstractSerializerFrom& serializer ) override;
    void            AbstractSerializable_serializeTo        ( AbstractSerializerTo& serializer ) const override;

public:

    static const char*  DEFAULT_EC_GROUP_NAME;
    static const int    DEFAULT_EC_GROUP_NID = NID_secp256k1;

    //----------------------------------------------------------------//
                        CryptoKey               ();
                        ~CryptoKey              ();
    void                elliptic                ( int nid = DEFAULT_EC_GROUP_NID );
    void                elliptic                ( string groupName );
    static string       getGroupNameFromNID     ( int nid );
    static int          getNIDFromGroupName     ( string groupName );
    static bool         hasCurve                ( int nid );
    static bool         hasCurve                ( string groupName );
    Signature           sign                    ( const AbstractSerializable& serializable, string hashAlgorithm = Signature::DEFAULT_HASH_ALGORITHM ) const;
    bool                verify                  ( const Signature& signature, const AbstractSerializable& serializable ) const;
    
    //----------------------------------------------------------------//
    operator const bool () const {
        return this->mKeyPair ? true : false;
    }
    
    //----------------------------------------------------------------//
    operator const Poco::Crypto::ECKey* () const {
        Poco::Crypto::ECKey* ecKey = dynamic_cast < Poco::Crypto::ECKey* >( this->mKeyPair.get ());
        return ecKey;
    }
    
    //----------------------------------------------------------------//
    operator const Poco::Crypto::ECKey& () const {
        const Poco::Crypto::ECKey* ecKey = *this;
        assert ( ecKey );
        return *ecKey;
    }
};

} // namespace Volition
#endif
