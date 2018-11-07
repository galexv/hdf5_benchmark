/** @file single_proc.cpp Create an HDF5 file by a single process */

#include <string>
#include <iostream>
#include <cstddef>
#include <array>
#include <vector>
#include <hdf5.h>
#include <alps/params.hpp>


int main(int argc, char** argv)
{
    using std::string;
    using std::size_t;
    using std::cerr;
    using std::cout;
    using std::endl;
    typedef std::vector<double> dvec_t;

    alps::params par(argc, argv);
    par.define<string>("file", "File name to create").
        define<size_t>("size", "Data size (MB)").
        define<string>("name", "double_set", "Data set name");

    if (par.help_requested(cerr) || par.has_missing(cerr)) {
        return 1;
    }

    hsize_t datasize=par["size"].as<size_t>()*1024*1024/sizeof(double);
    dvec_t data(datasize);

    // TODO: fill the data with random numbers

    // make the file
    string fname=par["file"].as<string>();
    auto file_id = H5Fcreate(fname.c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);

    // make the dataspace: 1D array of dimension datasize
    std::array<hsize_t,1> dims={datasize};
    auto dataspace_id=H5Screate_simple(dims.size(), dims.data(), nullptr);

    // make dataset: in this file, with this name, IEEE 64-bit FP, little-endian,
    //               of the dimensions specified by the dataspace
    string dname=par["name"].as<string>();
    auto dataset_id = H5Dcreate2(file_id, dname.c_str(), H5T_IEEE_F64LE, dataspace_id,
                                 H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);

    // write into the dataset: from the whole `double` array to the whole dataset
    auto status=H5Dwrite(dataset_id, H5T_NATIVE_DOUBLE, H5S_ALL, H5S_ALL, H5P_DEFAULT, data.data());

    if (status < 0) {
        cerr << "HDF5 error has occurred\n";
    }

    // free the resources:
    H5Dclose(dataset_id);
    H5Sclose(dataspace_id);
    H5Fclose(file_id);

    return 0;
}
