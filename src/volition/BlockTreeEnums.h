// Copyright (c) 2017-2018 Cryptogogue, Inc. All Rights Reserved.
// http://cryptogogue.com

#ifndef VOLITION_BLOCKTREENUMS_H
#define VOLITION_BLOCKTREENUMS_H

namespace Volition {

enum kBlockTreeAppendResult {
    APPEND_OK,
    ALREADY_EXISTS,
    MISSING_PARENT,
    TOO_SOON,
};

enum kBlockTreeEntryMeta {
    META_NONE,
    META_PROVISIONAL,
};

enum kBlockTreeEntryStatus {
    STATUS_NEW          = 0x01,
    STATUS_COMPLETE     = 0x02,
    STATUS_MISSING      = 0x04,
    STATUS_INVALID      = 0x08,
};

} // namespace Volition
#endif
