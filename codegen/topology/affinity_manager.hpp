/*
    RAW -- High-performance querying over raw, never-seen-before data.

                            Copyright (c) 2017
        Data Intensive Applications and Systems Labaratory (DIAS)
                École Polytechnique Fédérale de Lausanne

                            All Rights Reserved.

    Permission to use, copy, modify and distribute this software and
    its documentation is hereby granted, provided that both the
    copyright notice and this permission notice appear in all copies of
    the software, derivative works or modified versions, and any
    portions thereof, and that both notices appear in supporting
    documentation.

    This code is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. THE AUTHORS
    DISCLAIM ANY LIABILITY OF ANY KIND FOR ANY DAMAGES WHATSOEVER
    RESULTING FROM THE USE OF THIS SOFTWARE.
*/

#ifndef AFFINITY_MANAGER_HPP_
#define AFFINITY_MANAGER_HPP_

#include "topology/topology.hpp"

class exec_location{
private:
    int         gpu_device;
    cpu_set_t   cpus;

public:
    exec_location(){
        gpu_run(cudaGetDevice(&gpu_device));

        CPU_ZERO(&cpus);
        pthread_getaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpus);
    }

    exec_location(int gpu);

    exec_location(int gpu, cpu_set_t cpus): gpu_device(gpu), cpus(cpus){
    }

    exec_location(cpu_set_t cpus): cpus(cpus){
        gpu_device = -1;
    }

    exec_location(const topology::cpunumanode &cpu): cpus(cpu.local_cpu_set){}

    exec_location(const topology::gpunode     &gpu);

public:
    void activate() const{
        // std::cout << "d" << gpu_device << " " << cpus << std::endl;
        if (gpu_device >= 0) gpu_run(cudaSetDevice(gpu_device));
        pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpus);
    }

};

class set_device_on_scope{
private:
    const topology::gpunode device;
public:
    inline set_device_on_scope(int set):
            device(topology::getInstance().getActiveGpu()){
        if (set >= 0) gpu_run(cudaSetDevice(set));
    }

    inline set_device_on_scope(const topology::gpunode &gpu):
            device(topology::getInstance().getActiveGpu()){
        gpu_run(cudaSetDevice(gpu.id));
    }

    inline ~set_device_on_scope(){
        gpu_run(cudaSetDevice(device.id));
    }
};


class set_exec_location_on_scope{
private:
    exec_location old;
public:
    inline set_exec_location_on_scope(int gpu){
        exec_location{gpu}.activate();
    }

    inline set_exec_location_on_scope(int gpu, cpu_set_t cpus){
        exec_location{gpu, cpus}.activate();
    }

    inline set_exec_location_on_scope(cpu_set_t cpus){
        exec_location{cpus}.activate();
    }

    inline set_exec_location_on_scope(const exec_location &loc){
        loc.activate();
    }

    inline set_exec_location_on_scope(const topology::cpunumanode &cpu){
        exec_location{cpu}.activate();
    }

    inline set_exec_location_on_scope(const topology::gpunode     &gpu){
        exec_location{gpu}.activate();
    }

    inline ~set_exec_location_on_scope(){
        old.activate();
    }
};

int numa_node_of_gpu(int device);


#endif /* AFFINITY_MANAGER_HPP_ */