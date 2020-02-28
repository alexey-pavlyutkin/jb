#ifndef __JB__RARE_EXCLUSIVE_FREQUENT_SHARED_MUTEX__H__
#define __JB__RARE_EXCLUSIVE_FREQUENT_SHARED_MUTEX__H__


#include "aligned_atomic.h"
#include <array>
#include <functional>
#include <thread>
#include <assert.h>


namespace jb
{
    namespace details
    {
        /** Implements SharedMutex concept optimized for rare EXCLUSIVE and frequent SHARED locks

        The root idea is to split shared locks between multiple atomics, each laying in different cache line,
        thus threads trying to take shared lock won't collide on the same cache line. That reduces probability
        of invalidation cache misses and optimizes the system performance.
        
        Imagine that thread A is trying to get shared lock on cache line #1 from CPU1, and thread B is getting lock on
        cache line #2 from CPU2. When thread A writes cache line #1 with release semantic, CPU1 notifies CPU2 that #1
        was modified. Later when thread B on CPU2 applies a read operation with acquire semantic, CPU2 drops cache
        line #1 from the cache, but it does not cause a cache miss on CPU2 because thread B operates on untouched
        cache line #2 and does not access dropped cache line #1, i.e. potentionally the most heavy store/load memory
        barrier becomes really flyweight and does not impact system performance. Also an atomic that represents 
        exclusive lock is accessed only by reading, i.e. there is not a need to synchronize corresponding cache line
        between CPU's at all.

        The cost of this optimization is extremely heavy exclusive lock, cuz it requires to exam all shared lock atomics

        @tparam SharedLockCount - number of atomics to represent shared lock
        */
        template < size_t SharedLockCount = 31 >
        class rare_exclusive_frequent_shared_mutex
        {
            static_assert( SharedLockCount );

            aligned_atomic< bool > exclusive_lock_;
            std::array< aligned_atomic< size_t >, SharedLockCount > shared_locks_;
            static constexpr size_t spin_count_per_lock = 0x1000;

        public:

            static constexpr size_t shared_lock_count() noexcept { return SharedLockCount; }

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
            bool try_lock( size_t spin_count = 0 ) noexcept
            {
                size_t spin = 0;
                spin_count = spin_count ? spin_count : spin_count_per_lock * SharedLockCount;

                // try to signal exclusive lock
                while ( true )
                {
                    if ( !exclusive_lock_.exchange( true, std::memory_order_acq_rel ) ) break;

                    // if spins exhausted - fail
                    if ( 0 == ++spin % spin_count ) return false;
                }
                // exclusive lock signalled!

                // mark all shared locks as taken
                std::array< bool, SharedLockCount > shared_released;
                shared_released.fill( false );

                // wait until all shared locks get released
                while ( true )
                {
                    // throught all shared locks
                    auto sl_it = shared_locks_.begin();
                    auto sr_it = shared_released.begin();
                    for ( ; sl_it != shared_locks_.end(); ++sl_it, ++sr_it )
                    {
                        //
                        // if we've already seen that shared lock released -- simply skip it, 
                        // it could not have been taken again cuz we had signalled exclusive lock
                        //
                        if ( *sr_it ) continue;

                        // check if shared lock got released
                        if ( 0 != sl_it->load( std::memory_order_acquire ) )
                        {
                            // the lock still taken -> yeild other threads if spins exhausted 
                            if ( 0 == ++spin % spin_count )
                            {
                                exclusive_lock_.store( false, std::memory_order_release );
                                return false;
                            }
                        }
                        else
                        {
                            // mark lock as released
                            *sr_it = true;
                        }
                    }

                    //
                    // if there is not taken shared locks anymore - succeeded, the last sync operation on the
                    // control flow was sl_it->load( std::memory_order_acquire ), so the whole function guaratnies
                    // ACQUIRE semantic
                    //
                    if ( std::find( shared_released.begin(), shared_released.end(), false ) == shared_released.end() ) return true;
                }
            }


            /** Takes EXCLUSIVE lock over the mutex

            @param [in] spin_count - number of tries before to yeild other threads
            @throw nothing
            */
            void lock( size_t spin_count = 0 ) noexcept
            {
                spin_count = spin_count ? spin_count : spin_count_per_lock * SharedLockCount;
                while ( !try_lock( spin_count ) ) std::this_thread::yield();
            }


            /** Releases EXCLUSIVE lock over the mutex

            @throw nothing
            */
            void unlock() noexcept
            {
                // release exclusive lock using RELEASE semantic
                exclusive_lock_.store( false, std::memory_order_release );
            }


            /** Tries to get SHARED lock over the mutex, returns immediately

            @param [in] locker_id - an identifier of the object requesting shared lock (uniqueness NOT required)
            @retval true if succeeded
            @throw nothing
            */
            bool try_lock_shared( size_t locker_id, size_t spin_count = 0 ) noexcept
            {
                size_t spin = 0;
                spin_count = spin_count ? spin_count : spin_count_per_lock;

                // hash shared lock by locker id
                auto& shared_lock = shared_locks_[ locker_id % SharedLockCount ];

                // acquire shared lock
                shared_lock.fetch_add( 1, std::memory_order_acq_rel );

                // spin while exclusive lock taken
                for ( size_t spin = 1; exclusive_lock_.load( std::memory_order_acquire ); ++spin )
                {
                    // if spin count exhausted
                    if ( 0 == ++spin % spin_count )
                    {
                        // release shared lock and fail
                        shared_lock.fetch_sub( 1, std::memory_order_acq_rel );
                        return false;
                    }
                }

                //
                // succeeded, the last sync operation on the control flow was exclusive_lock_.load( std::memory_order_acquire ),
                // therefore the function guaranties ACQUIRE semantic
                return true;
            }


            /** Takes SHARED lock over the mutex

            @param [in] locker_id - an identifier of the object requesting shared lock (uniqueness NOT required)
            @param [in] spin_count - number of tries before to yeild other threads
            @throw nothing
            */
            void lock_shared( size_t locker_id, size_t spin_count = 0 ) noexcept
            {
                spin_count = spin_count ? spin_count : spin_count_per_lock;
                while ( !try_lock_shared( locker_id, spin_count ) ) std::this_thread::yield();
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
                explicit unique_lock( rare_exclusive_frequent_shared_mutex& mtx, size_t spin_count = 0 ) noexcept
                    : mtx_( &mtx )
                    , spin_count_( spin_count )
                    , taken_( true )
                {
                    assert( mtx_ );
                    mtx_->lock( spin_count_ );
                }

                /** Destructor, releases EXCLUSIVE lock on associated mutex if taken
                */
                ~unique_lock()
                {
                    if ( mtx_ && taken_ ) mtx_->unlock();
                }

                /** Assignes an instance by moving content from given one

                Makes original instance becomes dummy. If target instance holds lock over an mutex,
                releases the lock

                @param [in/out] other - an instance to be moved from
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


                /* Swap an instance with another one competely exchanging their content

                @param [in/out] other - an instance to be swapped with
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


            /** General-purpose SHARED mutex ownership wrapper, provides life-time EXCLUSIVE lock over associated mutex
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
                explicit shared_lock( rare_exclusive_frequent_shared_mutex& mtx, const Locker& locker, size_t spin_count = 0 ) noexcept
                    : mtx_( &mtx )
                    , id_( std::hash< Locker >{}( locker ) )
                    , spin_count_( spin_count )
                    , taken_( true )
                {
                    assert( mtx_ );
                    mtx_->lock_shared( id_, spin_count_ );
                }


                /** Destructor, releases SHARED lock on associated mutex if taken
                */
                ~shared_lock()
                {
                    if ( mtx_ && taken_ ) mtx_->unlock_shared( id_ );
                }


                /** Assignes an instance by moving content from given one

                Makes original instance becomes dummy. If target instance holds lock over an mutex,
                releases the lock

                @param [in/out] other - an instance to be moved from
                @retval target instance as lvalue
                @throw nothing
                */
                shared_lock& operator = ( shared_lock&& other ) noexcept
                {
                    shared_lock dummy;
                    swap( dummy );
                    swap( other );
                    //
                    return *this;
                }


                /* Swap an instance with another one competely exchanging their content

                @param [in/out] other - an instance to be swapped with
                @throw nothing
                */
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


                /** Re-takes lock over associated mutex

                @throw nothing
                */
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
