// Copyright (c) 2017-2018 Cryptogogue, Inc. All Rights Reserved.
// http://cryptogogue.com

#include <volition/FileSys.h>

namespace Volition {

//================================================================//
// FileSys
//================================================================//
namespace FileSys {

//----------------------------------------------------------------//
void createDirectories ( string path ) {

    Poco::Path basePath ( path );
    basePath.makeAbsolute ();
    path = basePath.toString ();

    Poco::File directory ( path );
    directory.createDirectories ();
}

//----------------------------------------------------------------//
bool exists ( string path ) {
  struct stat buffer;
  return ( stat ( path.c_str(), &buffer ) == 0 );
}

//----------------------------------------------------------------//
void getFilenames ( string path, set < string > files ) {

    DIR* dir = opendir ( path.c_str ());
    assert ( dir );

    struct dirent* entry;
    while (( entry = readdir ( dir )) != NULL ) {
        if ( entry->d_type == DT_DIR ) continue; // NOTE: doesn't work on every OS
        files.insert ( entry->d_name );
    }
    closedir ( dir );
}

//----------------------------------------------------------------//
size_t getDirSize ( string path ) {

    DIR* dir = opendir ( path.c_str ());
    assert ( dir );

    size_t count = 0;
    struct dirent* entry;
    while (( entry = readdir ( dir )) != NULL ) {
        ++count;
    }
    closedir ( dir );
    return count;
}

} // namespace Format
} // namespace Volition
