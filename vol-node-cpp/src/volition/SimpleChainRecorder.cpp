// Copyright (c) 2017-2018 Cryptogogue, Inc. All Rights Reserved.
// http://cryptogogue.com

#include <volition/Block.h>
#include <volition/FileSys.h>
#include <volition/Miner.h>
#include <volition/SimpleChainRecorder.h>

namespace Volition {

//================================================================//
// Miner
//================================================================//

//----------------------------------------------------------------//
SimpleChainRecorder::SimpleChainRecorder ( const Miner& miner, string path ) {
    
    Poco::Path basePath ( path );
    basePath.makeAbsolute ();
    
    this->mBasePath = basePath.toString ();
    
    mkdir ( this->mBasePath.c_str (), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH );
    
    const Chain* chain = miner.getBestBranch ();
    if ( !chain ) return;

    shared_ptr < Block > genesisBlock = chain->getBlock ( 0 );
    assert ( genesisBlock );
    
    this->mGenesisHash = genesisBlock->getHash ();
    
    this->mChainFolderPath = Format::write ( "%s/%s/", this->mBasePath.c_str (), this->mGenesisHash.c_str ());
    mkdir ( this->mChainFolderPath.c_str (), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH );
    
    this->mBlocksFolderPath = Format::write ( "%s%s/", this->mChainFolderPath.c_str (), BLOCK_FOLDERNAME );
    mkdir ( this->mBlocksFolderPath.c_str (), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH );
    
    this->mIndexFilePath = Format::write ( "%s%s", this->mChainFolderPath.c_str (), INDEX_FILENAME );
}

//----------------------------------------------------------------//
SimpleChainRecorder::~SimpleChainRecorder () {
}

//================================================================//
// overrides
//================================================================//

//----------------------------------------------------------------//
void SimpleChainRecorder::AbstractChainRecorder_loadChain ( Miner& miner ) const {
    
    const Chain* chain = miner.getBestBranch ();
    if ( !chain ) return;
    assert ( chain->countBlocks () == 1 );
    
    // legacy
    if ( FileSys::exists ( this->mIndexFilePath )) {
    
        SerializableVector < string > hashes;
        FromJSONSerializer::fromJSONFile ( hashes, this->mIndexFilePath );
        
        size_t length = hashes.size ();
        for ( size_t i = 0; i < length; ++i ) {
        
            string hash = hashes [ i ];
            
            if ( i == 0 ) {
                assert ( hash == this->mGenesisHash );
                continue;
            }
            
            string blockPath = Format::write ( "%s%s.json", this->mBlocksFolderPath.c_str (), hash.c_str ());
            
            Block block;
            FromJSONSerializer::fromJSONFile ( block, blockPath );
            Miner::SubmissionResponse response = miner.submitBlock ( block );
            assert ( response == Miner::SubmissionResponse::ACCEPTED );
        }
        
        string indexFileBackup = Format::write ( "%s.bak", this->mIndexFilePath.c_str ());
        rename ( this->mIndexFilePath.c_str (), indexFileBackup.c_str ());
    }
    else {
        
        for ( size_t i = 1; true; ++i ) {
        
            string blockPath = Format::write ( "%sblock_%d.json", this->mChainFolderPath.c_str (), i );
            if ( !FileSys::exists ( blockPath )) break;
            
            Block block;
            FromJSONSerializer::fromJSONFile ( block, blockPath );
            Miner::SubmissionResponse response = miner.submitBlock ( block );
            assert ( response == Miner::SubmissionResponse::ACCEPTED );
        }
    }
}

//----------------------------------------------------------------//
void SimpleChainRecorder::AbstractChainRecorder_saveChain ( const Miner& miner ) {

    const Chain* chain = miner.getBestBranch ();
    if ( !chain ) return;

    size_t length = chain->countBlocks ();
    for ( size_t i = 0; i < length; ++i ) {
    
        shared_ptr < Block > block = chain->getBlock ( i );
        assert ( block );
                
        string blockPath = Format::write ( "%sblock_%d.json", this->mChainFolderPath.c_str (), ( int )i );
        if ( FileSys::exists ( blockPath )) continue;
        ToJSONSerializer::toJSONFile ( *block, blockPath );
    }
}

} // namespace Volition
