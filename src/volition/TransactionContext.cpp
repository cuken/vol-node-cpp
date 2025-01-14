// Copyright (c) 2017-2018 Cryptogogue, Inc. All Rights Reserved.
// http://cryptogogue.com

#include <volition/Ledger.h>
#include <volition/LedgerFieldODBM.h>
#include <volition/TransactionContext.h>

namespace Volition {

//================================================================//
// TransactionContext
//================================================================//

//----------------------------------------------------------------//
TransactionContext::TransactionContext ( Ledger& ledger, AccountODBM& accountODBM, const KeyAndPolicy& keyAndPolicy, time_t time ) :
    mAccount ( *accountODBM.mBody.get ()),
    mAccountODBM ( accountODBM ),
    mIndex ( accountODBM.mAccountID ),
    mKeyAndPolicy ( keyAndPolicy ),
    mLedger ( ledger ),
    mTime ( time ) {
    
    if ( ledger.isGenesis ()) {
        this->mAccountEntitlements = *AccountEntitlements::getMasterEntitlements ();
        this->mKeyEntitlements = *KeyEntitlements::getMasterEntitlements ();
    }
    else {
        this->mAccountEntitlements = ledger.getEntitlements < AccountEntitlements >( *accountODBM.mBody.get ());
        this->mKeyEntitlements = ledger.getEntitlements < KeyEntitlements >( keyAndPolicy );
    }
    
    this->mFeeSchedule = ledger.getFeeSchedule ();
}

} // namespace Volition
