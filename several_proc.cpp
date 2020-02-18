/** @file single_proc.cpp Create an HDF5 file by several processes */

#include <string>
#include <iostream>
#include <cstddef>
#include <array>
#include <vector>
#include <hdf5.h>

#include <cmdline/cmdline.hpp>
#include <mpiwrap/mpiwrap.hpp>

namespace po=program_options;
namespace mpi=mpiwrap;

struct my_params {
    std::string file;
    size_t size;
    std::string name;
    bool do_collective;
};

namespace mpiwrap {
    void bcast(const communicator& comm, const my_params& par, int root)
    {
        if (comm.rank()!=root) {
            throw std::runtime_error("Cannot bcast a const from non-root");
        }
        bcast(comm, par.file, root);
        bcast(comm, par.size, root);
        bcast(comm, par.name, root);
        bcast(comm, par.do_collective, root);
    }

    void bcast(const communicator& comm, my_params& par, int root)
    {
        bcast(comm, par.file, root);
        bcast(comm, par.size, root);
        bcast(comm, par.name, root);
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
        mpi::bcast(comm, bool(par), master);
        if (!par) {
            std::cerr << "Usage: " << argv[0]
                      << " file=<file_name> size=<data_size_MB> name=<dataset_name> collective=<yes|no>"
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

        auto maybe_size = par->get<std::size_t>("size");
        if (!maybe_size) {
            std::cerr << "size parameter is missing or invalid\n";
            return empty;
        }
        
        auto maybe_name = par->get<std::string>("name");
        if (!maybe_size) {
            std::cerr << "name parameter is missing or invalid\n";
            return empty;
        }

        const my_params my_par = {
            *maybe_file,
            *maybe_size,
            *maybe_name,
            *maybe_collective
        };
        mpi::bcast(comm, my_par, master);
        return po::make_optional(my_par);
    }

    bool ok;
    mpi::bcast(comm, ok, master);
    if (!ok) return empty;
    
    my_params my_par;
    mpi::bcast(comm, my_par, master);
    return po::make_optional(my_par);
}


int main(int argc, char** argv)
{
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
    if (!maybe_par) return 2;
    const auto& par = *maybe_par;

    // DEBUG:
    for (int r=0; r<comm.size(); ++r) {
        if (comm.rank()==r) {
            cout << std::boolalpha
                 << "Rank " << r << " is running with"
                 << " file=" << par.file
                 << " size=" << par.size
                 << " name=" << par.name
                 << " collective=" << par.do_collective
                 << std::endl;
        }
        comm.barrier();
    }
    
    
    hsize_t datasize=par.size*1024*1024/sizeof(double);
    dvec_t data(datasize, comm.rank());

    // TODO: fill the data with random numbers

    // make the file-access property
    auto plist_id=H5Pcreate(H5P_FILE_ACCESS);
    // set this property to parallel access:
    //  1) not collective
    //  2) `comm` is duplicated and remembered
    //  3) `info` object is duplicated
    H5Pset_fapl_mpio(plist_id, comm, MPI_INFO_NULL);

    // make the file (collectively!)
    auto file_id = H5Fcreate(par.file.c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, plist_id);


    // make the dataspace: 1D array of dimension datasize
    std::array<hsize_t,1> dims={datasize};
    auto dataspace_id=H5Screate_simple(dims.size(), dims.data(), nullptr);


    // make datasets: in this file, with this name, IEEE 64-bit FP, little-endian,
    //                of the dimensions specified by the dataspace
    // caveat: all processes must make all datasets
    size_t nsets=comm.size();
    std::vector<hid_t> dsets(nsets);
    for (size_t i=0; i<nsets; ++i) {
        string dname=par.name+std::to_string(i);
        dsets[i] = H5Dcreate2(file_id, dname.c_str(), H5T_IEEE_F64LE, dataspace_id,
                              H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
        // cerr << "Rank " << comm.rank() << ": dsets[" << i << "]=" << dsets[i] << endl;
        if (dsets[i]==-1) env.abort(1);
    }

    // make the data-transfer property
    auto plist_xfer_id=H5Pcreate(H5P_DATASET_XFER);
    // set this property to independent or collective IO
    const auto mode = par.do_collective? H5FD_MPIO_COLLECTIVE : H5FD_MPIO_INDEPENDENT;
    H5Pset_dxpl_mpio(plist_xfer_id, mode);

    // write into the dataset:
    //   from the whole `double` array to the whole dataset,
    //   using the specified transfer mode (collective or independent)
    auto status=H5Dwrite(dsets[comm.rank()], H5T_NATIVE_DOUBLE, H5S_ALL, H5S_ALL, plist_xfer_id, data.data());

    if (status < 0) {
        cerr << "HDF5 error has occurred on rank " << comm.rank() << endl;
    }

    // free the resources:
    H5Pclose(plist_xfer_id);
    for (auto id : dsets) {
        H5Dclose(id);
    }
    H5Sclose(dataspace_id);
    H5Fclose(file_id);
    H5Pclose(plist_id);
    return 0;
}
