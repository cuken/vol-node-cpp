// Copyright (c) 2017-2018 Cryptogogue, Inc. All Rights Reserved.
// http://cryptogogue.com

#include <volition/SyncChainTask.h>

namespace Volition {

//================================================================//
// SyncChainTask
//================================================================//

//----------------------------------------------------------------//
SyncChainTask::SyncChainTask ( string minerID, string url ) :
    Task ( "SYNC CHAIN" ),
    mMinerID ( minerID ),
    mURL ( url ) {
}

//----------------------------------------------------------------//
SyncChainTask::~SyncChainTask () {
}

//================================================================//
// overrides
//================================================================//

//----------------------------------------------------------------//
void SyncChainTask::runTask () {

    Poco::URI uri ( this->mURL );
    std::string path ( uri.getPathAndQuery ());

    this->mBlockQueueEntry = make_unique < BlockQueueEntry >();
    this->mBlockQueueEntry->mMinerID = this->mMinerID;
    this->mBlockQueueEntry->mBlock = NULL;

    try {
        Poco::Net::HTTPClientSession session ( uri.getHost (), uri.getPort ());
        Poco::Net::HTTPRequest request ( Poco::Net::HTTPRequest::HTTP_GET, path, Poco::Net::HTTPMessage::HTTP_1_1 );
        Poco::Net::HTTPResponse response;
        //session.setKeepAlive ( true );
        session.setTimeout ( Poco::Timespan ( 1, 0 ));
        session.sendRequest ( request );
        
        if ( response.getStatus () == Poco::Net::HTTPResponse::HTTP_OK ) {
        
            std::istream& jsonStream = session.receiveResponse ( response );
        
            //string jsonString ( istreambuf_iterator < char >( jsonStream ), {});
            //LGN_LOG ( VOL_FILTER_ROOT, INFO, "JSON: %s", jsonString ());
        
            Poco::JSON::Parser parser;
            Poco::Dynamic::Var result = parser.parse ( jsonStream );
            Poco::JSON::Object::Ptr json = result.extract < Poco::JSON::Object::Ptr >();
        
            json = json ? json->getObject ( "block" ) : NULL;
        
            if ( json ) {
                shared_ptr < Block > block = make_shared < Block >();
                FromJSONSerializer::fromJSON ( *block, *json );
                this->mBlockQueueEntry->mBlock = block;
            }
        }
    }
    catch ( Poco::Exception& exc ) {
        string msg = exc.message ();
        if ( msg.size () > 0 ) {
            LGN_LOG ( VOL_FILTER_ROOT, INFO, "%s", exc.message ().c_str ());
        }
    }
}

} // namespace Volition
