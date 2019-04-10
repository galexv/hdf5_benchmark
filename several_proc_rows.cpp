/** @file sveral_proc_rows.cpp Writes data to the HDF5 file by rows.

    Adopted from https://support.hdfgroup.org/ftp/HDF5/examples/parallel/Hyperslab_by_row.c
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
        // .define<size_t>("size", "Data size (MB)")
        .define<size_t>("rows", "Number of rows")
        .define<size_t>("cols", "Number of columns")
        .define<string>("name", "double_set", "Data set name")
        .define("collective", "Whether to use collective write")
        ;

    if (par.help_requested(cerr) || par.has_missing(cerr)) {
        return 1;
    }

    const bool do_collective=par["collective"];
    const size_t nrows=par["rows"];
    const size_t ncols=par["cols"];
    const string file_name=par["file"];
    const string data_name=par["name"];


    /*
     * Set up file access property list with parallel I/O access
     */
    auto plist_id = h5::plist_wrapper(H5Pcreate(H5P_FILE_ACCESS));
    H5Pset_fapl_mpio(plist_id, comm, MPI_INFO_NULL);

    /*
     * Create a new file collectively and release property list identifier.
     */
    auto file_id = h5::fd_wrapper(H5Fcreate(file_name.c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, plist_id));
    plist_id.close();


    /*
     * Create the dataspace for the dataset.
     */
    std::array<hsize_t,2> dims={nrows, ncols};
    auto filespace = h5::dspace_wrapper(H5Screate_simple(dims.size(), dims.data(), nullptr));

    /*
     * Create the dataset with default properties
     */
    auto dset_id = h5::dset_wrapper(H5Dcreate(file_id, data_name.c_str(),
                                              H5T_NATIVE_DOUBLE, filespace,
                                              H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT));
    //AG: let's NOT close it:
    // filespace.close();

    /*
     * Each process defines dataset in memory and writes it to the hyperslab
     * in the file.
     */
    std::array<hsize_t,2> count={dims[0]/comm.size(), dims[1]};
    std::array<hsize_t,2> offset={comm.rank()*count[0], 0};
    auto memspace = h5::dspace_wrapper(H5Screate_simple(count.size(), count.data(), nullptr));

    /*
     * Select hyperslab in the file.
     */
    // AG: we already have it, from prior code:
    // filespace = H5Dget_space(dset_id);
    H5Sselect_hyperslab(filespace, H5S_SELECT_SET, offset.data(), nullptr, count.data(), nullptr);

    /*
     * Initialize data buffer
     */
    dvec_t data(count[0]*count[1], 10+comm.rank());

    /*
     * Create property list for collective dataset write.
     */
    auto xfer_plist_id = h5::plist_wrapper(H5Pcreate(H5P_DATASET_XFER));
    H5Pset_dxpl_mpio(xfer_plist_id, do_collective? H5FD_MPIO_COLLECTIVE:H5FD_MPIO_INDEPENDENT);

    /*
      Write the data
    */
    auto status = H5Dwrite(dset_id, H5T_NATIVE_DOUBLE, memspace, filespace,
                           xfer_plist_id, data.data());

    return status;
}
