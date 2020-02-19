/** @file sveral_proc_blocks.cpp Writes data to the HDF5 file by repeated blocks with gaps in-between

 */

#include <vector>
#include <string>
#include <iostream>

#include <mpiwrap/mpiwrap.hpp>
#include <cmdline/cmdline.hpp>

#include "h5_cxx_interface.hpp"

namespace po=program_options;
namespace mpi=mpiwrap;

struct my_params {
    std::string file_name;
    std::string data_name;
    std::size_t block_size;
    std::ptrdiff_t gap_size;
    std::size_t repeat_factor;
    bool do_collective;
};

namespace mpiwrap {
    void bcast(const communicator& comm, const my_params& par, int root)
    {
        if (comm.rank()!=root) {
            throw std::runtime_error("Cannot bcast a const from non-root");
        }
        bcast(comm, par.file_name, root);
        bcast(comm, par.data_name, root);
        bcast(comm, par.block_size, root);
        bcast(comm, par.gap_size, root);
        bcast(comm, par.repeat_factor, root);
        bcast(comm, par.do_collective, root);
    }

    void bcast(const communicator& comm, my_params& par, int root)
    {
        bcast(comm, par.file_name, root);
        bcast(comm, par.data_name, root);
        bcast(comm, par.block_size, root);
        bcast(comm, par.gap_size, root);
        bcast(comm, par.repeat_factor, root);
        bcast(comm, par.do_collective, root);
    }

}


po::optional<my_params> parse_and_bcast(int argc, const char* const* argv,
                                        const mpi::communicator& comm)
{
    const po::optional<my_params> empty;
    const int master=0;
    if (comm.rank()==master) {
        auto par = po::parse(argc, argv);
        if (!par) {
            std::cerr << "Usage: " << argv[0]
                      << " file=<file_name> blocksize=<values_per_block> [gap=<gap_size_in_values>] [repeat=<block_repeat_factor>] [name=<dataset_name>] collective=<yes|no>"
                      << std::endl;
            return empty;
        }

        auto maybe_collective = par->get<bool>("collective");
        if (!maybe_collective) {
            std::cerr << "collective parameter is missing or invalid\n";
            return empty;
        }

        auto maybe_file = par->get<std::string>("file");
        if (!maybe_file) {
            std::cerr << "file parameter is missing or invalid\n";
            return empty;
        }

        auto maybe_bsize = par->get<std::size_t>("blocksize");
        if (!maybe_bsize) {
            std::cerr << "blocksize parameter is missing or invalid\n";
            return empty;
        }

        auto maybe_gap = par->get_or<std::ptrdiff_t>("gap", 0);
        if (!maybe_gap) {
            std::cerr << "gap parameter is missing or invalid\n";
            return empty;
        }
        
        auto maybe_repeat = par->get_or<std::size_t>("repeat", 1);
        if (!maybe_repeat) {
            std::cerr << "repeat parameter is missing or invalid\n";
            return empty;
        }
        
        auto maybe_name = par->get_or("name", "double_set");
        if (!maybe_name) {
            std::cerr << "name parameter is missing or invalid\n";
            return empty;
        }

        if (*maybe_bsize<=0 || *maybe_repeat<1 || (*maybe_bsize + *maybe_gap)<0) {
            std::cerr << "Incorrect values of parameters";
            return empty;
        }

        const my_params my_par = {
            *maybe_file,
            *maybe_name,
            *maybe_bsize,
            *maybe_gap,
            *maybe_repeat,
            *maybe_collective
        };

        
        mpi::bcast(comm, my_par, master);
        return po::make_optional(my_par);
    }

    my_params my_par;
    mpi::bcast(comm, my_par, master);
    return po::make_optional(my_par);
}




int main (int argc, char **argv)
{
    MPI_Info info  = MPI_INFO_NULL;

    using std::string;
    using std::size_t;
    using std::ptrdiff_t;
    using std::cerr;
    using std::cout;
    using std::endl;
    typedef std::vector<double> dvec_t;

    mpi::environment env(argc, argv);
    mpi::communicator comm;
    const int master=0;
    bool is_master = comm.rank()==master;


    const auto maybe_par = parse_and_bcast(argc, argv, comm);
    if (!maybe_par) env.abort(3);
    const auto& par = *maybe_par;


    // DEBUG:
    for (int r=0; r<comm.size(); ++r) {
        if (comm.rank()==r) {
            cout << std::boolalpha
                 << "Rank " << r << " is running with"
                 << " file=" << par.file_name
                 << " blocksize=" << par.block_size
                 << " gap=" << par.gap_size
                 << " repeat=" << par.repeat_factor
                 << " name=" << par.data_name
                 << " collective=" << par.do_collective
                 << std::endl;
        }
        comm.barrier();
    }
    

    
    const hsize_t data_sz=
        (par.block_size+par.gap_size)*par.repeat_factor*comm.size()
        - (par.gap_size>0?par.gap_size:0);


    // Set up file access property list with parallel I/O access
    auto plist_id = h5::plist_wrapper(H5Pcreate(H5P_FILE_ACCESS));
    H5Pset_fapl_mpio(plist_id, comm, MPI_INFO_NULL);

    // Create a new file collectively and release property list identifier.
    auto file_id = h5::fd_wrapper(H5Fcreate(par.file_name.c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, plist_id));
    plist_id.close();


    // Create the dataspace for the dataset.
    std::array<hsize_t,1> dims={data_sz};
    auto filespace = h5::dspace_wrapper(H5Screate_simple(dims.size(), dims.data(), nullptr));

    // Create the dataset with default properties
    auto dset_id = h5::dset_wrapper(H5Dcreate(file_id, par.data_name.c_str(),
                                              H5T_NATIVE_DOUBLE, filespace,
                                              H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT));

    // Each process defines dataset in memory (to write it to the hyperslab later)
    std::array<hsize_t,1> mem_sz={par.block_size*par.repeat_factor};
    auto memspace = h5::dspace_wrapper(H5Screate_simple(mem_sz.size(), mem_sz.data(), nullptr));

    // Initialize data buffer
    dvec_t data(mem_sz[0]);
    for (dvec_t::iterator it=data.begin(); it!=data.end(); ++it) {
        *it = 1000*(comm.rank()+1) + (it-data.begin());
    }


    // Select hyperslab in the file.
    std::array<hsize_t,1> offset={comm.rank()*(par.block_size+par.gap_size)};
    std::array<hsize_t,1> count={par.repeat_factor};
    std::array<hsize_t,1> stride={(par.block_size+par.gap_size)*comm.size()};
    std::array<hsize_t,1> block={par.block_size};

    h5::check_error(H5Sselect_hyperslab(filespace, H5S_SELECT_SET, offset.data(), stride.data(), count.data(), block.data()));

    // Create property list for collective dataset write.
    auto xfer_plist_id = h5::plist_wrapper(H5Pcreate(H5P_DATASET_XFER));
    H5Pset_dxpl_mpio(xfer_plist_id, par.do_collective? H5FD_MPIO_COLLECTIVE:H5FD_MPIO_INDEPENDENT);

    // Write the data
    h5::check_error(
        H5Dwrite(dset_id, H5T_NATIVE_DOUBLE, memspace, filespace,
                 xfer_plist_id, data.data()) );

    return 0;
}
