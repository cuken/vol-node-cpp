// Copyright (c) 2017-2018 Cryptogogue, Inc. All Rights Reserved.
// http://cryptogogue.com

#ifndef VOLITION_WEBMINERAPI_CONSENSUSPEEKHANDLER_H
#define VOLITION_WEBMINERAPI_CONSENSUSPEEKHANDLER_H

#include <volition/Block.h>
#include <volition/AbstractAPIRequestHandler.h>
#include <volition/TheTransactionBodyFactory.h>
#include <volition/MinerAPIFactory.h>

namespace Volition {
namespace WebMinerAPI {

//================================================================//
// ConsensusPeekHandler
//================================================================//
class ConsensusPeekHandler :
    public AbstractMinerAPIRequestHandler {
public:

    static const size_t HEADER_BATCH_SIZE = 32;

    SUPPORTED_HTTP_METHODS ( HTTP::GET )
    
    //----------------------------------------------------------------//
    void peek ( Ledger& ledger, Poco::JSON::Object& jsonOut, string key, u64 height, u64 totalBlocks ) const {
    
        if ( height < totalBlocks ) {
    
            Poco::JSON::Object::Ptr headerJSON = new Poco::JSON::Object ();
            
            shared_ptr < const BlockHeader > header = ledger.getHeader ( height );
            headerJSON->set ( "height", header->getHeight ());
            headerJSON->set ( "digest", header->getDigest ().toHex ());
            
            jsonOut.set ( key, headerJSON );
        }
    }

    //----------------------------------------------------------------//
    HTTPStatus AbstractMinerAPIRequestHandler_handleRequest ( HTTP::Method method, Ledger& ledger, const Poco::JSON::Object& jsonIn, Poco::JSON::Object& jsonOut ) const override {
        UNUSED ( method );
        UNUSED ( jsonIn );

        try {
            
            jsonOut.set ( "minerID", this->mWebMiner->getMinerID ());
            
            size_t totalBlocks = ledger.countBlocks ();
            jsonOut.set ( "totalBlocks", totalBlocks );

            this->peek ( ledger, jsonOut, "peek", this->optQuery ( "peek", 0 ), totalBlocks );
            this->peek ( ledger, jsonOut, "prev", this->optQuery ( "prev", 0 ), totalBlocks );
            
            u64 sampleMiners = this->optQuery ( "sampleMiners", 0 );
            
            if ( sampleMiners ) {
            
                set < string > minerURLs = this->mWebMiner->sampleOnlineMinerURLs ( sampleMiners );
                Poco::JSON::Array::Ptr minerURLsJSON = new Poco::JSON::Array ();
            
                set < string >::const_iterator urlIt = minerURLs.cbegin ();
                for ( ; urlIt != minerURLs.cend (); ++urlIt ) {
                    minerURLsJSON->add ( *urlIt );
                }
                jsonOut.set ( "miners", minerURLsJSON );
            }
            
            return Poco::Net::HTTPResponse::HTTP_OK;
        }
        catch ( ... ) {
            return Poco::Net::HTTPResponse::HTTP_BAD_REQUEST;
        }
        return Poco::Net::HTTPResponse::HTTP_NOT_FOUND;
    }
};

} // namespace TheWebMinerAPI
} // namespace Volition
#endif
