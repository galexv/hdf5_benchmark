/** @file sveral_proc_blocks.cpp Writes data to the HDF5 file by repeated blocks with gaps in-between

 */

#include <vector>
#include <string>
#include <alps/params.hpp>
#include "h5_cxx_interface.hpp"

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

    alps::mpi::environment env(argc, argv);
    alps::mpi::communicator comm;
    const int master=0;
    bool is_master = comm.rank()==master;


    alps::params par(argc, argv, comm);
    par
        .define<string>("file", "File name to create")
        .define<size_t>("blocksize", "Number of values per block")
        .define<ptrdiff_t>("gap", 0, "Gap size (number of values that would fit in the gap)")
        .define<size_t>("repeat", 1, "Repeat factor for blocks")
        .define<string>("name", "double_set", "Data set name")
        .define("collective", "Whether to use collective write")
        ;

    if (par.help_requested(cerr) || par.has_missing(cerr)) {
        return 1;
    }

    const bool do_collective=par["collective"];
    const size_t block_size=par["blocksize"];
    const ptrdiff_t gap_size=par["gap"];
    const size_t repeat_factor=par["repeat"];
    const string file_name=par["file"];
    const string data_name=par["name"];

    if (block_size<=0 || repeat_factor<1 || block_size+gap_size<0) {
        if (is_master) cerr << "Incorrect values of parameters\n";
        return 2;
    }

    const hsize_t data_sz=(block_size+gap_size)*repeat_factor*comm.size() - (gap_size>0?gap_size:0);


    // Set up file access property list with parallel I/O access
    auto plist_id = h5::plist_wrapper(H5Pcreate(H5P_FILE_ACCESS));
    H5Pset_fapl_mpio(plist_id, comm, MPI_INFO_NULL);

    // Create a new file collectively and release property list identifier.
    auto file_id = h5::fd_wrapper(H5Fcreate(file_name.c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, plist_id));
    plist_id.close();


    // Create the dataspace for the dataset.
    std::array<hsize_t,1> dims={data_sz};
    auto filespace = h5::dspace_wrapper(H5Screate_simple(dims.size(), dims.data(), nullptr));

    // Create the dataset with default properties
    auto dset_id = h5::dset_wrapper(H5Dcreate(file_id, data_name.c_str(),
                                              H5T_NATIVE_DOUBLE, filespace,
                                              H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT));

    // Each process defines dataset in memory (to write it to the hyperslab later)
    std::array<hsize_t,1> mem_sz={block_size*repeat_factor};
    auto memspace = h5::dspace_wrapper(H5Screate_simple(mem_sz.size(), mem_sz.data(), nullptr));

    // Initialize data buffer
    dvec_t data(mem_sz[0]);
    for (dvec_t::iterator it=data.begin(); it!=data.end(); ++it) {
        *it = 1000*(comm.rank()+1) + (it-data.begin());
    }


    // Select hyperslab in the file.
    std::array<hsize_t,1> offset={comm.rank()*(block_size+gap_size)};
    std::array<hsize_t,1> count={repeat_factor};
    std::array<hsize_t,1> stride={(block_size+gap_size)*comm.size()};
    std::array<hsize_t,1> block={block_size};

    h5::check_error(H5Sselect_hyperslab(filespace, H5S_SELECT_SET, offset.data(), stride.data(), count.data(), block.data()));

    // Create property list for collective dataset write.
    auto xfer_plist_id = h5::plist_wrapper(H5Pcreate(H5P_DATASET_XFER));
    H5Pset_dxpl_mpio(xfer_plist_id, do_collective? H5FD_MPIO_COLLECTIVE:H5FD_MPIO_INDEPENDENT);

    // Write the data
    h5::check_error(
        H5Dwrite(dset_id, H5T_NATIVE_DOUBLE, memspace, filespace,
                 xfer_plist_id, data.data()) );

    return 0;
}
