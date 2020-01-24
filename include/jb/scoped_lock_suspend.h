#ifndef __JB__SCOPED_LOCK_SUSPEND__H__
#define __JB__SCOPED_LOCK_SUSPEND__H__


#include <mutex>
#include <exception>
#include "exception.h"
#include <assert.h>


namespace jb
{
    namespace details
    {
        /** Implements temprorary suspending of unique lock over a mutex object that meets...
        
        ...BasicLockable requirements, i.e. provides lock() and unlock() methods

        @tparam BasicLockable - mutex type
        */
        template < typename BasicLockable >
        class scoped_lock_suspend
        {
            std::unique_lock< BasicLockable >& lock_;

        public:

            /** The class is not default/copy/move-constructible and also it is not copyable/movable
            */
            scoped_lock_suspend() = delete;
            scoped_lock_suspend( scoped_lock_suspend&& ) = delete;

            /** Explicit constructor, temporary suspends taken lock

            @param [in/out] lock - taken unique lock over a mutex
            @throw any exception thrown by BasicLockable::unlock() or std::system_error if an logic
                   error occured (passed lock does not have associated mutex or lock is not taken)
            */
            explicit scoped_lock_suspend( std::unique_lock< BasicLockable >& lock ) try : lock_( lock )
            {
                lock_.unlock();
            }
            catch ( const std::system_error& e )
            {
                auto what = e.what();
                assert( !what );
                throw runtime_error( RetCode::UnknownError, what );
            }

            /** Destructor, resumes suspended lock

            @throw any exception thrown by BasicLockable::lock() or std::system_error if an logic
                   error occured (e.g. lock was moved)
            */
            ~scoped_lock_suspend() noexcept( false ) try
            {
                lock_.lock();
            }
            catch ( const std::system_error & e )
            {
                auto what = e.what();
                assert( !what );
                throw runtime_error( RetCode::UnknownError, what );
            }
        };
    }
}

#endif
