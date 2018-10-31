/** @file single_proc.cpp Create an HDF5 file by a single process */

#include <string>
#include <iostream>
#include <cstddef>
#include <array>
#include <vector>
#include <H5Cpp.h>
#include <alps/params.hpp>

int main(int argc, char** argv)
{
    using namespace H5;
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
    H5File file(fname, H5F_ACC_TRUNC);

    // make the dataspace
    std::array<hsize_t,1> dims={datasize};
    DataSpace dataspace(dims.size(), dims.data());

    // make the datatype
    FloatType datatype(PredType::IEEE_F64LE);
    // datatype.setOrder( H5T_ORDER_LE );

    // make dataset
    string dname=par["name"].as<string>();
    DataSet dataset = file.createDataSet(dname, datatype, dataspace);

    // write the dataset
    dataset.write(data.data(), PredType::IEEE_F64LE);

    return 0;
}
