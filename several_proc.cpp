/** @file single_proc.cpp Create an HDF5 file by several processes */

#include <string>
#include <iostream>
#include <cstddef>
#include <array>
#include <vector>
#include <hdf5.h>
#include <alps/params.hpp>
#include <alps/utilities/mpi.hpp>

int main(int argc, char** argv)
{
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
        .define<size_t>("size", "Data size (MB)")
        .define<string>("name", "double_set", "Data set name")
        .define("collective", "Whether to use collective write")
        ;

    if (par.help_requested(cerr) || par.has_missing(cerr)) {
        return 1;
    }

    const bool do_collective=par["collective"];
    hsize_t datasize=par["size"].as<size_t>()*1024*1024/sizeof(double);
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
    string fname=par["file"].as<string>();
    auto file_id = H5Fcreate(fname.c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, plist_id);


    // make the dataspace: 1D array of dimension datasize
    std::array<hsize_t,1> dims={datasize};
    auto dataspace_id=H5Screate_simple(dims.size(), dims.data(), nullptr);


    // make datasets: in this file, with this name, IEEE 64-bit FP, little-endian,
    //                of the dimensions specified by the dataspace
    // caveat: all processes must make all datasets
    size_t nsets=comm.size();
    std::vector<hid_t> dsets(nsets);
    for (size_t i=0; i<nsets; ++i) {
        string dname=par["name"].as<string>()+std::to_string(i);
        dsets[i] = H5Dcreate2(file_id, dname.c_str(), H5T_IEEE_F64LE, dataspace_id,
                              H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
        // cerr << "Rank " << comm.rank() << ": dsets[" << i << "]=" << dsets[i] << endl;
        if (dsets[i]==-1) env.abort(1);
    }

    // make the data-transfer property
    auto plist_xfer_id=H5Pcreate(H5P_DATASET_XFER);
    // set this property to independent or collective IO
    const auto mode = do_collective? H5FD_MPIO_COLLECTIVE : H5FD_MPIO_INDEPENDENT;
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
