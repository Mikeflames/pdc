## Proactive Data Containers (PDC)
Proactive Data Containers (PDC) provides an object-centric API and a runtime system with a set of services that make intelligent data placement decisions in the memory and storage hierarchy and provide scalable metadata operations. Using object-centric abstractions to represent data that moves in the high-performance computing (HPC) memory and storage subsystems, PDC revolutionizes how scientific data is stored and accessed. PDC manages extensive metadata describing data objects, which allows finding desired data efficiently.

PDC API, data types, and developer notes are available in docs/readme.md. 

## Installation 
The following instructions are for installing PDC on Linux and Cray machines. 
GCC version 7 or newer and a version of MPI are needed to install PDC. 
Current PDC tests have been verified with MPICH. To install MPICH, follow the documentation in https://www.mpich.org/static/downloads/3.4.1/mpich-3.4.1-installguide.pdf
PDC also depends on libfabric and Mercury. We provide detailed instructions for installing libfabric, Mercury, and PDC below.
Make sure to record the environmental variables (lines that contains the "export" commands). They are needed for running PDC and to use the libraries again.
# Install libfabric
```
0. wget https://github.com/ofiwg/libfabric/archive/v1.11.2.tar.gz
1. tar xvzf v1.11.2.tar.gz
2. cd libfabric-1.11.2
3. mkdir install
4. export LIBFABRIC_DIR=$(pwd)/install
5. ./autogen.sh
6. ./configure --prefix=$LIBFABRIC_DIR CC=gcc CFLAG="-O2"
7. make -j8
8. make install
9. export LD_LIBRARY_PATH="$LIBFABRIC_DIR/lib:$LD_LIBRARY_PATH"
10. export PATH="$LIBFABRIC_DIR/include:$LIBFABRIC_DIR/lib:$PATH"
```
# Install Mercury
Make sure the ctest passes. PDC may not work without passing all the tests of Mercury.
Step 2 is not required. It is a stable commit I am using when I write this instruction. You may skip it if you believe the current master branch of mercury works.
```
0. git clone https://github.com/mercury-hpc/mercury.git
1. cd mercury
2. git checkout e741051fbe6347087171f33119d57c48cb438438
3. git submodule update --init
4. export MERCURY_DIR=$(pwd)/install
5. mkdir install
6. cd install
7. cmake ../ -DCMAKE_INSTALL_PREFIX=$MERCURY_DIR -DCMAKE_C_COMPILER=gcc -DBUILD_SHARED_LIBS=ON -DBUILD_TESTING=ON -DNA_USE_OFI=ON -DNA_USE_SM=OFF
8. make
9. make install
10. ctest
11. export LD_LIBRARY_PATH="$MERCURY_DIR/lib:$LD_LIBRARY_PATH"
12. export PATH="$MERCURY_DIR/include:$MERCURY_DIR/lib:$PATH"
```
# Install PDC
You can replace mpicc to whatever MPI compilers you are using. For example, on Cori, you may need to use cc.
The ctest contains both sequential and MPI tests for our settings. These regression tests should work.
```
0. git clone https://github.com/hpc-io/pdc.git
1. cd pdc
2. git checkout qiao_develop
3. cd src
4. mkdir install
5. cd install
6. export PDC_DIR=$(pwd)
7. cmake ../ -DBUILD_MPI_TESTING=ON -DBUILD_SHARED_LIBS=ON -DBUILD_TESTING=ON -DCMAKE_INSTALL_PREFIX=$PDC_DIR -DPDC_ENABLE_MPI=ON -DMERCURY_DIR=$MERCURY_DIR -DCMAKE_C_COMPILER=mpicc
8. make
9. make -j8
10. ctest
```

# Environmental variables
During installation, we have set some environmental variables. These variables will disappear when you close the current shell.
I recommend adding the following lines to ~/.bashrc. (you can also manually execute them when you login).
The MERCURY_DIR and LIBFABRIC_DIR should be identical to the values you set during your installations of Mercury and Libfabric.
Remember, the install path is the path containing bin and lib directory, instead of the one containing source code.
```
export PDC_DIR="where/you/installed/your/pdc"
export MERCURY_DIR="where/you/installed/your/mercury"
export LIBFABRIC_DIR="where/you/installed/your/libfabric"
export LD_LIBRARY_PATH="$LIBFABRIC_DIR/lib:$MERCURY_DIR/lib:$LD_LIBRARY_PATH"
export PATH="$LIBFABRIC_DIR/include:$LIBFABRIC_DIR/lib:$MERCURY_DIR/include:$MERCURY_DIR/lib:$PATH"
```
You can also manage the path with Spack, which is a lot more easier to load and unload these libraries.
## Running PDC
The ctest under PDC install folder runs PDC examples using PDC APIs.
PDC needs to run at least two applications. First, you need to start servers. Then, you can run client programs that send I/O request to servers as mercury RPCs.
For example, you can do the following. On Cori, you need to change the mpiexec argument to srun. On Theta, it is aprun. On Summit, it is jsrun.
```
cd $PDC_DIR/bin
./mpi_test.sh ./pdc_init mpiexec 2 4
```
This is test will start 2 processes for PDC servers. The client program ./pdc_init will start 4 processes. Similarly, you can run any of the client examples in ctest.
These source code will provide you some knowledge of how to use PDC. For more reference, you may check the documentation folder in this repository.
# PDC on Cori.
Installation on Cori is not very different from a regular linux machine. Simply replace all gcc/mpicc with the default cc compiler on Cori. Add options -DCMAKE_C_FLAGS="-dynamic" to the cmake line of PDC. Add -DCMAKE_C_FLAGS="-dynamic" -DCMAKE_CXX_FLAGS="-dynamic" at the end of the cmake line for mercury as well. Finally, "-DMPI_RUN_CMD=srun" is needed for ctest command later. Sometimes you may need to unload darshan before installation.


