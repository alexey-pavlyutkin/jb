#ifndef __JB__ALIGNED_ATOMIC__H__
#define __JB__ALIGNED_ATOMIC__H__


#include <atomic>
#include <new>


namespace jb
{
    namespace details
    {
        /** Wraps std::atomic<> and forces it to align onto cache line to ensure that there are

        no another variable sharing the same cache line

        @tparam T - type to be hold as atomic
        @tparam A - the holder type, used for testing
        */
        template < typename T, typename A = std::atomic< T > >
        class aligned_atomic
        {
            alignas( std::hardware_destructive_interference_size ) A a_ = 0;

        public:

            aligned_atomic() noexcept = default;
            constexpr aligned_atomic( T desired ) noexcept : a_( T ) {}

            /** Wraps std::atomic< T >::store() method */
            template < typename... Args >
            auto store( Args&&... args ) noexcept { return a_.store( std::forward< Args >( args )... ); }
            template < typename... Args >
            auto store( Args&&... args ) volatile noexcept { return a_.store( std::forward< Args >( args )... ); }

            /** Wraps std::atomic< T >::load() method */
            template < typename... Args >
            auto load( Args&&... args ) const noexcept { return a_.load( std::forward< Args >( args )... ); }
            template < typename... Args >
            auto load( Args&&... args ) const volatile noexcept { return a_.load( std::forward< Args >( args )... ); }

            /** Wraps std::atomic< T >::exchange() method */
            template < typename... Args >
            auto exchange( Args&&... args ) noexcept { return a_.exchange( std::forward< Args >( args )... ); }
            template < typename... Args >
            auto exchange( Args&&... args ) volatile noexcept { return a_.exchange( std::forward< Args >( args )... ); }

            /** Wraps std::atomic< T >::compare_exchange_weak() method */
            template < typename... Args >
            auto compare_exchange_weak( Args&&... args ) noexcept { return a_.compare_exchange_weak ( std::forward< Args >( args )... ); }
            template < typename... Args >
            auto compare_exchange_weak( Args&&... args ) volatile noexcept { return a_.compare_exchange_weak( std::forward< Args >( args )... ); }

            /** Wraps std::atomic< T >::compare_exchange_strong() method */
            template < typename... Args >
            auto compare_exchange_strong( Args&&... args ) noexcept { return a_.compare_exchange_strong( std::forward< Args >( args )... ); }
            template < typename... Args >
            auto compare_exchange_strong( Args&&... args ) volatile noexcept { return a_.compare_exchange_strong( std::forward< Args >( args )... ); }

            /** Wraps std::atomic< T >::fetch_add() method */
            template < typename... Args >
            auto fetch_add( Args&&... args ) noexcept { return a_.fetch_add( std::forward< Args >( args )... ); }
            template < typename... Args >
            auto fetch_add( Args&&... args ) volatile noexcept { return a_.fetch_add( std::forward< Args >( args )... ); }

            /** Wraps std::atomic< T >::fetch_sub() method */
            template < typename... Args >
            auto fetch_sub( Args&&... args ) noexcept { return a_.fetch_sub( std::forward< Args >( args )... ); }
            template < typename... Args >
            auto fetch_sub( Args&&... args ) volatile noexcept { return a_.fetch_sub( std::forward< Args >( args )... ); }

            /** Wraps std::atomic< T >::fetch_and() method */
            template < typename... Args >
            auto fetch_and( Args&&... args ) noexcept { return a_.fetch_and( std::forward< Args >( args )... ); }
            template < typename... Args >
            auto fetch_and( Args&&... args ) volatile noexcept { return a_.fetch_and( std::forward< Args >( args )... ); }

            /** Wraps std::atomic< T >::fetch_or() method */
            template < typename... Args >
            auto fetch_or( Args&&... args ) noexcept { return a_.fetch_or( std::forward< Args >( args )... ); }
            template < typename... Args >
            auto fetch_or( Args&&... args ) volatile noexcept { return a_.fetch_or( std::forward< Args >( args )... ); }

            /** Wraps std::atomic< T >::fetch_xor() method */
            template < typename... Args >
            auto fetch_xor( Args&&... args ) noexcept { return a_.fetch_xor( std::forward< Args >( args )... ); }
            template < typename... Args >
            auto fetch_xor( Args&&... args ) volatile noexcept { return a_.fetch_xor( std::forward< Args >( args )... ); }
        };
    }
}

#endif
