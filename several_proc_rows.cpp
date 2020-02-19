/** @file sveral_proc_rows.cpp Writes data to the HDF5 file by rows.

    Adopted from https://support.hdfgroup.org/ftp/HDF5/examples/parallel/Hyperslab_by_row.c
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
    std::size_t nrows;
    std::size_t ncols;
    std::string data_name;
    bool do_collective;
};

namespace mpiwrap {
    void bcast(const communicator& comm, const my_params& par, int root)
    {
        if (comm.rank()!=root) {
            throw std::runtime_error("Cannot bcast a const from non-root");
        }
        bcast(comm, par.file_name, root);
        bcast(comm, par.nrows, root);
        bcast(comm, par.ncols, root);
        bcast(comm, par.data_name, root);
        bcast(comm, par.do_collective, root);
    }

    void bcast(const communicator& comm, my_params& par, int root)
    {
        bcast(comm, par.file_name, root);
        bcast(comm, par.nrows, root);
        bcast(comm, par.ncols, root);
        bcast(comm, par.data_name, root);
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
                      << " file=<file_name> rows=<number> cols=<number> [name=<dataset_name>] collective=<yes|no>"
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

        auto maybe_rows = par->get<std::size_t>("rows");
        if (!maybe_rows) {
            std::cerr << "rows parameter is missing or invalid\n";
            return empty;
        }
        
        auto maybe_cols = par->get<std::size_t>("cols");
        if (!maybe_cols) {
            std::cerr << "cols parameter is missing or invalid\n";
            return empty;
        }
        
        auto maybe_name = par->get_or("name", "double_set");
        if (!maybe_name) {
            std::cerr << "name parameter is missing or invalid\n";
            return empty;
        }

        const my_params my_par = {
            *maybe_file,
            *maybe_rows,
            *maybe_cols,
            *maybe_name,
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
    using std::cerr;
    using std::cout;
    using std::endl;
    typedef std::vector<double> dvec_t;

    mpi::environment env(argc, argv);
    mpi::communicator comm;
    const int master=0;
    bool is_master = comm.rank()==master;

    const auto maybe_par = parse_and_bcast(argc, argv, comm);
    if (!maybe_par) {
        env.abort(3);
        return 3;
    }
    const auto& par = *maybe_par;

    // DEBUG:
    for (int r=0; r<comm.size(); ++r) {
        if (comm.rank()==r) {
            cout << std::boolalpha
                 << "Rank " << r << " is running with"
                 << " file_name=" << par.file_name
                 << " (rows,cols)=(" << par.nrows << ", " << par.ncols
                 << ") data_name=" << par.data_name
                 << " collective=" << par.do_collective
                 << std::endl;
        }
        comm.barrier();
    }

    
    /*
     * Set up file access property list with parallel I/O access
     */
    auto plist_id = h5::plist_wrapper(H5Pcreate(H5P_FILE_ACCESS));
    H5Pset_fapl_mpio(plist_id, comm, MPI_INFO_NULL);

    /*
     * Create a new file collectively and release property list identifier.
     */
    auto file_id = h5::fd_wrapper(H5Fcreate(par.file_name.c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, plist_id));
    plist_id.close();


    /*
     * Create the dataspace for the dataset.
     */
    std::array<hsize_t,2> dims={par.nrows, par.ncols};
    auto filespace = h5::dspace_wrapper(H5Screate_simple(dims.size(), dims.data(), nullptr));

    /*
     * Create the dataset with default properties
     */
    auto dset_id = h5::dset_wrapper(H5Dcreate(file_id, par.data_name.c_str(),
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
    H5Pset_dxpl_mpio(xfer_plist_id, par.do_collective? H5FD_MPIO_COLLECTIVE:H5FD_MPIO_INDEPENDENT);

    /*
      Write the data
    */
    auto status = H5Dwrite(dset_id, H5T_NATIVE_DOUBLE, memspace, filespace,
                           xfer_plist_id, data.data());

    return status;
}
