DESCRIPTION
===========

Programs to benchmark I/O in parallel via HDF5 library


BUILDING
========

On Great Lakes:

```
$ module add cmake gcc/8.2.0 openmpi/3.1.4 phdf5
$ cd hdf5_benchmarks
$ mkdir -p 000build && cd 000build
$ export CC=$(which mpicc)
$ export CXX=$(which mpic++)
$ cmake .. 
$ make
$ ls -F | grep -F \*
several_proc*
several_proc_blocks*
several_proc_rows*
single_proc*

```

RUNNING
=======

1. To run on a single core:

```
$ ./single_proc file=test.h5 size=10 name=data
$ ls *.h5
test.h5
```

2. To run in parallel, each process creates a dataset (in this example, 10MB size)

```
$ mpiexec -n 2 ./several_proc file=test1.h5 size=10 name=data collective=yes
Rank 0 is running with file=test1.h5 size=10 name=data collective=true
Rank 1 is running with file=test1.h5 size=10 name=data collective=true

$ du -hc test1.h5
23M     test1.h5
23M     total

$ h5dump -H test1.h5 
HDF5 "test1.h5" {
GROUP "/" {
   DATASET "data0" {
      DATATYPE  H5T_IEEE_F64LE
      DATASPACE  SIMPLE { ( 1310720 ) / ( 1310720 ) }
   }
   DATASET "data1" {
      DATATYPE  H5T_IEEE_F64LE
      DATASPACE  SIMPLE { ( 1310720 ) / ( 1310720 ) }
   }
}
}


```
The file is called `test1.h5`, each dataset is 10MB size, the names of the datasets
start with `data`, and the HDF5 I/O operations are requested to be collective.

3. To run in parallel, each process writes several rows into the same rectangular array dataset
```
$ mpiexec -n 2 ./several_proc_rows file=test2.h5 rows=1024 cols=1024 collective=yes 
Rank 0 is running with file_name=test2.h5 (rows,cols)=(1024, 1024) data_name=double_set collective=true
Rank 1 is running with file_name=test2.h5 (rows,cols)=(1024, 1024) data_name=double_set collective=true

$ du -hc test2.h5
9.1M    test2.h5
9.1M    total

$ h5dump -H test2.h5 
HDF5 "test2.h5" {
GROUP "/" {
   DATASET "double_set" {
      DATATYPE  H5T_IEEE_F64LE
      DATASPACE  SIMPLE { ( 1024, 1024 ) / ( 1024, 1024 ) }
   }
}
}


```
In this example, it is 1024 rows total, each of the 2 processes wrote 512 rows.

4. To run in parallel, each process writes several blocks (slabs) into the same dataset.
```
$ mpiexec -n 3 ./several_proc_blocks file=test3.h5 blocksize=10 gap=6 repeat=2 collective=yes 
Rank 0 is running with file=test3.h5 blocksize=10 gap=6 repeat=2 name=double_set collective=true
Rank 1 is running with file=test3.h5 blocksize=10 gap=6 repeat=2 name=double_set collective=true
Rank 2 is running with file=test3.h5 blocksize=10 gap=6 repeat=2 name=double_set collective=true

$ du -hc test3.h5
26K     test3.h5
26K     total

$ h5dump -H test3.h5 
HDF5 "test3.h5" {
GROUP "/" {
   DATASET "double_set" {
      DATATYPE  H5T_IEEE_F64LE
      DATASPACE  SIMPLE { ( 90 ) / ( 90 ) }
   }
}
}

$ h5dump test3.h5 
HDF5 "test3.h5" {
GROUP "/" {
   DATASET "double_set" {
      DATATYPE  H5T_IEEE_F64LE
      DATASPACE  SIMPLE { ( 90 ) / ( 90 ) }
      DATA {
      (0): 1000, 1001, 1002, 1003, 1004, 1005, 1006, 1007, 1008, 1009, 0, 0,
      (12): 0, 0, 0, 0, 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008,
      (25): 2009, 0, 0, 0, 0, 0, 0, 3000, 3001, 3002, 3003, 3004, 3005, 3006,
      (39): 3007, 3008, 3009, 0, 0, 0, 0, 0, 0, 1010, 1011, 1012, 1013, 1014,
      (53): 1015, 1016, 1017, 1018, 1019, 0, 0, 0, 0, 0, 0, 2010, 2011, 2012,
      (67): 2013, 2014, 2015, 2016, 2017, 2018, 2019, 0, 0, 0, 0, 0, 0, 3010,
      (81): 3011, 3012, 3013, 3014, 3015, 3016, 3017, 3018, 3019
      }
   }
}
}

$ 
```
In this example, each of the 3 processes writes a block of 10 double values, followed by a gap of 6 values.
The blocks and gaps are repeated 2 times.

In other words: here we have 3 processes with repeat factor of 2, so the dataset will look like this:

```
B11 G B21 G B31 G B12 G B22 G B32
```

where `Bnm` is a block written by process `n` on repetition `m`, and `G` is a gap.

