// Copyright (c) 2017-2018 Cryptogogue, Inc. All Rights Reserved.
// http://cryptogogue.com

#ifndef VOLITION_ACCOUNTODBM_H
#define VOLITION_ACCOUNTODBM_H

#include <volition/common.h>
#include <volition/Account.h>
#include <volition/Asset.h>
#include <volition/Ledger.h>
#include <volition/LedgerFieldODBM.h>
#include <volition/LedgerKey.h>
#include <volition/LedgerObjectFieldODBM.h>
#include <volition/Policy.h>
#include <volition/Schema.h>

namespace Volition {

//================================================================//
// AccountODBM
//================================================================//
class AccountODBM {
private:

    //----------------------------------------------------------------//
    static LedgerKey keyFor_assetCount ( Account::Index index ) {
        return LedgerKey ([ = ]() { return Format::write ( "account.%d.assetCount", index ); });
    }

    //----------------------------------------------------------------//
    static LedgerKey keyFor_body ( Account::Index index ) {
        return LedgerKey ([ = ]() { return Format::write ( "account.%d", index ); });
    }

    //----------------------------------------------------------------//
    static LedgerKey keyFor_inventoryField ( Account::Index index, size_t position ) {
        return Format::write ( "account.%d.assets.%d", index, position );
    }

    //----------------------------------------------------------------//
    static LedgerKey keyFor_inventoryLogEntry ( Account::Index index, u64 inventoryNonce ) {
        return Format::write ( "account.%d.inventoryLog.%d", index, inventoryNonce );
    }

    //----------------------------------------------------------------//
    static LedgerKey keyFor_inventoryNonce ( Account::Index index ) {
        return LedgerKey ([ = ]() { return Format::write ( "account.%d.inventoryNonce", index ); });
    }

    //----------------------------------------------------------------//
    static LedgerKey keyFor_minerInfo ( Account::Index index ) {
        return LedgerKey ([ = ]() { return Format::write ( "account.%d.miner", index ); });
    }

    //----------------------------------------------------------------//
    static LedgerKey keyFor_name ( Account::Index index ) {
        return LedgerKey ([ = ]() { return Format::write ( "account.%d.name", index ); });
    }
    
    //----------------------------------------------------------------//
    static LedgerKey keyFor_transactionLookup ( Account::Index index, string uuid ) {
        return Format::write ( "account.%d.transactionLookup.%s", index, uuid.c_str ());
    }

    //----------------------------------------------------------------//
    static LedgerKey keyFor_transactionNonce ( Account::Index index ) {
        return LedgerKey ([ = ]() { return Format::write ( "account.%d.transactionNonce", index ); });
    }

public:

    ConstOpt < Ledger >     mLedger;
    Account::Index          mIndex;

    LedgerFieldODBM < u64 >                 mAssetCount;
    LedgerFieldODBM < u64 >                 mInventoryNonce;
    LedgerFieldODBM < u64 >                 mTransactionNonce;
    LedgerFieldODBM < string >              mName;

    LedgerObjectFieldODBM < Account >       mBody;
    LedgerObjectFieldODBM < MinerInfo >     mMinerInfo;

    //----------------------------------------------------------------//
    operator bool () {
        return this->mName.exists ();
    }

    //----------------------------------------------------------------//
    AccountODBM ( ConstOpt < Ledger > ledger, Account::Index index ) :
        mLedger ( ledger ),
        mIndex ( index ),
        mAssetCount ( ledger,           keyFor_assetCount ( this->mIndex ),             0 ),
        mInventoryNonce ( ledger,       keyFor_inventoryNonce ( this->mIndex ),         0 ),
        mTransactionNonce ( ledger,     keyFor_transactionNonce ( this->mIndex ),       0 ),
        mName ( ledger,                 keyFor_name ( this->mIndex ),                   "" ),
        mBody ( ledger,                 keyFor_body ( this->mIndex )),
        mMinerInfo ( ledger,            keyFor_minerInfo ( this->mIndex )) {
    }
    
    //----------------------------------------------------------------//
    LedgerFieldODBM < AssetID::Index > getInventoryField ( size_t position ) {
    
        return LedgerFieldODBM < AssetID::Index >( this->mLedger, keyFor_inventoryField ( this->mIndex, position ), AssetID::NULL_INDEX );
    }
    
    //----------------------------------------------------------------//
    LedgerObjectFieldODBM < InventoryLogEntry > getInventoryLogEntryField ( u64 inventoryNonce ) {
    
        return LedgerObjectFieldODBM < InventoryLogEntry >( this->mLedger, keyFor_inventoryLogEntry ( this->mIndex, inventoryNonce ));
    }
    
    //----------------------------------------------------------------//
    LedgerFieldODBM < u64 > getTransactionLookupField ( string uuid ) {
    
        return LedgerFieldODBM < u64 >( this->mLedger, keyFor_transactionLookup ( this->mIndex, uuid ), 0 );
    }
    
    //----------------------------------------------------------------//
    void incAccountTransactionNonce ( u64 nonce, string uuid ) {
        
        if ( !( *this )) return;
        
        this->getTransactionLookupField ( uuid ).set ( nonce );
        this->mTransactionNonce.set ( nonce + 1 );
    }
};

} // namespace Volition
#endif
