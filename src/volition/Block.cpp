// Copyright (c) 2017-2018 Cryptogogue, Inc. All Rights Reserved.
// http://cryptogogue.com

#include <volition/Block.h>
#include <volition/TheContext.h>
#include <volition/TheTransactionFactory.h>

namespace Volition {

//================================================================//
// Block
//================================================================//

//----------------------------------------------------------------//
bool Block::apply ( State& state ) {

    if ( state.getVersion () != this->mHeight ) return false;
    if ( !this->verify ( state )) return false;

    for ( size_t i = 0; i < this->mTransactions.size (); ++i ) {
        const AbstractTransaction& transaction = *this->mTransactions [ i ];
        if ( !transaction.apply ( state )) {
            return false;
        }
    }
    return true;
}

//----------------------------------------------------------------//
Block::Block () :
    mCycleID ( 0 ),
    mHeight ( 0 ) {
}

//----------------------------------------------------------------//
Block::Block ( string minerID, size_t cycleID, const CryptoKey& key, string hashAlgorithm ) :
    mMinerID ( minerID ),
    mCycleID ( cycleID ),
    mHeight ( 0 ) {
    
    Poco::Crypto::ECDSADigestEngine signature ( key, hashAlgorithm );
    Poco::DigestOutputStream signatureStream ( signature );
    signatureStream << this->mCycleID;
    signatureStream.close ();
    
    this->mAllure = signature.signature ();
}

//----------------------------------------------------------------//
Block::~Block () {
}

//----------------------------------------------------------------//
size_t Block::countTransactions () const {

    return this->mTransactions.size ();
}

//----------------------------------------------------------------//
const Signature& Block::getSignature () const {

    return this->mSignature;
}

//----------------------------------------------------------------//
string Block::getMinerID () const {

    return this->mMinerID;
}

//----------------------------------------------------------------//
size_t Block::getScore () const {

    if ( TheContext::get ().getScoringMode () == TheContext::ScoringMode::ALLURE ) {
        string allureString = Poco::DigestEngine::digestToHex ( this->mAllure );
        return std::hash < string >{}( allureString );
    }
    return strtol ( this->mMinerID.c_str (), 0, 10 );
}

//----------------------------------------------------------------//
void Block::pushTransaction ( shared_ptr < AbstractTransaction > transaction ) {

    this->mTransactions.push_back ( transaction );
}

//----------------------------------------------------------------//
void Block::setCycleID ( size_t cycleID ) {

    this->mCycleID = cycleID;
}

//----------------------------------------------------------------//
void Block::setAllure ( const Digest& allure ) {

    this->mAllure = allure;
}

//----------------------------------------------------------------//
void Block::setMinerID ( string minerID ) {

    this->mMinerID = minerID;
}

//----------------------------------------------------------------//
void Block::setPreviousBlock ( const Block& prevBlock ) {

    this->mHeight = prevBlock.mHeight + 1;
    this->mPrevDigest = prevBlock.mSignature.getDigest ();
}

//----------------------------------------------------------------//
const Digest& Block::sign ( const CryptoKey& key, string hashAlgorithm ) {
    
    this->mSignature = key.sign ( *this, hashAlgorithm );
    return this->mSignature.getSignature ();
}

//----------------------------------------------------------------//
bool Block::verify ( const State& state ) {

    const MinerInfo* minerInfo = state.getMinerInfoOrNil ( this->mMinerID );

    if ( minerInfo ) {
        return this->verify ( state, minerInfo->getPublicKey ());
    }

    // no miner info; must be the genesis block
    if ( this->mHeight > 0 ) return false; // genesis block must be height 0

    // check that it's the expected genesis block
    if ( !Poco::DigestEngine::constantTimeEquals ( TheContext::get ().getGenesisBlockDigest (), this->getSignature ().getDigest ())) return false;

    return this->verify ( state, TheContext::get ().getGenesisBlockKey ());
}

//----------------------------------------------------------------//
bool Block::verify ( const State& state, const CryptoKey& key ) {

    if ( this->mHeight > 0 ) {

        string hashAlgorithm = this->mSignature.getHashAlgorithm ();

        Poco::Crypto::ECDSADigestEngine signature ( key, hashAlgorithm );
        Poco::DigestOutputStream signatureStream ( signature );
        signatureStream << this->mCycleID;
        signatureStream.close ();

        if ( !signature.verify ( this->mAllure )) {
            return false;
        }
    }
    return key.verify ( this->mSignature, *this );
}

//================================================================//
// overrides
//================================================================//

//----------------------------------------------------------------//
void Block::AbstractSerializable_serialize ( AbstractSerializer& serializer ) {
    
    serializer.serialize ( "height",        this->mHeight );
    serializer.serialize ( "cycleID",       this->mCycleID );
    
    if ( this->mHeight > 0 ) {
        serializer.serialize ( "minerID",       this->mMinerID );
        serializer.serialize ( "prevDigest",    this->mPrevDigest );
        serializer.serialize ( "allure",        this->mAllure );
    }
    
    if ( serializer.getMode () != AbstractSerializer::SERIALIZE_DIGEST ) {
        serializer.serialize ( "signature",     this->mSignature );
    }
    
    serializer.serialize ( "transactions",  this->mTransactions );
}

} // namespace Volition
