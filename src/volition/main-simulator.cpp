// Copyright (c) 2017-2018 Cryptogogue, Inc. All Rights Reserved.
// http://cryptogogue.com

#include <volition/simulator/Simulations.h>

//================================================================//
// ConsensusApp
//================================================================//
class ConsensusApp :
    public Poco::Util::Application {
public:

    //----------------------------------------------------------------//
    int main ( const vector < string > &args ) override {
    
        Volition::Simulator::SmallSimulation simulation;
        simulation.run ();
        return EXIT_OK;
    }
};

POCO_APP_MAIN ( ConsensusApp );
