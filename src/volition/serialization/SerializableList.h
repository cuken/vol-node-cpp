// Copyright (c) 2017-2018 Cryptogogue, Inc. All Rights Reserved.
// http://cryptogogue.com

#ifndef VOLITION_SERIALIZATION_SERIALIZABLELIST_H
#define VOLITION_SERIALIZATION_SERIALIZABLELIST_H

#include <volition/serialization/AbstractSerializerFrom.h>
#include <volition/serialization/AbstractSerializerTo.h>

namespace Volition {

//================================================================//
// SerializableSet
//================================================================//
template < typename TYPE >
class SerializableList :
    public AbstractSerializableCollection,
    public list < TYPE > {
public:
    
    //----------------------------------------------------------------//
    void AbstractSerializable_serializeFrom ( const AbstractSerializerFrom& serializer ) {
        
        size_t size = serializer.getSize ();
        for ( size_t i = 0; i < size; ++i ) {
            TYPE value;
            serializer.serialize ( i, value );
            this->push_back ( value );
        }
    }
    
    //----------------------------------------------------------------//
    void AbstractSerializable_serializeTo ( AbstractSerializerTo& serializer ) const {
        
        if ( serializer.isDigest ()) {
            // TODO: sort set elements and hash
        }
        else {
            typename list < TYPE >::const_iterator it = this->begin ();
            for ( size_t i = 0; it != this->end (); ++it, ++i ) {
                serializer.serialize ( i, *it );
            }
        }
    }
    
    //----------------------------------------------------------------//
    void AbstractSerializableCollection_resize ( size_t size ) {
    }
};

} // namespace Volition
#endif