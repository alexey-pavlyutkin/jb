#ifndef __JB__RARE_EXCLUSIVE_FREQUENT_SHARED_MUTEX__H__
#define __JB__RARE_EXCLUSIVE_FREQUENT_SHARED_MUTEX__H__


#include <atomic>
#include <array>
#include <functional>
#include <new>
#include <thread>
#include <assert.h>


namespace jb
{
    namespace details
    {
        /** Implements SharedMutex concept optimized for rare EXCLUSIVE and frequent SHARED locks

        The root idea is to split shared locks between multiple atomics, each laying in different cache line,
        thus threads trying to take shared lock won't collide on the same cache line. That reduces probability
        of cache misses by invalidation and optimizes system performance.
        
        Imagine that thread A tries to get shared lock on cache line #1 from CPU1, and thread B gets lock on cache
        line #2 from CPU2. When thread A writes cache line #1 with release semantic, CPU1 notifies CPU2 that
        #1 was modified. Later when thread B on CPU2 applies a read operation with acquire semantic, CPU2 invalidates
        cache line #1 in cache, but it does not cause cache miss on CPU2, just because thread B operates on cache
        line #2 and does not access cache line #1, i.e. potentionally the most heavy store/load memory barrier becomes
        flyweight and does not have impact system performance

        The cost is extremely heavy exclusive lock, cuz it requires to exam all shared lock atomics

        @tparam SharedLockCount - number of atomics to represent shared lock
        */
        template < size_t SharedLockCount = 31 >
        class rare_exclusive_frequent_shared_mutex
        {
            static_assert( SharedLockCount );

            alignas( std::hardware_destructive_interference_size ) std::atomic_bool exclusive_lock_;
            std::array< alignas( std::hardware_destructive_interference_size ) std::atomic_size_t, SharedLockCount > shared_locks_;

        public:

            /** Default constructor, initializes an instance to unlocked state

            @throw nothing
            */
            rare_exclusive_frequent_shared_mutex() noexcept = default;


            /** Explicitly deleted moving constructor, makes the class to be non-copyable/movable
            */
            rare_exclusive_frequent_shared_mutex( rare_exclusive_frequent_shared_mutex&& ) = delete;


            /** Tries to take EXCLUSIVE lock over the mutex, returns immediately

            @retval true if the operation succeeded
            @throw nothing
            */
            bool try_lock() noexcept
            {
                // try to signal exclusive lock
                if ( exclusive_lock_.exchange_( true, std::memory_order_acq_rel ) )
                {
                    // if exclusive lock is already signalled - fail
                    return false;
                }
                // exclusive lock signalled!

                // through all the shared locks
                for ( auto& shared_lock : shared_locks_ )
                {
                    // if shared lock signalled
                    if ( shared_lock.load( std::memory_order_acquire ) )
                    {
                        // release exclusive lock...
                        exclusive_lock_.store( 0, std::memory_order_release );

                        // ...and fail
                        return false;
                    }
                }

                // succeeded
                return true;
            }


            /** Takes EXCLUSIVE lock over the mutex

            @param [in] spin_count - number of tries before to yeild other threads
            @throw nothing
            */
            void lock( size_t spin_count = 0x1000 ) noexcept
            {
                // try to signal exclusive lock
                for ( size_t spin = 1; ; ++spin )
                {
                    if ( !exclusive_lock_.exchange_( true, std::memory_order_acq_rel ) )
                    {
                        break;
                    }

                    // yeild other threads if spins exhausted 
                    if ( 0 == spin % spin_count )
                    {
                        std::this_thread::yield();
                    }
                }
                // exclusive lock signalled!

                // mark all shared locks as taken
                std::array< bool, SharedLockCount > shared_released;
                shared_released.fill( false );

                // spin counter
                size_t spin = 1;

                while ( true )
                {
                    // throught all shared locks
                    for ( auto sl_it = shared_locks_.begin(), sr_it = shared_released.begin(); sl_it != shared_locks_.end(); ++sl_it, ++sr_it )
                    {
                        // if we've already seen that shared lock released -- simply skip it, it could be taken later cuz we had signalled exclusive lock
                        if ( *sr_it ) continue;

                        // check if shared lock got released
                        if ( sl_it->load( std::memory_order_acquire ) )
                        {
                            // the lock still taken -> yeild other threads if spins exhausted 
                            if ( 0 == ++spin % spin_count )
                            {
                                std::this_thread::yield();
                            }
                        }
                        else
                        {
                            // mark lock as released
                            *sr_it = true;
                        }
                    }

                    // if there is not taken shared locks anymore
                    if ( !std::find( shared_released.begin(), shared_released.end(), false ) )
                    {
                        // succeeded
                        break;
                    }
                }
            }


            /** Releases EXCLUSIVE lock over the mutex

            @throw nothing
            */
            void unlock() noexcept
            {
                // release exclusive lock
                exclusive_lock_.store( false, std::memory_order_release );
            }


            /** Tries to get SHARED lock over the mutex, returns immediately

            @param [in] locker_id - an identifier of the object requesting shared lock (uniqueness NOT required)
            @retval true if succeeded
            @throw nothing
            */
            bool try_lock_shared( size_t locker_id ) noexcept
            {
                // hash shared lock by locker id
                auto& shared_lock = shared_locks_[ locker_id % SharedLockCount ];

                // acquire shared lock
                shared_lock.fetch_add( 1, std::memory_order_acq_rel );

                // if exclusive lock taken
                if ( exclusive_lock_.load( std::memory_order_acquire ) )
                {
                    // release shared lock
                    shared_lock.fetch_sub( 1, std::memory_order_acq_rel );

                    // failed
                    return false;
                }

                // succeeded
                return true;
            }


            /** Takes shared lock over the mutex

            @param [in] locker_id - an identifier of the object requesting shared lock (uniqueness NOT required)
            @param [in] spin_count - number of tries before to yeild other threads
            @throw nothing
            */
            void lock_shared( size_t locker_id, size_t spin_count = 0x1000 ) noexcept
            {
                // hash shared lock by locker id
                auto& shared_lock = shared_locks_[ locker_id % SharedLockCount ];

                // acquire shared lock
                shared_lock.fetch_add( 1, std::memory_order_acq_rel );

                // spin while exclusive lock taken
                for ( size_t spin = 1; exclusive_lock_.load( std::memory_order_acquire ); ++spin )
                {
                    // if spin count exhausted
                    if ( 0 == spin % spin_count )
                    {
                        // suspend shared lock
                        shared_lock.fetch_sub( 1, std::memory_order_acq_rel );

                        // yield other threads
                        std::this_thread::yield();

                        // acquire shared lock again
                        shared_lock.fetch_add( 1, std::memory_order_acq_rel );
                    }
                }
            }


            /** Releases taken shared lock

            @param [in] locker_id - an identifier of the object releasing shared lock (uniqueness NOT required)
            @throw nothing
            */
            void unlock_shared( size_t locker_id ) noexcept
            {
                // simply clear shared lock
                shared_locks_[ locker_id % SharedLockCount ].fetch_sub( 1, std::memory_order_acq_rel ) ;
            }


            /** General-purpose EXCLUSIVE mutex ownership wrapper, provides life-time EXCLUSIVE lock over associated mutex
            */
            class unique_lock
            {
                rare_exclusive_frequent_shared_mutex* mtx_ = nullptr;
                size_t spin_count_ = 0;
                bool taken_ = false;

            public:

                /** Default constructor, creates dummy instance without associated mutex

                @throw nothing
                */
                unique_lock() noexcept = default;


                /** Move constructor, creates an instance from another one, make origin instance dummy

                @param [in/out] other - an instance to move from
                @throw nothing
                */
                unique_lock( unique_lock&& other ) noexcept
                {
                    swap( other );
                }


                /** Creates new instance, associates it with given mutex, and takes EXCLUSIVE lock on it

                @param [in/out] mtx - mutex to be locked
                @param [in] spin_count - number of lock tries before to yeild other threads
                @throw nothing
                */
                explicit unique_lock( rare_exclusive_frequent_shared_mutex& mtx, size_t spin_count = 0x1000 ) noexcept
                    : mtx_( &mtx )
                    , spin_count_( spin_count )
                    , taken( true )
                {
                    assert( mtx_ && spin_count_ );
                    mtx_->lock( spin_count_ );
                }

                /** Destructor, releases EXCLUSIVE lock on associated mutex if taken
                */
                ~unique_lock()
                {
                    if ( mtx_ && taken_ ) mtx_->unlock();
                }

                /** Assignes an instance by moving content from given one.
                
                Makes original instance becomes dummy. If target instance holds lock over an mutex,
                releases the lock

                @retval target instance as lvalue
                @throw nothing
                */
                unique_lock& operator = ( unique_lock&& other ) noexcept
                {
                    unique_lock dummy;
                    swap( dummy );
                    swap( other );
                    //
                    return *this;
                }


                /* Swap an instance with another one competely swaping their content

                @throw nothing
                */
                void swap( unique_lock& other ) noexcept
                {
                    std::swap( mtx_, other.mtx_ );
                    std::swap( spin_count_, other.spin_count_ );
                    std::swap( taken_, other.taken_ );
                }


                /** Releases hold lock over associated mutex

                @throw nothing
                */
                void unlock() noexcept
                {
                    assert( mtx_ && taken_ );
                    mtx_->unlock();
                    taken_ = false;
                }


                /** Re-takes released lock over associated mutex

                @throw nothing
                */
                void lock() noexcept
                {
                    assert( mtx_ && !taken_ );
                    mtx_->lock( spin_count_ );
                    taken_ = true;
                }
            };


            /** General-purpose EXCLUSIVE mutex ownership wrapper, provides life-time EXCLUSIVE lock over associated mutex
            */
            class shared_lock
            {
                rare_exclusive_frequent_shared_mutex* mtx_ = nullptr;
                size_t id_ = 0;
                size_t spin_count_ = 0;
                bool taken_ = false;

            public:

                /** Default constructor, creates dummy instance without associated mutex

                @throw nothing
                */
                shared_lock() noexcept = default;


                /** Move constructor, creates an instance from another one, make origin instance dummy

                @param [in/out] other - an instance to move from
                @throw nothing
                */
                shared_lock( shared_lock&& other ) noexcept
                {
                    swap( other );
                }


                /** Creates new instance, associates it with given mutex, and takes SHARED lock on it

                @param [in/out] mtx - mutex to be locked
                @param [in] locker - id of the object requesting lock
                @param [in] spin_count - number of lock tries before to yeild other threads
                @throw nothing
                */
                template < typename Locker >
                explicit shared_lock( rare_exclusive_frequent_shared_mutex& mtx, const Locker& locker, size_t spin_count = 0x1000 ) noexcept
                    : mtx_( &mtx )
                    , id_( std::hash< Locker >{}( locker ) )
                    , spin_count_( spin_count )
                    , taken_( true )
                {
                    assert( mtx_ && spin_count_ );
                    mtx_->lock_shared( id_, spin_count_ );
                }

                ~shared_lock()
                {
                    if ( mtx_ && taken_ ) mtx_->unlock_shared( id_ );
                }

                shared_lock& operator = ( shared_lock&& other ) noexcept
                {
                    shared_lock dummy;
                    swap( dummy );
                    swap( other );
                    //
                    return *this;
                }

                void swap( shared_lock& other ) noexcept
                {
                    std::swap( mtx_, other.mtx_ );
                    std::swap( id_, other.id_ );
                    std::swap( spin_count_, other.spin_count_ );
                    std::swap( taken_, other.taken_ );
                }

                /** Releases hold lock over associated mutex

                @throw nothing
                */
                void unlock() noexcept
                {
                    assert( mtx_ && taken_ );
                    mtx_->unlock_shared( id_ );
                    taken_ = false;
                }


                void lock() noexcept
                {
                    assert( mtx_ && !taken_ );
                    mtx_->lock_shared( id_, spin_count_ );
                    taken_ = true;
                }
            };
        };
    }
}

#endif