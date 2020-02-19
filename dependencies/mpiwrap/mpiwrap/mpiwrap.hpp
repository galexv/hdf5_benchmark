/* C++ wrappers around a few MPI calls, inspired by boost::mpi and ALPCore */

#include <mpi.h>
#include <type_traits>
#include <cstdint>
#include <stdexcept>

namespace mpiwrap {

    /// RAII-style MPI initilizer/finalizer
    class environment {
      public:
        environment(int argc, char** argv)
        {
            int is_in;
            MPI_Initialized(&is_in);
            if (!is_in) MPI_Init(&argc, &argv);
        }

        virtual ~environment()
        {
            int flag;
            MPI_Initialized(&flag);
            if (!flag) return;

            MPI_Finalized(&flag);
            if (!flag) MPI_Finalize();
        }

        void abort(int rc) const
        {
            MPI_Abort(MPI_COMM_WORLD, rc);
        }
    };


    /// wrapper around MPI communicator
    class communicator {
        MPI_Comm comm_;
      public:
        /// create a WORLD communicator
        communicator(): comm_(MPI_COMM_WORLD) {}

        // TODO: if needed, construct from an existing MPI communicator
        //       by appropriating or duplicating
        
        communicator(const communicator&) =delete;
        communicator& operator=(const communicator&) =delete;

        /// implicitly convert to a vanilla MPI communicator
        operator MPI_Comm() const { return comm_; }


        int size() const
        {
            int sz;
            MPI_Comm_size(comm_, &sz);
            return sz;
        }

        int rank() const
        {
            int r;
            MPI_Comm_rank(comm_, &r);
            return r;
        }

        void barrier() const
        {
            MPI_Barrier(comm_);
        }
        
    };
    

    template <typename T>
    inline void bcast(const communicator& comm, const T& val, int root)
    {
        static_assert(std::is_trivially_copyable<T>::value,
                      "Generic broadcast is possible only for trivially-copyable types");
        if (comm.rank()!=root) {
            throw std::runtime_error("bcast() of a constant from non-root");
        }
        auto* buf = const_cast<void*>(static_cast<const void*>(&val));
        MPI_Bcast(buf, sizeof(val), MPI_BYTE, root, comm);
    }

    
    template <typename T>
    inline void bcast(const communicator& comm, T& val, int root)
    {
        static_assert(std::is_trivially_copyable<T>::value,
                      "Generic broadcast is possible only for trivially-copyable types");
        auto* buf = static_cast<void*>(&val);
        MPI_Bcast(buf, sizeof(val), MPI_BYTE, root, comm);
    }


    // Broadcasting of a few chosen STL types

   
    inline void bcast(const communicator& comm, const std::string& val, int root)
    {
        if (comm.rank()!=root) {
            throw std::runtime_error("bcast() of a constant std::string from non-root");
        }
        unsigned long len = val.size();
        MPI_Bcast(&len, 1, MPI_UNSIGNED_LONG, root, comm);
        if (len!=0) {
            auto* buf = const_cast<char*>(&val[0]);
            MPI_Bcast(buf, len, MPI_CHAR, root, comm);
        }
    }

    
    inline void bcast(const communicator& comm, std::string& val, int root)
    {
        if (comm.rank()==root) {
            bcast(comm, const_cast<const std::string&>(val), root);
            return;
        }
        unsigned long len;
        MPI_Bcast(&len, 1, MPI_UNSIGNED_LONG, root, comm);
        std::string new_string(len, '\0');
        if (len!=0) {
            char* buf = &new_string[0];
            MPI_Bcast(buf, len, MPI_CHAR, root, comm);
        }
        std::swap(val, new_string);
    }
    
}
