/* eslint-disable no-whitespace-before-property */

import { assert }           from '../util/assert';
import { Binding }          from './Binding';
import { SchemaMethod }     from './SchemaMethod';
import _                    from 'lodash';
import uuidv4               from 'uuid/v4';

//================================================================//
// Schema
//================================================================//
export class Schema {

    //----------------------------------------------------------------//
    addTestAsset ( assets, typeName, assetID ) {

        assetID = assetID || uuidv4 ();

        let asset = this.newAsset ( assetID, typeName );
        assert ( Boolean ( asset ));
        assets [ assetID ] = asset;

        return assetID;
    }

    //----------------------------------------------------------------//
    applyTemplate ( template ) {

        if ( !template ) return;

        const name = template.name;

        if ( this.applied [ name ]) return;
        this.applied [ name ] = true;

        for ( let typeName in template.definitions ) {
            this.definitions [ typeName ] = template.definitions [ typeName ]; // TODO: deep copy
        }

        for ( let methodName in template.methods ) {
            this.methods [ methodName ] = new SchemaMethod ( methodName, template.methods [ methodName ]);
        }
    }

    //----------------------------------------------------------------//
    constructor ( template ) {

        this.applied        = {}; // table of schema names that have already been applied
        this.methods        = {}; // table of all available methods
        this.definitions    = {}; // table of all known asset types

        this.applyTemplate ( template );
    }

    //----------------------------------------------------------------//
    generateBinding ( assets ) {

        let methodBindingsByAssetID     = {};
        let methodBindingsByName        = {};

        // generate all the empty method bindings.
        for ( let methodName in this.methods ) {
            methodBindingsByName [ methodName ] = this.methods [ methodName ].newBinding ();
        }

        // bind each asset and each method...
        for ( let assetID in assets ) {

            methodBindingsByAssetID [ assetID ] = {};

            for ( let methodName in this.methods ) {

                this.methods [ methodName ].bindAsset (
                    this,
                    assets [ assetID ],
                    methodBindingsByName [ methodName ],
                    methodBindingsByAssetID [ assetID ]
                );
            }
        }

        // at this stage, assets are populated and linked to methods.
        // all methods and method params track assets that qualify.
        // now we have to iterate through all the methods and find out if they can be executed.
        for ( let methodName in this.methods ) {

            // create a relationship if the asset qualifies.
            this.methods [ methodName ].validate ( methodBindingsByName [ methodName ]);
        }

        return new Binding ( methodBindingsByName, methodBindingsByAssetID );
    }

    //----------------------------------------------------------------//
    newAsset ( assetID, typeName ) {

        let definition = this.definitions [ typeName ];
        assert ( Boolean ( definition ));

        let asset = {
            type:           typeName,
            assetID:        assetID,
            fields:         {},
            alternates:     {},
        };

        for ( let fieldName in definition.fields ) {
            let field = definition.fields [ fieldName ];
            asset.fields [ fieldName ] = {
                value:          field.value,
                alternates:     _.cloneDeep ( field.alternates ),
            }
        }
        return asset;
    }
}
