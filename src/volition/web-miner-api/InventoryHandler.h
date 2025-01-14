// Copyright (c) 2017-2018 Cryptogogue, Inc. All Rights Reserved.
// http://cryptogogue.com

#ifndef VOLITION_WEBMINERAPI_INVENTORYHANDLER_H
#define VOLITION_WEBMINERAPI_INVENTORYHANDLER_H

#include <volition/Block.h>
#include <volition/AbstractAPIRequestHandler.h>
#include <volition/InventoryLogEntry.h>
#include <volition/TheTransactionBodyFactory.h>
#include <volition/MinerAPIFactory.h>

namespace Volition {
namespace WebMinerAPI {

//================================================================//
// InventoryHandler
//================================================================//
class InventoryHandler :
    public AbstractMinerAPIRequestHandler {
public:

    SUPPORTED_HTTP_METHODS ( HTTP::GET )

    //----------------------------------------------------------------//
    HTTPStatus AbstractMinerAPIRequestHandler_handleRequest ( HTTP::Method method, Ledger& ledger, const Poco::JSON::Object& jsonIn, Poco::JSON::Object& jsonOut ) const override {
        UNUSED ( method );
        UNUSED ( jsonIn );
    
        try {
        
            string accountName = this->getMatchString ( "accountName" );
        
            AccountID accountID = ledger.getAccountID ( accountName );
            
            AccountODBM accountODBM ( ledger, accountID );
            jsonOut.set ( "assetCount", accountODBM.mAssetCount.get ( 0 ));
            jsonOut.set ( "inventoryNonce", accountODBM.mInventoryNonce.get ( 0 ));
        
            shared_ptr < const InventoryLogEntry > baseEntry = accountODBM.getInventoryLogEntryField ( 0 ).get ();
            if ( baseEntry ) {
                jsonOut.set ( "inventoryTimestamp", ( string )baseEntry->mTime );
            }
            
            const Schema& schema = ledger.getSchema ();
            jsonOut.set ( "schemaVersion", ToJSONSerializer::toJSON ( schema.getVersion ()));
            jsonOut.set ( "schemaHash", ledger.getSchemaHash ());
        }
        catch ( ... ) {
            return Poco::Net::HTTPResponse::HTTP_BAD_REQUEST;
        }
        return Poco::Net::HTTPResponse::HTTP_OK;
    }
};

} // namespace TheWebMinerAPI
} // namespace Volition
#endif
