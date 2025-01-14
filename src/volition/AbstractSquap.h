// Copyright (c) 2017-2018 Cryptogogue, Inc. All Rights Reserved.
// http://cryptogogue.com

#ifndef VOLITION_ABSTRACTSQUAP_H
#define VOLITION_ABSTRACTSQUAP_H

#include <volition/common.h>
#include <volition/Asset.h>
#include <volition/AssetFieldValue.h>
#include <volition/FNV1a.h>
#include <volition/serialization/Serialization.h>

namespace Volition {

// Schema QUAlifier oPerator

//================================================================//
// SquapEvaluationContext
//================================================================//
class SquapEvaluationContext {
private:

    typedef SerializableMap < string, AssetID::Index > AssetParams;
    typedef SerializableMap < string, AssetFieldValue > ConstParams;

    const Asset*                                        mAsset;
    AssetFieldValue                                     mValue;
    const map < string, shared_ptr < const Asset >>     mAssetParams;
    const map < string, AssetFieldValue >               mValueParams;

public:

    //----------------------------------------------------------------//
    const Asset* getAsset () const {
        if ( this->mAsset ) {
            return this->mAsset;
        }
        return NULL;
    };

    //----------------------------------------------------------------//
    const Asset* getAsset ( string paramName ) const {
    
        if ( paramName.size () == 0 ) {
            return this->getAsset ();
        }
        
        map < string, shared_ptr < const Asset >>::const_iterator paramIt = this->mAssetParams.find ( paramName );
        if ( paramIt != this->mAssetParams.cend ()) {
            return paramIt->second.get ();
        }
        return NULL;
    };

    //----------------------------------------------------------------//
    AssetFieldValue getValue () const {

        return this->mValue;
    };

    //----------------------------------------------------------------//
    AssetFieldValue getValue ( string paramName ) const {
    
        if ( paramName.size () == 0 ) {
            return this->getValue ();
        }
        
        map < string, AssetFieldValue >::const_iterator paramIt = this->mValueParams.find ( paramName );
        if ( paramIt != this->mValueParams.cend ()) {
            return paramIt->second;
        }
        return AssetFieldValue ();
    };

    //----------------------------------------------------------------//
    SquapEvaluationContext ( const Asset& asset ) :
        mAsset ( &asset ) {
    }
    
    //----------------------------------------------------------------//
    SquapEvaluationContext ( const AssetFieldValue& value ) :
        mValue ( value ) {
    }
    
    //----------------------------------------------------------------//
    SquapEvaluationContext ( const map < string, shared_ptr < const Asset >>& assetParams, const map < string, AssetFieldValue >& valueParams ) :
        mAsset ( NULL ),
        mAssetParams ( assetParams ),
        mValueParams ( valueParams ) {
    }
};

//================================================================//
// AbstractSquap
//================================================================//
class AbstractSquap :
     public virtual AbstractSerializable {
public:

    enum OpCode : u64 {
        ADD                 = FNV1a::const_hash_64 ( "ADD" ),
        AND                 = FNV1a::const_hash_64 ( "AND" ),
        CONST               = FNV1a::const_hash_64 ( "CONST" ),
        EQUAL               = FNV1a::const_hash_64 ( "EQUAL" ),
        DIV                 = FNV1a::const_hash_64 ( "DIV" ),
        FUNC                = FNV1a::const_hash_64 ( "FUNC" ),
        GREATER             = FNV1a::const_hash_64 ( "GREATER" ),
        GREATER_OR_EQUAL    = FNV1a::const_hash_64 ( "GREATER_OR_EQUAL" ),
        INDEX               = FNV1a::const_hash_64 ( "INDEX" ),
        LESS                = FNV1a::const_hash_64 ( "LESS" ),
        LESS_OR_EQUAL       = FNV1a::const_hash_64 ( "LESS_OR_EQUAL" ),
        MOD                 = FNV1a::const_hash_64 ( "MOD" ),
        MUL                 = FNV1a::const_hash_64 ( "MUL" ),
        NOT                 = FNV1a::const_hash_64 ( "NOT" ),
        NOT_EQUAL           = FNV1a::const_hash_64 ( "NOT_EQUAL" ),
        OR                  = FNV1a::const_hash_64 ( "OR" ),
        SUB                 = FNV1a::const_hash_64 ( "SUB" ),
        XOR                 = FNV1a::const_hash_64 ( "XOR" ),
    };

    OpCode      mOpCode;
    
    //----------------------------------------------------------------//
    virtual AssetFieldValue     AbstractSquap_evaluate      ( const SquapEvaluationContext& context ) const = 0;
    
    //----------------------------------------------------------------//
    AssetFieldValue evaluate ( const SquapEvaluationContext& context ) const {
        return this->AbstractSquap_evaluate ( context );
    }

    //----------------------------------------------------------------//
    static string getOpCodeName ( OpCode opCode ) {
    
        switch ( opCode ) {
            case ADD:                   return "ADD";
            case AND:                   return "AND";
            case CONST:                 return "CONST";
            case EQUAL:                 return "EQUAL";
            case DIV:                   return "DIV";
            case FUNC:                  return "FUNC";
            case GREATER:               return "GREATER";
            case GREATER_OR_EQUAL:      return "GREATER_OR_EQUAL";
            case INDEX:                 return "INDEX";
            case LESS:                  return "LESS";
            case LESS_OR_EQUAL:         return "LESS_OR_EQUAL";
            case MOD:                   return "MOD";
            case MUL:                   return "MUL";
            case NOT:                   return "NOT";
            case NOT_EQUAL:             return "NOT_EQUAL";
            case OR:                    return "OR";
            case SUB:                   return "SUB";
            case XOR:                   return "XOR";
        }
        return "";
    }

    //----------------------------------------------------------------//
    void setOpCode ( OpCode opCode ) {
        this->mOpCode = opCode;
    }
    
    //----------------------------------------------------------------//
    void AbstractSerializable_serializeTo ( AbstractSerializerTo& serializer ) const override {
     
        serializer.serialize ( "op",    AbstractSquap::getOpCodeName ( this->mOpCode ));
    }
};

} // namespace Volition
#endif
