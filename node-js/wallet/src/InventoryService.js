/* eslint-disable no-whitespace-before-property */

import { AssetLayout }                          from './AssetLayout';
import { barcodeToSVG }                         from './util/pdf417';
import { Service }                              from './Service';
import { action, computed, extendObservable, observable, observe, runInAction } from 'mobx';
import { Binding }                              from './schema/Binding';
import { Schema }                               from './schema/Schema';
import { buildSchema, op, LAYOUT_COMMAND }      from './schema/SchemaBuilder';
import { JUSTIFY }                              from './util/TextFitter';
import handlebars                               from 'handlebars';
import _                                        from 'lodash';
import * as opentype                            from 'opentype.js';

//================================================================//
// InventoryService
//================================================================//
export class InventoryService extends Service {

    @observable loading         = true;

    @observable assets          = {};
    @observable assetLayouts    = {};
    @observable schema          = new Schema (); // empty schema
    @observable binding         = new Binding (); // empty binding

    //----------------------------------------------------------------//
    composeAssetContext ( asset, filters, overrideContext ) {

        let context = {};

        for ( let fieldName in asset.fields ) {

            const field = asset.fields [ fieldName ];
            const alternates = field.alternates;

            context [ fieldName ] = field.value;

            for ( let i in filters ) {
                const filter = filters [ i ];
                if ( _.has ( alternates, filter )) {
                    context [ fieldName ] = alternates [ filter ];
                }
            }
        }
        return Object.assign ( context, overrideContext );
    }

    //----------------------------------------------------------------//
    constructor ( appState ) {
        super ();

        extendObservable ( this, {
            appState:   appState,
        });

        this.templates = {};
        this.layouts = {};
        this.fonts = {};

        if ( appState ) {
            this.fetchInventory ( appState.accountID, appState.node );
        }

        if ( appState ) {
            observe ( appState, 'assetsUtilized', ( change ) => {
                console.log ( 'ASSETS UTILIZED DID CHANGE' );
                this.refreshBinding ();
            });
        }
    }

    //----------------------------------------------------------------//
    async fetchInventory ( accountID, minerURL ) {

        try {
            console.log ( 'FETCH INVENTORY', accountID, minerURL );

            const schemaJSON        = await this.revocableFetchJSON ( minerURL + '/schemas', null, 20000 );
            console.log ( schemaJSON );

            const inventoryJSON     = await this.revocableFetchJSON ( minerURL + '/accounts/' + accountID + '/inventory', null, 20000 );
            console.log ( inventoryJSON );

            let assets = {};
            for ( let asset of inventoryJSON.inventory ) {
                assets [ asset.assetID ] = asset;
            }
            await this.update ( schemaJSON.schemas, assets );
        }
        catch ( error ) {
            console.log ( error );
        }
        this.finishLoading ();
    }

    //----------------------------------------------------------------//
    @action
    finishLoading () {
        this.loading = false;
    }

    //----------------------------------------------------------------//
    @computed
    get availableAssetsArray () {

        const assetsUtilized = this.appState ? this.appState.assetsUtilized : [];

        let assets = [];
        for ( let assetID in this.assets ) {
            if ( !assetsUtilized.includes ( assetID )) {
                assets.push ( this.assets [ assetID ]);
            }
        }
        return assets;
    }

    //----------------------------------------------------------------//
    @computed
    get availableAssetsByID () {

        const assetsUtilized = this.appState ? this.appState.assetsUtilized : [];

        let assets = {};
        for ( let assetID in this.assets ) {
            if ( !assetsUtilized.includes ( assetID )) {
                assets [ assetID ] = this.assets [ assetID ];
            }
        }
        return assets;
    }

    //----------------------------------------------------------------//
    getCraftingMethodBindings () {
        return this.binding.methodBindingsByName;
    }

    //----------------------------------------------------------------//
    getCraftingMethodBindingsForAssetID ( assetID ) {
        return this.binding.methodBindingsByAssetID [ assetID ];
    }

    //----------------------------------------------------------------//
    methodIsValid ( methodName, assetID ) {
        return ( methodName !== '' ) && this.binding.methodIsValid ( methodName, assetID );
    }

    //----------------------------------------------------------------//
    @action
    refreshAssetLayouts () {

        this.assetLayouts = {};
        for ( let assetId in this.assets ) {
            this.assetLayouts [ assetId ] = new AssetLayout ( this, assetId, [ 'EN', 'RGB' ]);
        }
    }

    //----------------------------------------------------------------//
    @action
    refreshBinding ( schema, assets ) {

        this.schema = schema || this.schema;
        this.assets = assets || this.assets;

        const availableAssetsByID = this.availableAssetsByID;
        console.log ( 'REFRESH BINDING:', availableAssetsByID );
        this.binding = this.schema.generateBinding ( availableAssetsByID );
    }

    //----------------------------------------------------------------//
    @action
    reset ( template ) {

        this.loading = true;

        if ( template ) {
            this.update ([ template ]);
        }
    }

    //----------------------------------------------------------------//
    async update ( templates, assets ) {

        const fetchFont = async ( url ) => {
            if ( !url ) return false;
            const response  = await this.revocableFetch ( url );
            const buffer    = await response.arrayBuffer ();
            return opentype.parse ( buffer );
        }

        let schema = new Schema ();

        for ( let template of templates ) {

            await schema.applyTemplate ( template );

            for ( let layoutName in template.layouts ) {
                
                const layout = _.cloneDeep ( template.layouts [ layoutName ]);

                for ( let command of layout.commands ) {

                    if ( command.type === LAYOUT_COMMAND.DRAW_TEXT_BOX ) {
                        for ( let segment of command.segments ) {
                            segment.template = handlebars.compile ( segment.template );
                        }
                    }
                    else {
                        command.template = handlebars.compile ( command.template );
                    }
                }
                this.layouts [ layoutName ] = layout;
            }

            for ( let name in template.fonts ) {

                try {
                    const fontDesc = template.fonts [ name ];
                    const faces = {};

                    for ( let face in fontDesc ) {
                        const url = fontDesc [ face ];
                        console.log ( 'FETCHING FONT', face, url );
                        faces [ face ] = await fetchFont ( url );
                    }
                    this.fonts [ name ] = faces;
                }
                catch ( error ) {
                    console.log ( error );
                }
            }
        }

        if ( !assets ) {
            assets = {};
            for ( let typeName in schema.definitions ) {
                schema.addTestAsset ( assets, typeName );
            }
        }

        // TODO: properly handle layout field alternatives; doing this here is a big, fat hack
        let assetsWithLayouts = {};
        for ( let assetID in assets ) {

            const asset = assets [ assetID ];
            const layoutName = _.has ( asset.fields, 'layout' ) ? asset.fields.layout.value : '';
            
            if ( this.layouts [ layoutName ]) {
                assetsWithLayouts [ assetID ] = asset;
            }
        }

        this.refreshBinding ( schema, assetsWithLayouts );
        this.refreshAssetLayouts ();
        this.finishLoading ();
    }

    //----------------------------------------------------------------//
    @computed get
    validMethods () {

        let methods = [];
        const bindingsByName = this.binding.methodBindingsByName;
        for ( let name in bindingsByName ) {
            if ( bindingsByName [ name ].valid ) {
                methods.push ( name );
            }
        }
        return methods;
    }
}
