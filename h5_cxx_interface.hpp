/** @file h5_cxx_interface.hpp
    A quick attempt to use RAII idiom to control HDF5 resources
*/
#pragma once

#include <memory>
#include <exception>

#include <hdf5.h>

namespace h5 {

    namespace detail {

        template <typename H5T, herr_t (*CLOSER_F)(H5T)>
        class wrapper_helper {
            typedef H5T h5_id_type;

            typedef void (*deleter_type)(const h5_id_type*);
            typedef std::unique_ptr<H5T,deleter_type> keeper_type;

            static void close_(h5_id_type id) {
                if (id<0) return;
                auto err=CLOSER_F(id);
                // std::cerr << "DEBUG: resource with id=" << id << " destroyed, err=" << err << "\n";
                if (err<0) {
                    throw std::runtime_error("Error freeing an HDF5 resource");
                }
            }

            static void call_closer(const h5_id_type* id_ptr) noexcept {
                close_(*id_ptr); // on exception, calls std::terminate() right away
            }

            h5_id_type id_;
            keeper_type keeper_;


            public:
            wrapper_helper(h5_id_type id): id_(id), keeper_(&id_, &call_closer)
            {
                if (id<0) {
                    throw std::runtime_error("Error allocating an HDF5 resource");
                }
                // std::cerr << "DEBUG: resource with id=" << id_ << " created\n";
            }

            operator h5_id_type() const {
                // std::cerr << "DEBUG: resource with id=" << id_ << " accessed\n";
                if (id_<0) {
                    throw std::runtime_error("Attempt to re-use deallocated HDF5 resource");
                }
                return id_;
            }

            void close() {
                auto old_id=id_;
                id_=-1; // the resource is marked "closed" even if closing fails (FIXME?)
                close_(old_id);
            }
        };
    }
    using fd_wrapper=detail::wrapper_helper<hid_t, H5Fclose>;
    using dspace_wrapper=detail::wrapper_helper<hid_t, H5Sclose>;
    using dset_wrapper=detail::wrapper_helper<hid_t, H5Dclose>;
    using plist_wrapper=detail::wrapper_helper<hid_t, H5Pclose>;
}
