// Copyright (c) 2017-2018 Cryptogogue, Inc. All Rights Reserved.
// http://cryptogogue.com

#include <volition/AccountODBM.h>
#include <volition/Block.h>
#include <volition/Transaction.h>
#include <volition/TransactionQueue.h>

namespace Volition {

//================================================================//
// MakerQueueInfo
//================================================================//
class MakerQueueInfo {
public:

    AccountID  mAccountIndex;
    u64             mNonce;

    //----------------------------------------------------------------//
    MakerQueueInfo () :
        mAccountIndex ( AccountID::NULL_INDEX ),
        mNonce ( 0 ) {
    }
};

//================================================================//
// MakerQueue
//================================================================//

//----------------------------------------------------------------//
shared_ptr < const Transaction > MakerQueue::getTransaction ( string uuid ) const {

    TransactionLookupConstIt transactionIt = this->mLookup.find ( uuid );
    return transactionIt != this->mLookup.cend () ? transactionIt->second : NULL;
}

//----------------------------------------------------------------//
bool MakerQueue::hasTransaction ( u64 nonce ) const {

    return ( this->mQueue.find ( nonce ) != this->mQueue.end ());
}

//----------------------------------------------------------------//
bool MakerQueue::hasTransactions () const {

    return ( this->mQueue.size () > 0 );
}

//----------------------------------------------------------------//
void MakerQueue::ignoreTransaction ( string message, string uuid ) {

    this->mQueueStatus = BLOCKED_ON_IGNORE;
    this->mTransactionStatus = TransactionStatus ( TransactionStatus::IGNORED, message, uuid );
}

//----------------------------------------------------------------//
bool MakerQueue::isBlocked () const {

    return ( this->mQueueStatus != STATUS_OK );
}

//----------------------------------------------------------------//
MakerQueue::MakerQueue () {
    
    this->setTransactionResult ( true );
}

//----------------------------------------------------------------//
shared_ptr < const Transaction > MakerQueue::nextTransaction ( u64 nonce ) const {

    TransactionQueueConstIt transactionIt = this->mQueue.find ( nonce );
    return transactionIt != this->mQueue.cend () ? transactionIt->second : NULL;
}

//----------------------------------------------------------------//
void MakerQueue::pushTransaction ( shared_ptr < const Transaction > transaction ) {

    this->mQueue [ transaction->getNonce ()] = transaction;
    this->mLookup [ transaction->getUUID ()] = transaction;

    this->setTransactionResult ( true );
}

//----------------------------------------------------------------//
void MakerQueue::prune ( u64 nonce ) {

    TransactionQueueIt transactionItCursor = this->mQueue.begin ();
    while ( transactionItCursor != this->mQueue.end ()) {

        TransactionQueueIt transactionIt = transactionItCursor++;

        if ( transactionIt->first < nonce ) {
            this->mLookup.erase ( transactionIt->second->getUUID ());
            this->mQueue.erase ( transactionIt );
        }
    }
}

//----------------------------------------------------------------//
void MakerQueue::setTransactionResult ( TransactionResult result ) {

    if ( result ) {
        this->mQueueStatus = STATUS_OK;
        this->mTransactionStatus = TransactionStatus ();
    }
    else {
        this->mQueueStatus = BLOCKED_ON_ERROR;
        this->mTransactionStatus = TransactionStatus (
            TransactionStatus::REJECTED,
            result.getMessage (),
            result.getUUID ()
        );
        this->mQueue.clear ();
    }
}

//================================================================//
// TransactionQueue
//================================================================//

//----------------------------------------------------------------//
void TransactionQueue::acceptTransaction ( shared_ptr < const Transaction > transaction ) {

    const TransactionMaker* maker = transaction->getMaker ();
    assert ( maker );
    
    this->mDatabase [ maker->getAccountName ()].pushTransaction ( transaction );
}

//----------------------------------------------------------------//
void TransactionQueue::fillBlock ( Ledger& chain, Block& block, Block::VerificationPolicy policy, u64 minimumGratuity ) {

    Ledger ledger;
    ledger.takeSnapshot ( chain );

    const u64 maxBlockWeight = ledger.getMaxBlockWeight ();
    u64 blockWeight = 0;

    map < string, MakerQueueInfo > infoCache;

    bool more = true;
    while ( more ) {

        more = false;

        MakerQueueIt makerQueueItCursor = this->mDatabase.begin ();
        while ( makerQueueItCursor != this->mDatabase.end ()) {
            MakerQueueIt makerQueueIt = makerQueueItCursor++;
            
            string accountName = makerQueueIt->first;
            MakerQueue& makerQueue = makerQueueIt->second;
          
            // skip if there's a cached error or if there aren't any transactions
            if ( makerQueue.isBlocked ()) continue;
            if ( !makerQueue.hasTransactions ()) continue;
          
            MakerQueueInfo info = infoCache [ accountName ];
          
            // make sure the account exists
            if ( info.mAccountIndex == AccountID::NULL_INDEX ) {
                info.mAccountIndex = ledger.getAccountID ( accountName );
                if ( info.mAccountIndex == AccountID::NULL_INDEX ) {
                    this->mDatabase.erase ( makerQueueIt );
                    continue;
                }
                info.mNonce = AccountODBM ( ledger, info.mAccountIndex ).mTransactionNonce.get ();
                infoCache [ accountName ] = info;
            }
            
            // get the next transaction
            shared_ptr < const Transaction > transaction = makerQueue.nextTransaction ( info.mNonce );
            if ( !transaction ) continue; // skip if no transaction
            
            u64 gratuity = transaction->getGratuity ();
            u64 expectedGratuity = transaction->getWeight () * minimumGratuity;
            if ( gratuity < expectedGratuity ) {
                makerQueue.ignoreTransaction ( Format::write ( "Transaction gratuity of %d less than minimum gratuity of %d.", gratuity, expectedGratuity ), transaction->getUUID ());
                continue;
            }
            
            u64 transactionWeight = transaction->getWeight ();
            if ( maxBlockWeight < transactionWeight ) {
                TransactionResult result ( Format::write ( "Transaction weight of %d exceeds maximum block size of %d.", transactionWeight, maxBlockWeight ));
                result.setTransactionDetails ( *transaction );
                makerQueue.setTransactionResult ( result );
                continue;
            }
            
            if (( blockWeight + transactionWeight ) > maxBlockWeight ) continue;
            
            // push a version in case the transaction fails
            ledger.pushVersion ();
            
            TransactionResult result = transaction->apply ( ledger, block.getTime (), policy );
            if ( result ) {
                // transaction succeeded!
                block.pushTransaction ( transaction );
                blockWeight += transactionWeight;
                info.mNonce = transaction->getNonce () + 1;
                infoCache [ accountName ] = info;
                
                more = ( more || makerQueue.hasTransaction ( info.mNonce ));
            }
            else {
                makerQueue.setTransactionResult ( result );
                ledger.popVersion ();
            }
        }
    }
}

//----------------------------------------------------------------//
TransactionStatus TransactionQueue::getLastStatus ( string accountName ) const {

    const MakerQueue* makerQueue = this->getMakerQueueOrNull ( accountName );
    if ( makerQueue ) {
        return makerQueue->mTransactionStatus;
    }
    return TransactionStatus ();
}

//----------------------------------------------------------------//
const MakerQueue* TransactionQueue::getMakerQueueOrNull ( string accountName ) const {

    MakerQueueConstIt makerQueueIt = this->mDatabase.find ( accountName );
    if ( makerQueueIt != this->mDatabase.end ()) {
        return &makerQueueIt->second;
    }
    return NULL;
}

//----------------------------------------------------------------//
bool TransactionQueue::hasTransaction ( string accountName, string uuid ) const {

    const MakerQueue* makerQueue = this->getMakerQueueOrNull ( accountName );
    if ( makerQueue ) {
        shared_ptr < const Transaction > transaction = makerQueue->getTransaction ( uuid );
        if ( transaction ) return true;
    }
    return false;
}

//----------------------------------------------------------------//
bool TransactionQueue::isBlocked ( string accountName ) const {

    const MakerQueue* makerQueue = this->getMakerQueueOrNull ( accountName );
    if ( makerQueue ) {
        return makerQueue->isBlocked ();
    }
    return false;
}

//----------------------------------------------------------------//
void TransactionQueue::processTransactions () {

//    while ( this->mIncoming.size ()) {
//
//        shared_ptr < const Transaction > transaction = this->mIncoming.front ();
//        this->mIncoming.pop_front ();
//        this->acceptTransaction ( transaction );
//    }
}

//----------------------------------------------------------------//
void TransactionQueue::pruneTransactions ( const Ledger& chain ) {

    const Ledger& ledger = chain;

    // TODO: fix this brute force
    MakerQueueIt makerQueueItCursor = this->mDatabase.begin ();
    while ( makerQueueItCursor != this->mDatabase.end ()) {
        MakerQueueIt makerQueueIt = makerQueueItCursor++;
        
        string accountName = makerQueueIt->first;
        MakerQueue& makerQueue = makerQueueIt->second;
    
        AccountID accountID = ledger.getAccountID ( accountName );
        if ( accountID != AccountID::NULL_INDEX ) {
            
            makerQueue.prune ( AccountODBM ( ledger, accountID ).mTransactionNonce.get ());
            
            if ( makerQueue.isBlocked ()) continue;
            if ( makerQueue.hasTransactions ()) continue;
        }
        this->mDatabase.erase ( makerQueueIt );
    }
}

//----------------------------------------------------------------//
void TransactionQueue::pushTransaction ( shared_ptr < const Transaction > transaction ) {

    if ( transaction ) {
//        this->mIncoming.push_back ( transaction );
        this->acceptTransaction ( transaction );
    }
}

//----------------------------------------------------------------//
void TransactionQueue::reset () {

    this->mDatabase.clear ();
//    this->mIncoming.clear ();
}

//----------------------------------------------------------------//
TransactionQueue::TransactionQueue () {
}

//----------------------------------------------------------------//
TransactionQueue::~TransactionQueue () {
}

//================================================================//
// overrides
//================================================================//

//----------------------------------------------------------------//

} // namespace Volition
