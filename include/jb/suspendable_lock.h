#ifndef __JB__SUSPENDABLE_LOCK__H__
#define __JB__SUSPENDABLE_LOCK__H__


#include <utility>
#include <assert.h>


namespace jb
{
    namespace details
    {
        /** Implements lock guard over a lockable class with the ability to suspend lock temporary

        @tparam BasicLockable - lockable class, must meet BasicLockable expectations, i.e. must
                                provide lock() and unlock() functions
        */
        template < typename BasicLockable >
        class suspendable_lock
        {
            //
            // From the perfomance perspective it's also expected that BasicLockable::lock() and 
            // BasicLockable::unlock() are non-throwing
            //
            static_assert( noexcept( std::declval< BasicLockable >().lock() ), "BasicLockable::lock() MUST meet noexcept condition" );
            static_assert( noexcept( std::declval< BasicLockable >().unlock() ), "BasicLockable::unlock() MUST meet noexcept condition" );

            BasicLockable* lockable_ = nullptr;
            bool suspended_ = false;

        public:

            /** Default constructor...
            
            ...creates a dummy instance that doesn't lock anything but potentially can be assigned
            with locking one by move assignment

            @throw nothing
            */
            suspendable_lock() noexcept = default;

            /** Move constructor...

            ...creates an instance on the base of another one, keeps a lock if taken, makes source
            instance dummy

            @param [in/out] other - source instance
            @throw nothing
            */
            suspendable_lock( suspendable_lock&& other ) noexcept
            {
                assert( !other.suspended_ );
                //
                std::swap( lockable_, other.lockable_ );
                std::swap( suspended_, other.suspended_ );
            }

            /** Move assignment...

            ...assignes an instance with the base of another one, keeps a lock if taken, makes source
            instance dummy

            @param [in/out] other - source instance
            @throw nothing
            */
            suspendable_lock& operator = ( suspendable_lock&& other ) noexcept
            {
                assert( !suspended_ && !other.suspended_ );
                //
                if ( lockable_ ) lockable_->unlock();
                //
                lockable_ = nullptr;
                suspended_ = false;
                //
                std::swap( lockable_, other.lockable_ );
                std::swap( suspended_, other.suspended_ );
                //
                return *this;
            }

            /** Explicit constructor...
            
            Creates an instance, associated it with given Lockable object, and takes a lock

            @param [in/out] lockable - Locable to be locked
            @throw nothing
            */
            explicit suspendable_lock( BasicLockable& lockable ) noexcept : lockable_( &lockable )
            {
                assert( lockable_ );
                //
                lockable_->lock();
            }

            /** Destroys an instance and release lock over associated Lockable if taken

            @throw nothing
            */
            ~suspendable_lock()
            {
                assert( !suspended_ );
                //
                if ( lockable_ ) lockable_->unlock();
            }

            /** Suspends locking

            @throw nothing
            */
            void suspend() noexcept
            {
                assert( lockable_ && !suspended_ );
                //
                lockable_->unlock();
                suspended_ = true;
            }

            /** Resumes locking

            @throw nothing
            */
            void resume() noexcept
            {
                assert( lockable_ && suspended_ );
                //
                lockable_->lock();
                suspended_ = false;
            }

            /** Provides scoped suspending of suspendable lock

            @warning moving/assignment of suspended lock will cause undefined behaviour
            */
            class scoped_lock_suspend
            {
                suspendable_lock& lock_;

            public:

                /** The class is not default/copy/move-constructible and it also cannot be copied/moved
                */
                scoped_lock_suspend() = delete;
                scoped_lock_suspend( scoped_lock_suspend&& ) = delete;


                /** Explicit constructor...
                
                Suspends given suspendable lock

                @param [in/out] lock - Suspendabe lock to be suspended
                @throw nothing
                */
                explicit scoped_lock_suspend( suspendable_lock& lock ) noexcept : lock_( lock )
                {
                    lock_.suspend();
                }


                /** Destructor...
                
                Resumes associated suspendable lock

                @throw nothing
                */
                ~scoped_lock_suspend()
                {
                    lock_.resume();
                }
            };
        };
    }
}

#endif
