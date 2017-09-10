#include "common/gpu/gpu-common.hpp"

void launch_kernel(CUfunction function, void ** args, dim3 gridDim, dim3 blockDim){
    gpu_run(cuLaunchKernel(function, gridDim.x, gridDim.y, gridDim.z,
                                 blockDim.x, blockDim.y, blockDim.z,
                                 0, NULL, args, NULL));
}

void launch_kernel(CUfunction function, void ** args, dim3 gridDim){
    launch_kernel(function, args, gridDim, defaultBlockDim);
}

extern "C" {
void launch_kernel(CUfunction function, void ** args){
    launch_kernel(function, args, defaultGridDim, defaultBlockDim);
}
}

std::ostream& operator<<(std::ostream& out, const cpu_set_t& cpus) {
    long cores = sysconf(_SC_NPROCESSORS_ONLN);

    bool printed = false;

    for (int i = 0 ; i < cores ; ++i) if (CPU_ISSET(i, &cpus)) {
        if (printed) out << ",";
        printed = true;
        out << i;
    }

    return out;
}

mmap_file::mmap_file(std::string name, data_loc loc): loc(loc){
    time_block t("Topen (" + name + "): ");

    filesize = ::getFileSize(name.c_str());
    fd       = open(name.c_str(), O_RDONLY, 0);

    if (fd == -1){
        string msg("[Storage: ] Failed to open input file " + name);
        LOG(ERROR) << msg;
        throw runtime_error(msg);
    }

    //Execute mmap
    data     = mmap(NULL, filesize, PROT_READ, MAP_PRIVATE | MAP_POPULATE, fd, 0);
    assert(data != MAP_FAILED);

    // gpu_run(cudaHostRegister(data, filesize, 0));
    if (loc == PINNED){
        void * data2;
        gpu_run(cudaMallocHost(&data2, filesize));
        memcpy(data2, data, filesize);
        munmap(data, filesize);
        close (fd  );
        data = data2;
    }

    gpu_data = data;

    if (loc == GPU_RESIDENT){
        gpu_run(cudaMalloc(&gpu_data,       filesize));
        gpu_run(cudaMemcpy( gpu_data, data, filesize, cudaMemcpyDefault));
    }
}

mmap_file::~mmap_file(){
    if (loc == GPU_RESIDENT) gpu_run(cudaFree(gpu_data));

    // gpu_run(cudaHostUnregister(data));
    if (loc == PINNED)       gpu_run(cudaFreeHost(data));

    if (loc == PAGEABLE){
        munmap(data, filesize);
        close (fd  );
    }
}

const void * mmap_file::getData() const{
    return gpu_data;
}

size_t mmap_file::getFileSize() const{
    return filesize;
}