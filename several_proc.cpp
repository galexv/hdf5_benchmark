/** @file single_proc.cpp Create an HDF5 file by several processes */

#include <string>
#include <iostream>
#include <cstddef>
#include <array>
#include <vector>
#include <H5Cpp.h>
#include <alps/params.hpp>
#include <alps/utilities/mpi.hpp>

H5::H5File open_file(const alps::mpi::communicator& comm, const std::string& fname, int root)
{
    using namespace H5;
    bool is_root=comm.rank()==root;
    if (is_root) {
        H5File fd(fname, H5F_ACC_TRUNC);
        comm.barrier();
        return fd;
    } else {
        comm.barrier();
        H5File fd(fname, H5F_ACC_RDWR);
        return fd;
    }
}

int main(int argc, char** argv)
{
    using namespace H5;
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
    par.define<string>("file", "File name to create").
        define<size_t>("size", "Data size (MB)").
        define<string>("name", "double_set", "Data set name");

    if (par.help_requested(cerr) || par.has_missing(cerr)) {
        return 1;
    }

    hsize_t datasize=par["size"].as<size_t>()*1024*1024/sizeof(double);
    dvec_t data(datasize);

    // TODO: fill the data with random numbers


    // make the dataspace
    std::array<hsize_t,1> dims={datasize};
    DataSpace dataspace(dims.size(), dims.data());

    // make the datatype
    FloatType datatype(PredType::IEEE_F64LE);
    // datatype.setOrder( H5T_ORDER_LE );

    // dataset name
    string dname=par["name"].as<string>()+std::to_string(comm.rank());

    // make the file
    H5File file=open_file(comm, par["file"].as<string>(), master);

    DataSet dataset = file.createDataSet(dname, datatype, dataspace);

    // write the dataset
    dataset.write(data.data(), PredType::IEEE_F64LE);

    return 0;
}
