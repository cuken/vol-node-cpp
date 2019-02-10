/* eslint-disable no-whitespace-before-property */
/* eslint-disable no-loop-func */

import { withAppStateAndUser }      from './AppStateProvider';
import BaseComponent                from './BaseComponent';
import * as crypto                  from './utils/crypto';
import React                        from 'react';
import { Header, Icon, Button }     from 'semantic-ui-react';

const STATUS_SEARCHING_FOR_BIDS         = 0;
const STATUS_NO_PROVIDERS_FOUND         = 1;
const STATUS_NO_BIDS_FOUND              = 2;
const STATUS_FOUND_BID                  = 3;
const STATUS_POSTING_TRANSACTION        = 4;
const STATUS_DONE                       = 5;

//================================================================//
// NewAccountScreen
//================================================================//
class NewAccountScreen extends BaseComponent {
    
    //----------------------------------------------------------------//
    async acceptBid () {

        const bid = this.state.bestBid;
        if ( !bid ) return;

        console.log ( 'ACCEPTING BID' );

        this.setState ({ status: STATUS_POSTING_TRANSACTION })

        const order = {
            accountName:    this.state.accountId,
            keyName:        'master',
            publicKey:      this.state.publicKey,
            amount:         0,
        };

        try {

            let data = await this.revocableFetchJSON ( bid.provider + '/bid', {
                method : 'POST',
                headers : { 'content-type': 'application/json' },
                body : JSON.stringify ( order )
            });

            console.log ( 'GOT TRANSACTION' );
            console.log ( data );
            this.postTransaction ( data );
        }
        catch ( error ) {

            // TODO: handle error
            console.log ( error );
        }
    }

    //----------------------------------------------------------------//
    constructor ( props ) {
        super ( props );

        // TODO: posible to show spinner while this sets up?

        // Create random string for account ID
        const accountId = 'vol_' + Math.random ().toString ( 36 ).substr ( 2, 9 );

        const mnemonic = crypto.generateMnemonic ();
        const key = crypto.mnemonicToKey ( mnemonic );
        const privateKey = key.getPrivateHex ();
        const publicKey = key.getPublicHex ();

        this.state = {
            accountId: accountId,
            privateKey: privateKey,
            publicKey: publicKey,
            seedPhrase: mnemonic,
            status: STATUS_SEARCHING_FOR_BIDS,
            searchCount: 0,
            providers: [],
            bestBid: false,
        };

        this.miners = [];

        this.revocableTimeout (() => { this.searchForBids (); }, 0 );
    }

    //----------------------------------------------------------------//
    async postTransaction ( transaction ) {

        const { minerURLs } = this.props.appState;

        console.log ( 'POSTING TRANSACTION' );
        console.log ( 'MINERS', minerURLs );

        this.setState ({ status: STATUS_POSTING_TRANSACTION })

        let success = 0;

        let postTransaction = async ( url ) => {
            
            console.log ( 'POST TO MINER:', url );

            try {
                await this.revocableFetch ( url + '/transactions', {
                    method : 'POST',
                    headers : { 'content-type': 'application/json' },
                    body : JSON.stringify ( transaction )
                });

                success++;
            }
            catch ( error ) {
                console.log ( error );
            }
        }

        let promises = [];
        minerURLs.forEach (( url ) => {
            promises.push ( postTransaction ( url ));
        });
        await this.revocableAll ( promises );

        if ( success > 0 ) {
            this.props.appState.saveAccount ( this.state.accountId, this.state.privateKey, this.state.publicKey );
            this.revocableSetState ({ status: STATUS_DONE });
        }
    }

    //----------------------------------------------------------------//
    render () {

        if ( this.state.status === STATUS_DONE ) return this.redirect ( '/accounts/' + this.state.accountId );

        return (
            <div>
                <Header>{ this.state.accountId }</Header>
                <Header>Mnemonic seed phrase</Header>
                <p>{ this.state.seedPhrase }</p>
                <Header>Keys</Header>
                <p>Public Key: { this.state.publicKey }</p>
                <p>Private Key: { this.state.privateKey }</p>
                <Header size = "small">You will not be able to recover your seed phrase and private key later</Header>

                { this.renderBid ()}
                { this.renderButton ()}
            </div>
        );
    }

    //----------------------------------------------------------------//
    renderBid () {

        const { status, bestBid } = this.state;

        if ( status === STATUS_SEARCHING_FOR_BIDS ) {
            return (<Header>SEARCHING FOR BIDS...</Header>);
        }
        else if ( status === STATUS_NO_PROVIDERS_FOUND ) {
            return (<Header>NO PROVIDERS FOUND</Header>);
        }
        else if ( status === STATUS_NO_BIDS_FOUND ) {
            return (<Header>NO BIDS FOUND</Header>);
        }
        else if ( bestBid ) {
            return (<Header>{ 'BEST BID: $' + bestBid.accountPrice }</Header>);
        }
    }

    //----------------------------------------------------------------//
    renderButton () {

        const isEnabled = ( this.state.status === STATUS_FOUND_BID );

        return (
            <Button primary icon labelPosition = "right" disabled = { !isEnabled } onClick = {() => { this.acceptBid ()}}>
                Accept bid
                <Icon name = "right arrow" />
            </Button>
        );
    }

    //----------------------------------------------------------------//
    async searchForBids () {

        const { marketURLs } = this.props.appState;

        if ( marketURLs.size <= 0 ) {
            this.setState ({ status: STATUS_NO_PROVIDERS_FOUND });
            return;
        }

        this.setState ({
            status: STATUS_SEARCHING_FOR_BIDS,
            searchCount: 0,
        })

        let searchCount = 0;
        let bestBid = false;

        let checkBid = async ( url ) => {

            try {
                const data = await ( await this.revocableFetch ( url + '/bid')).json ();

                if (( data.type === 'VOL_BID' ) && ( !bestBid  || ( data.volPrice <= bestBid.volPrice ))) {
                    bestBid = data;
                    bestBid.provider = url;
                }
            }
            catch ( error ) {
                console.log ( error );
            }
            this.setState ({ searchCount: searchCount++ });
        }

        let promises = [];
        marketURLs.forEach (( url ) => {
            promises.push ( checkBid ( url ));
        });
        await Promise.all ( promises );
        
        if ( bestBid ) {

            let bid = {
                volPrice:       bestBid.volPrice,
                maxSale:        bestBid.maxSale,
                accountPrice:   bestBid.accountPrice,
                provider:       bestBid.provider,
            };

            this.setState ({
                status: STATUS_FOUND_BID,
                bestBid: bid,
            });
        }
        else {
            this.setState ({ status: STATUS_NO_BIDS_FOUND })
        }
    }
}

export default withAppStateAndUser ( NewAccountScreen );