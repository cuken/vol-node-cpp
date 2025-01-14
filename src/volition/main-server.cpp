// Copyright (c) 2017-2018 Cryptogogue, Inc. All Rights Reserved.
// http://cryptogogue.com

#include <padamose/padamose.h>
#include <volition/Block.h>
#include <volition/FileSys.h>
#include <volition/RouteTable.h>
#include <volition/Singleton.h>
#include <volition/TheTransactionBodyFactory.h>
#include <volition/version.h>
#include <volition/MinerActivity.h>
#include <volition/MinerAPIFactory.h>

using namespace Volition;

//================================================================//
// ServerApp
//================================================================//
class ServerApp :
public Poco::Util::ServerApplication {
protected:

    shared_ptr < MinerActivity > mMinerActivity;

    //----------------------------------------------------------------//
    void defineOptions ( Poco::Util::OptionSet& options ) override {
        Application::defineOptions ( options );
        
        options.addOption (
            Poco::Util::Option ( "config", "c", "path to configuration file" )
                .required ( false )
                .argument ( "value", true )
                .binding ( "config" )
        );
        
        options.addOption (
            Poco::Util::Option ( "control-key", "", "path to public key for verifying control commands." )
                .required ( false )
                .argument ( "value", true )
                .binding ( "control-key" )
        );
        
        options.addOption (
            Poco::Util::Option ( "control-level", "", "miner control level ('config', 'admin')" )
                .required ( false )
                .argument ( "value", true )
                .binding ( "control-level" )
        );
        
        options.addOption (
            Poco::Util::Option ( "delay-fixed", "df", "set update interval (in milliseconds)" )
                .required ( false )
                .argument ( "value", true )
                .binding ( "delay-fixed" )
        );
        
        options.addOption (
            Poco::Util::Option ( "delay-variable", "dv", "set update interval (in milliseconds)" )
                .required ( false )
                .argument ( "value", true )
                .binding ( "delay-variable" )
        );
        
        options.addOption (
            Poco::Util::Option ( "dump", "dmp", "dump ledger to sqlite file" )
                .required ( false )
                .argument ( "value", true )
                .binding ( "dump" )
        );
        
        options.addOption (
            Poco::Util::Option ( "genesis", "g", "path to the genesis file" )
                .required ( false )
                .argument ( "value", true )
                .binding ( "genesis" )
        );
        
        options.addOption (
            Poco::Util::Option ( "keyfile", "k", "path to public miner key file" )
                .required ( false )
                .argument ( "value", true )
                .binding ( "keyfile" )
        );
        
        options.addOption (
            Poco::Util::Option ( "logpath", "l", "path to log folder" )
                .required ( false )
                .argument ( "value", true )
                .binding ( "logpath" )
        );
        
        options.addOption (
            Poco::Util::Option ( "miner", "m", "miner name" )
                .required ( false )
                .argument ( "value", true )
                .binding ( "miner" )
        );
        
        options.addOption (
            Poco::Util::Option ( "persist", "", "path to folder for persist files." )
                .required ( false )
                .argument ( "value", true )
                .binding ( "persist" )
        );
        
        options.addOption (
            Poco::Util::Option ( "port", "p", "set port to serve from" )
                .required ( false )
                .argument ( "value", true )
                .binding ( "port" )
        );
    }

    //----------------------------------------------------------------//
    int main ( const vector < string >& ) override {
                
        Poco::Util::AbstractConfiguration& configuration = this->config ();
        
        string configfile = configuration.getString ( "config", "" );
        
        if ( configfile.size () > 0 ) {
            this->loadConfiguration ( configfile, PRIO_APPLICATION - 1 );
        }
        else {
            this->loadConfiguration ( PRIO_APPLICATION - 1 );
        }
        
        this->printProperties ();
        
        string controlKeyfile           = configuration.getString       ( "control-key", "" );
        string controlLevel             = configuration.getString       ( "control-level", "" );
        string dump                     = configuration.getString       ( "dump", "" );
        string genesis                  = configuration.getString       ( "genesis", "genesis.json" );
        int fixedDelay                  = configuration.getInt          ( "delay-fixed", MinerActivity::DEFAULT_FIXED_UPDATE_MILLIS );
        int variableDelay               = configuration.getInt          ( "delay-variable", MinerActivity::DEFAULT_VARIABLE_UPDATE_MILLIS );
        string keyfile                  = configuration.getString       ( "keyfile" );
        string logpath                  = configuration.getString       ( "logpath", "" );
        string minerID                  = configuration.getString       ( "miner", "" );
        string nodelist                 = configuration.getString       ( "nodelist", "" );
        string persist                  = configuration.getString       ( "persist", "persist-chain" );
        int port                        = configuration.getInt          ( "port", 9090 );
        string sslCertFile              = configuration.getString       ( "openSSL.server.certificateFile", "" );
        
        if ( logpath.size () > 0 ) {
        
            FileSys::createDirectories ( logpath );
            
            time_t t;
            time ( &t );
            string timeStr = Poco::DateTimeFormatter ().format ( Poco::Timestamp ().fromEpochTime ( t ), "%Y-%m-%d-%H%M%S" );
            
            string logname = Format::write ( "%s/%s-%s.log", logpath.c_str (), minerID.c_str (), timeStr.c_str ());
            freopen ( logname.c_str (), "w+", stderr );
        }
        
        LOG_F ( INFO, "\nHello from VOLITION main.cpp!" );
        LOG_F ( INFO, "commit: %s", VOLITION_GIT_COMMIT_STR );
        LOG_F ( INFO, "build: %s %s", VOLITION_BUILD_DATE_STR, VOLITION_GIT_TAG_STR );
        
        this->mMinerActivity = make_shared < MinerActivity >();
        this->mMinerActivity->setMinerID ( minerID );
        
        if ( controlKeyfile.size ()) {
        
            CryptoPublicKey controlKey;
            controlKey.load ( controlKeyfile );
            
            if ( !controlKey ) {
                LOG_F ( INFO, "CONTROL KEY NOT FOUND: %s", controlKeyfile.c_str ());
                return Application::EXIT_CONFIG;
            }
            LOG_F ( INFO, "CONTROL KEY: %s", controlKeyfile.c_str ());
            this->mMinerActivity->setControlKey ( controlKey );
        }
        
        switch ( FNV1a::hash_64 ( controlLevel )) {
        
            case FNV1a::const_hash_64 ( "config" ):
                LOG_F ( INFO, "CONTROL LEVEL: CONFIG" );
                this->mMinerActivity->setControlLevel ( Miner::CONTROL_CONFIG );
                break;
            
            case FNV1a::const_hash_64 ( "admin" ):
               LOG_F ( INFO, "CONTROL LEVEL: ADMIN" );
                this->mMinerActivity->setControlLevel ( Miner::CONTROL_ADMIN );
                break;
        }
        
        LOG_F ( INFO, "LOADING GENESIS BLOCK: %s", genesis.c_str ());
        if ( !FileSys::exists ( genesis )) {
            LOG_F ( INFO, "...BUT THE FILE DOES NOT EXIST!" );
            return Application::EXIT_CONFIG;
        }
        
        shared_ptr < Block > genesisBlock = Miner::loadGenesisBlock ( genesis );
        
        if ( !genesisBlock ) {
            LOG_F ( INFO, "UNABLE TO LOAD GENESIS BLOCK" );
            return Application::EXIT_CONFIG;
        }
        
        if ( persist.size ()) {
            this->mMinerActivity->persist ( persist, genesisBlock );
        }
        else {
            this->mMinerActivity->setGenesis ( genesisBlock );
        }
        
        if ( this->mMinerActivity->getLedger ().countBlocks () == 0 ) {
            LOG_F ( INFO, "MISSING OR CORRUPT GENESIS BLOCK" );
            return Application::EXIT_CONFIG;
        }
        
        if ( dump.size ()) {
            this->mMinerActivity->getLedger ().dump ( dump );
            return Application::EXIT_OK;
        }
        
        LOG_F ( INFO, "LOADING KEY FILE: %s\n", keyfile.c_str ());
        if ( !FileSys::exists ( keyfile )) {
            LOG_F ( INFO, "...BUT THE FILE DOES NOT EXIST!" );
            return Application::EXIT_CONFIG;
        }
        
        this->mMinerActivity->loadKey ( keyfile );
        this->mMinerActivity->affirmKey ();
        this->mMinerActivity->affirmVisage ();
        this->mMinerActivity->setVerbose ();
        this->mMinerActivity->setReportMode ( Miner::REPORT_ALL_BRANCHES );
        this->mMinerActivity->setFixedUpdateDelayInMillis (( u32 )fixedDelay );
        this->mMinerActivity->setVariableUpdateDelayInMillis (( u32 )variableDelay );
        
        LOG_F ( INFO, "MINER ID: %s", this->mMinerActivity->getMinerID ().c_str ());

        this->serve ( port, sslCertFile.length () > 0 );
        
        LOG_F ( INFO, "SHUTDOWN: main" );
        
        return Application::EXIT_OK;
    }
    
    //----------------------------------------------------------------//
    const char* name () const override {
    
        return "volition";
    }
    
    //----------------------------------------------------------------//
    void printProperties ( const std::string& base = "" ) {
    
        Poco::Util::AbstractConfiguration::Keys keys;
        this->config ().keys ( base, keys );
    
        if ( keys.empty ()) {
            if ( config ().hasProperty ( base )) {
                std::string msg;
                msg.append ( base );
                msg.append ( " = " );
                msg.append ( config ().getString ( base ));
                //logger ().information ( msg );
                printf ( "%s\n", msg.c_str ());
            }
        }
        else {
            for ( Poco::Util::AbstractConfiguration::Keys::const_iterator it = keys.begin (); it != keys.end (); ++it ) {
                std::string fullKey = base;
                if ( !fullKey.empty ()) fullKey += '.';
                fullKey.append ( *it );
                printProperties ( fullKey );
            }
        }
    }
    
    //----------------------------------------------------------------//
    void serve ( int port, bool ssl ) {

        Poco::ThreadPool threadPool;

        Poco::Net::HTTPServer server (
            new MinerAPIFactory ( this->mMinerActivity ),
            threadPool,
            ssl ? Poco::Net::SecureServerSocket (( Poco::UInt16 )port ) : Poco::Net::ServerSocket (( Poco::UInt16 )port ),
            new Poco::Net::HTTPServerParams ()
        );
        
        server.start ();
        this->mMinerActivity->start ();

        LOG_F ( INFO, "\nSERVING YOU BLOCKCHAIN REALNESS ON PORT: %d\n", port );
        this->mMinerActivity->waitForShutdown ();
        LOG_F ( INFO, "SHUTDOWN: this->mMinerActivity->waitForShutdown ()" );

        server.stop ();
        threadPool.stopAll ();
        
        {
            // wait for miner activity to fully shut down
            ScopedMinerLock scopedLock ( this->mMinerActivity );
            this->mMinerActivity->shutdown ( false );
        }
        LOG_F ( INFO, "SHUTDOWN: ~serve" );
    }

public:

    //----------------------------------------------------------------//
    ServerApp () {
    
        Poco::Net::initializeSSL ();
    }
    
    //----------------------------------------------------------------//
    ~ServerApp () {
    
        Poco::Net::uninitializeSSL ();
        LOG_F ( INFO, "SHUTDOWN: ~ServerApp" );
    }
    
    //----------------------------------------------------------------//
    void shutdown () {
    
        this->mMinerActivity->shutdown ( true );
    }
};

//================================================================//
// main
//================================================================//

static shared_ptr < ServerApp > sApp;

//----------------------------------------------------------------//
void onSignal ( int sig );
void onSignal ( int sig ) {

    signal ( sig, SIG_IGN );

    LOG_F ( INFO, "CAUGHT A SIGNAL - SHUTTING DOWN." );

    if ( sApp ) {
        sApp->shutdown ();
    }
}

//----------------------------------------------------------------//
int main ( int argc, char** argv ) {

    signal ( SIGINT, onSignal );
    signal ( SIGTERM, onSignal );
    signal ( SIGQUIT, onSignal );

    // force line buffering even when running as a spawned process
    setvbuf ( stdout, NULL, _IOLBF, 0 );
    setvbuf ( stderr, NULL, _IOLBF, 0 );

    Lognosis::setFilter ( PDM_FILTER_ROOT, Lognosis::OFF );
    Lognosis::init ( argc, argv );

    sApp = make_shared < ServerApp >();
    int result = sApp->run ( argc, argv );
    sApp = NULL;
    return result;
}
