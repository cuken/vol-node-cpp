// Copyright (c) 2017-2018 Cryptogogue, Inc. All Rights Reserved.
// http://cryptogogue.com

#include <volition/Block.h>
#include <volition/HTTPMiningMessenger.h>

namespace Volition {

//================================================================//
// HTTPGetJSONTask
//================================================================//
template < typename TYPE >
class HTTPGetJSONTask :
    public Poco::Task {
private:

    friend class HTTPMiningMessenger;

    TYPE                                mUserData;
    string                              mURL;
    Poco::JSON::Object::Ptr             mJSON;

    //----------------------------------------------------------------//
    void runTask () override {
    
        Poco::URI uri ( this->mURL );
        std::string path ( uri.getPathAndQuery ());

        try {
        
            Poco::Net::HTTPClientSession session ( uri.getHost (), uri.getPort ());
            Poco::Net::HTTPRequest request ( Poco::Net::HTTPRequest::HTTP_GET, path, Poco::Net::HTTPMessage::HTTP_1_1 );
            session.setKeepAlive ( true );
            session.setTimeout ( Poco::Timespan ( 30, 0 ));
            session.sendRequest ( request );
            
            Poco::Net::HTTPResponse response;
            std::istream& jsonStream = session.receiveResponse ( response );
            
            if ( response.getStatus () == Poco::Net::HTTPResponse::HTTP_OK ) {
                
                Poco::JSON::Parser parser;
                Poco::Dynamic::Var result = parser.parse ( jsonStream );
                this->mJSON = result.extract < Poco::JSON::Object::Ptr >();
            }
        }
        catch ( Poco::Exception& exc ) {
        
            string msg = exc.message ();
            if ( msg.size () > 0 ) {
                LGN_LOG ( VOL_FILTER_ROOT, INFO, "%s", exc.message ().c_str ());
            }
        }
    }

public:

    //----------------------------------------------------------------//
    HTTPGetJSONTask ( TYPE userData, string url ) :
        Task ( "HTTP GET JSON" ),
        mUserData ( userData ),
        mURL ( url ) {
    }
    
    //----------------------------------------------------------------//
    ~HTTPGetJSONTask () {
    }
};

//================================================================//
// HTTPMiningMessenger
//================================================================//

//----------------------------------------------------------------//
void HTTPMiningMessenger::deserailizeHeaderList ( const list < shared_ptr < const BlockHeader >>, Poco::JSON::Array::Ptr headersJSON ) {

    assert ( headersJSON );

    SerializableList < SerializableSharedConstPtr < BlockHeader >> headers;
    FromJSONSerializer::fromJSON ( headers, *headersJSON );
    
    list < shared_ptr < const BlockHeader >> responseHeaders;
    SerializableList < SerializableSharedConstPtr < BlockHeader >>::const_iterator headersIt = headers.cbegin ();
    for ( ; headersIt != headers.cend (); ++headersIt ) {
        responseHeaders.push_back ( *headersIt );
    }
}

//----------------------------------------------------------------//
HTTPMiningMessenger::HTTPMiningMessenger () :
    mTaskManager ( this->mThreadPool ) {
    
    this->mTaskManager.addObserver (
        Poco::Observer < HTTPMiningMessenger, Poco::TaskFinishedNotification > ( *this, &HTTPMiningMessenger::onTaskFinishedNotification )
    );
    
    this->mTaskManager.addObserver (
        Poco::Observer < HTTPMiningMessenger, Poco::TaskCancelledNotification > ( *this, &HTTPMiningMessenger::onTaskCancelledNotification )
    );

    this->mTaskManager.addObserver (
        Poco::Observer < HTTPMiningMessenger, Poco::TaskFailedNotification > ( *this, &HTTPMiningMessenger::onTaskFailedNotification )
    );
}

//----------------------------------------------------------------//
HTTPMiningMessenger::~HTTPMiningMessenger () {

    this->mTaskManager.cancelAll ();
    this->mTaskManager.joinAll ();
}

//----------------------------------------------------------------//
void HTTPMiningMessenger::onTaskCancelledNotification ( Poco::TaskCancelledNotification* pNf ) {
    
    Poco::TaskManager::TaskPtr taskPtr = pNf->task (); // TODO: this bullshit right here. pNf->task () increments the ref count. because of course it does.
    HTTPGetJSONTask < MiningMessengerRequest >* task = dynamic_cast < HTTPGetJSONTask < MiningMessengerRequest >* >( taskPtr.get ());
    
    if ( task ) {
        const MiningMessengerRequest& request = task->mUserData;
        this->enqueueErrorResponse ( request );
    }
}

//----------------------------------------------------------------//
void HTTPMiningMessenger::onTaskFailedNotification ( Poco::TaskFailedNotification* pNf ) {
        
    Poco::TaskManager::TaskPtr taskPtr = pNf->task (); // TODO: this bullshit right here. pNf->task () increments the ref count. because of course it does.
    HTTPGetJSONTask < MiningMessengerRequest >* task = dynamic_cast < HTTPGetJSONTask < MiningMessengerRequest >* >( taskPtr.get ());
    
    if ( task ) {
        const MiningMessengerRequest& request = task->mUserData;
        this->enqueueErrorResponse ( request );
    }
}

//----------------------------------------------------------------//
void HTTPMiningMessenger::onTaskFinishedNotification ( Poco::TaskFinishedNotification* pNf ) {

    Poco::TaskManager::TaskPtr taskPtr = pNf->task (); // TODO: this bullshit right here. pNf->task () increments the ref count. because of course it does.
    HTTPGetJSONTask < MiningMessengerRequest >* task = dynamic_cast < HTTPGetJSONTask < MiningMessengerRequest >* >( taskPtr.get ());
    
    if ( task ) {
        
        const MiningMessengerRequest& request = task->mUserData;
        Poco::JSON::Object::Ptr json = task->mJSON;
        
        if ( json ) {
        
            switch ( request.mRequestType ) {
                
                case MiningMessengerRequest::REQUEST_BLOCK: {
                
                    Poco::JSON::Object::Ptr blockJSON = json ? json->getObject ( "block" ) : NULL;
                    if ( blockJSON ) {
                        shared_ptr < Block > block = make_shared < Block >();
                        FromJSONSerializer::fromJSON ( *block, *blockJSON );
                        this->enqueueBlockResponse ( request, block );
                    }
                    break;
                }
                
                case MiningMessengerRequest::REQUEST_EXTEND_NETWORK: {
                
                    Poco::JSON::Array::Ptr minerListJSON = json ? json->getArray ( "miners" ) : NULL;
                    if ( minerListJSON ) {
                        SerializableSet < string > minerSet;
                        FromJSONSerializer::fromJSON ( minerSet, *minerListJSON );
                        this->enqueueExtendNetworkResponse ( request, minerSet );
                    }
                    break;
                }
                
                case MiningMessengerRequest::REQUEST_LATEST_HEADERS: {
                
                    Poco::JSON::Array::Ptr headersJSON = json ? json->getArray ( "headers" ) : NULL;
                    if ( headersJSON ) {
                        list < shared_ptr < const BlockHeader >> responseHeaders;
                        HTTPMiningMessenger::deserailizeHeaderList ( responseHeaders, headersJSON );
                        this->enqueueLatestHeadersResponse ( request, responseHeaders );
                    }
                    break;
                }
                
                case MiningMessengerRequest::REQUEST_MINER_INFO: {
                
                    Poco::JSON::Object::Ptr nodeJSON = json ? json->getObject ( "node" ) : NULL;
                    if ( nodeJSON ) {
                        string minerID  = nodeJSON->optValue < string >( "minerID", "" );
                        this->enqueueMinerInfoResponse ( request, minerID, request.mMinerURL );
                    }
                    break;
                }
                
                case MiningMessengerRequest::REQUEST_PREVIOUS_HEADERS: {
                
                    Poco::JSON::Array::Ptr headersJSON = json ? json->getArray ( "headers" ) : NULL;
                    if ( headersJSON ) {
                        list < shared_ptr < const BlockHeader >> responseHeaders;
                        HTTPMiningMessenger::deserailizeHeaderList ( responseHeaders, headersJSON );
                        this->enqueuePreviousHeadersResponse ( request, responseHeaders );
                    }
                    break;
                }
                
                default:
                    this->enqueueErrorResponse ( request );
                    break;
            }
        }
        else {
            this->enqueueErrorResponse ( request );
        }
    }
}

//================================================================//
// virtual
//================================================================//

//----------------------------------------------------------------//
void HTTPMiningMessenger::AbstractMiningMessenger_await () {

    this->mThreadPool.joinAll ();
}

//----------------------------------------------------------------//
void HTTPMiningMessenger::AbstractMiningMessenger_sendRequest ( const MiningMessengerRequest& request ) {
        
    string url;

    switch ( request.mRequestType ) {
        
        case MiningMessengerRequest::REQUEST_BLOCK:
            Format::write ( url, "%s/consensus/blocks/%s", request.mMinerURL.c_str (), request.mBlockDigest.toHex ().c_str ());
            break;
    
        case MiningMessengerRequest::REQUEST_EXTEND_NETWORK:
            Format::write ( url, "%s/miners?sample=random", request.mMinerURL.c_str ());
            break;
    
        case MiningMessengerRequest::REQUEST_LATEST_HEADERS:
            Format::write ( url, "%s/consensus/headers", request.mMinerURL.c_str ());
            break;
        
        case MiningMessengerRequest::REQUEST_MINER_INFO:
            Format::write ( url, "%s/node", request.mMinerURL.c_str ());
            break;
        
        case MiningMessengerRequest::REQUEST_PREVIOUS_HEADERS:
            Format::write ( url, "%s/consensus/headers?height=%llu", request.mMinerURL.c_str (), request.mHeight );
            break;
        
        default:
            assert ( false );
            break;
    }

    this->mTaskManager.start ( new HTTPGetJSONTask < MiningMessengerRequest >( request, url ));
}

} // namespace Volition
