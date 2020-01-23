#ifndef __JB__RET_CODES___H__
#define __JB__RET_CODES___H__

namespace jb
{
    enum class RetCode
    {
        Ok = 0,
        UnknownError,
        InsufficientMemory,
        InvalidHandle,
        InvalidFilePath,
        IoError
    };
}

#endif