/* eslint-disable no-whitespace-before-property */

const assert        = require ( 'assert' );
const Schema        = require ( './schema' ).Schema;

//================================================================//
// Binding
//================================================================//
class Binding {

    //----------------------------------------------------------------//
    constructor ( methodBindingsByName, methodBindingsByAssetID ) {

        this.methodBindingsByName       = methodBindingsByName || {};
        this.methodBindingsByAssetID    = methodBindingsByAssetID || {};
    }

    //----------------------------------------------------------------//
    getMethodParamBindings ( methodName ) {

        // we'll need the method template (from the schema) *and* the binding
        const methodBinding = this.methodBindingsByName [ methodName ];

        // form fields, by name
        let paramBindings = {};

        // for each asset field, set the type and the list of qualified assets
        for ( let argname in methodBinding.assetIDsByArgName ) {

            let options = [];
            let assetIDsForArg = methodBinding.assetIDsByArgName [ argname ];
            for ( let i in assetIDsForArg ) {
                let assetID = assetIDsForArg [ i ];
                options.push ( assetID );
            }
            paramBindings [ argname ] = options;
        }

        return paramBindings;
    }

    //----------------------------------------------------------------//
    methodIsValid ( methodName, assetID ) {

        if ( assetID ) {
            let methodBinding = this.methodBindingsByAssetID [ assetID ][ methodName ];
            return methodBinding ? methodBinding.valid : false;
        }
        return this.methodBindingsByName [ methodName ].valid;
    }
}

var exports = module.exports = {
    Binding:    Binding,
}
