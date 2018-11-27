/* eslint-disable no-whitespace-before-property */

const fs        = require ( 'fs' );

// const CONTENT_AUDIO     = 'audio';
// const CONTENT_IMAGE     = 'image';
// const CONTENT_TEXT      = 'text';
// const CONTENT_VIDEO     = 'video';

//----------------------------------------------------------------//
function makeBinaryOp ( opname ) {
    return ( left, right ) => {
        return {
            op:         opname,
            left:       left,
            right:      right,
        };
    };
}

//----------------------------------------------------------------//
function makeUnaryOp ( opname ) {
    return ( param ) => {
        return {
            op:         opname,
            param:      param,
        };
    };
}

//----------------------------------------------------------------//
const AND               = makeBinaryOp  ( 'AND' );
const CONST             = makeUnaryOp   ( 'CONST' );
const EQUAL             = makeBinaryOp  ( 'EQUAL' );
const FIELD             = makeUnaryOp   ( 'FIELD' );
const GREATER_THAN      = makeBinaryOp  ( 'GREATER_THAN' );
const GREATER_OR_EQUAL  = makeBinaryOp  ( 'GREATER_OR_EQUAL' );
const IN                = makeBinaryOp  ( 'IN' );
const IS                = makeUnaryOp   ( 'IS' );
const LESS_OR_EQUAL     = makeBinaryOp  ( 'LESS_OR_EQUAL' );
const LESS_THAN         = makeBinaryOp  ( 'LESS_THAN' );
const NOT               = makeUnaryOp   ( 'NOT' );
const NOT_EQUAL         = makeBinaryOp  ( 'NOT_EQUAL' );
const OR                = makeBinaryOp  ( 'OR' );

//----------------------------------------------------------------//
function jsonEscape ( str ) {
    return str
        .replace ( /\\n/g, "\\n" )
        .replace ( /\\'/g, "\\'" )
        .replace ( /\\"/g, '\\"' )
        .replace ( /\\&/g, "\\&" )
        .replace ( /\\r/g, "\\r" )
        .replace ( /\\t/g, "\\t" )
        .replace ( /\\b/g, "\\b" )
        .replace ( /\\f/g, "\\f" );
};

//----------------------------------------------------------------//
function makeSchemaTransaction ( schema ) {

    let lua = fs.readFileSync ( schema.lua, 'utf8' );
    //lua = jsonEscape ( lua );
    schema.lua = lua;

    return {
        type:   'PUBLISH_SCHEMA',
        name:   schema.name,
        json:   JSON.stringify ( schema ),
    }
}

//================================================================//
// schema
//================================================================//
let schema = {

    name: 'TEST_SCHEMA',

    lua: 'schema.lua',

    classes: {

        pack: {
            displayName:    'Booster Pack',
        },
        
        common: {
            displayName:    'Common',
            keywords:       [ 'card', 'common' ],
        },

        rare: {
            displayName:    'Rare',
            keywords:       [ 'card', 'rare' ],
        },

        ultraRare: {
            displayName:    'Ultra-Rare',
            keywords:       [ 'card', 'ulraRare' ],
        },
    },

    rules: {
 
        openPack: {
            description:    'Open a booster pack.',
            qualifiers:     IS ( 'pack' ),
        },

        makeRare: {
            description:    'Combine two commons to make a rare.',
            qualifiers:     [
                IS ( 'common' ),
                IS ( 'common' ),
            ],
        },

        makeUltraRare: {
            description:    'Combine two rares to make an ultra-rare.',
            qualifiers:     [
                IS ( 'rare' ),
                IS ( 'rare' ),
            ],
        },
    },
}

transaction = makeSchemaTransaction ( schema );

fs.writeFileSync ( 'schema.json', JSON.stringify ( transaction, null, 4 ), 'utf8' );