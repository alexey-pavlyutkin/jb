#ifndef __JB__MOUNT_POINT__H__
#define __JB__MOUNT_POINT__H__

#include <memory>
#include <assert.h>

namespace jb
{
    namespace regression
    {
        struct mount_point;
    }

    namespace details
    {
        template < typename Policies >
        class physical_volume;

        template < typename Policies >
        class virtual_volume;

        template < typename Policies >
        class mount_point
        {
            using physical_volume_t = physical_volume< Policies >;
            using physical_volume_ptr = std::shared_ptr< physical_volume_t >;
            using key_t = std::basic_string< typename Policies::key_char_t, typename Policies::key_traits_t >;
            using mount_point_ptr = std::shared_ptr< mount_point >;

            physical_volume_ptr physical_volume_;
            key_t physical_path_;
            mount_point_ptr parent_;
            key_t logical_path_;

            friend struct regression::mount_point;
            friend class virtual_volume< Policies >;

            explicit mount_point(
                const physical_volume_ptr& physical_volume, 
                key_t&& physical_path, 
                const mount_point_ptr& parent,
                key_t&& logical_path
            ) noexcept
                : physical_volume_( physical_volume )
                , physical_path_( std::move( physical_path ) )
                , parent_( parent )
                , logical_path_( std::move( logical_path ) )
            {
                assert( physical_volume_ );
            }

        public:

            mount_point() = delete;
            mount_point( mount_point&& ) = delete;

            const physical_volume_ptr& physical_volume() const noexcept { return physical_volume_; }
            const key_t& physical_path() const noexcept { return physical_path_; }
            const mount_point_ptr& parent() const noexcept { return parent_; }
            const key_t& logical_path() const noexcept { return logical_path_; }
            const int priority() const noexcept { return physical_volume_->priority(); }
        };
    }
}


#endif