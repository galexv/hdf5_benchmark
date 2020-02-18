/** @file single_proc.cpp Create an HDF5 file by a single process */

#include <string>
#include <iostream>
#include <cstddef>
#include <array>
#include <vector>

#include "h5_cxx_interface.hpp"

#include <cmdline.hpp>
namespace po=program_options;


int main(int argc, char** argv)
{
    using std::string;
    using std::size_t;
    using std::cerr;
    using std::cout;
    using std::endl;
    typedef std::vector<double> dvec_t;

    auto par = po::parse(argc, argv);
    if (!par) {
        cerr << "Usage: " << argv[0]
             << " file=<filename_to_create> size=<data_size_MB> name=<data_set_name>\n";
        return 1;
    }

    auto maybe_datasize = par->get<size_t>("size");
    if (!maybe_datasize) {
        cerr << "Invalid or missing datasize\n";
        return 2;
    }

    auto maybe_fname=par->get<string>("file");
    if (!maybe_fname) {
        cerr << "Invalid or missing file name\n";
        return 2;
    }

    auto maybe_dname=par->get<string>("name");
    if (!maybe_dname) {
        cerr << "Invalid or missing dataset name\n";
        return 2;
    }
    
    
    hsize_t datasize=(*maybe_datasize)*1024*1024/sizeof(double);
    dvec_t data(datasize);

    // TODO: fill the data with random numbers

    // make the file
    h5::fd_wrapper file_id{ H5Fcreate(maybe_fname->c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT) };

    // make the dataspace: 1D array of dimension datasize
    std::array<hsize_t,1> dims={datasize};
    h5::dspace_wrapper dataspace_id{ H5Screate_simple(dims.size(), dims.data(), nullptr) };

    // make dataset: in this file, with this name, IEEE 64-bit FP, little-endian,
    //               of the dimensions specified by the dataspace
    h5::dset_wrapper dataset_id{ H5Dcreate2(file_id, maybe_dname->c_str(), H5T_IEEE_F64LE, dataspace_id,
                                            H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT) };

    // write into the dataset: from the whole `double` array to the whole dataset
    auto status=H5Dwrite(dataset_id, H5T_NATIVE_DOUBLE, H5S_ALL, H5S_ALL, H5P_DEFAULT, data.data());

    if (status < 0) {
        cerr << "HDF5 error has occurred\n";
    }

    return 0;
}
