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
    DISCLAIM ANY LIABILITY OF ANY KIND FOR ANY DAMAGES WHATSOEVER
    RESULTING FROM THE USE OF THIS SOFTWARE.
*/

#include "util/sort/gpu/sort-util.cuh"

//NOTE: Clang fails to compile thrust, but that is fine if we call this part only from generated code
#include "common/gpu/gpu-common.hpp"

#include <thrust/sort.h>
#include <thrust/tuple.h>
#include <thrust/functional.h>
#include <thrust/device_ptr.h>

#include <thrust/system/cuda/execution_policy.h>

template<typename T>
void gpu_qsort(void * ptr, size_t N){
    thrust::device_ptr<T> mem{(T *) ptr};
    assert(N * sizeof(T) <= h_vector_size * sizeof(int32_t) && "Overflow in GPUSort's buffer");
    thrust::sort(thrust::system::cuda::par, mem, mem + N);
}

// 1 attribute:
extern "C" void qsort_i(void * ptr, size_t N){
    gpu_qsort<thrust::tuple<int32_t>>(ptr, N);
}

extern "C" void qsort_l(void * ptr, size_t N){
    gpu_qsort<thrust::tuple<int64_t>>(ptr, N);
}

// 2 attributes:
extern "C" void qsort_ii(void * ptr, size_t N){
    gpu_qsort<thrust::tuple<int32_t, int32_t>>(ptr, N);
}

extern "C" void qsort_il(void * ptr, size_t N){
    gpu_qsort<thrust::tuple<int32_t, int64_t>>(ptr, N);
}

extern "C" void qsort_li(void * ptr, size_t N){
    gpu_qsort<thrust::tuple<int64_t, int32_t>>(ptr, N);
}

extern "C" void qsort_ll(void * ptr, size_t N){
    gpu_qsort<thrust::tuple<int64_t, int64_t>>(ptr, N);
}

// 3 attributes:
extern "C" void qsort_iii(void * ptr, size_t N){
    gpu_qsort<thrust::tuple<int32_t, int32_t, int32_t>>(ptr, N);
}

extern "C" void qsort_iil(void * ptr, size_t N){
    gpu_qsort<thrust::tuple<int32_t, int32_t, int64_t>>(ptr, N);
}

extern "C" void qsort_ili(void * ptr, size_t N){
    gpu_qsort<thrust::tuple<int32_t, int64_t, int32_t>>(ptr, N);
}

extern "C" void qsort_ill(void * ptr, size_t N){
    gpu_qsort<thrust::tuple<int32_t, int64_t, int64_t>>(ptr, N);
}

extern "C" void qsort_lii(void * ptr, size_t N){
    gpu_qsort<thrust::tuple<int64_t, int32_t, int32_t>>(ptr, N);
}

extern "C" void qsort_lil(void * ptr, size_t N){
    gpu_qsort<thrust::tuple<int64_t, int32_t, int64_t>>(ptr, N);
}

extern "C" void qsort_lli(void * ptr, size_t N){
    gpu_qsort<thrust::tuple<int64_t, int64_t, int32_t>>(ptr, N);
}

extern "C" void qsort_lll(void * ptr, size_t N){
    gpu_qsort<thrust::tuple<int64_t, int64_t, int64_t>>(ptr, N);
}

// 4 attributes:
extern "C" void qsort_iiii(void * ptr, size_t N){
    gpu_qsort<thrust::tuple<int32_t, int32_t, int32_t, int32_t>>(ptr, N);
}

extern "C" void qsort_iiil(void * ptr, size_t N){
    gpu_qsort<thrust::tuple<int32_t, int32_t, int32_t, int64_t>>(ptr, N);
}

extern "C" void qsort_iili(void * ptr, size_t N){
    gpu_qsort<thrust::tuple<int32_t, int32_t, int64_t, int32_t>>(ptr, N);
}

extern "C" void qsort_iill(void * ptr, size_t N){
    gpu_qsort<thrust::tuple<int32_t, int32_t, int64_t, int64_t>>(ptr, N);
}

extern "C" void qsort_ilii(void * ptr, size_t N){
    gpu_qsort<thrust::tuple<int32_t, int64_t, int32_t, int32_t>>(ptr, N);
}

extern "C" void qsort_ilil(void * ptr, size_t N){
    gpu_qsort<thrust::tuple<int32_t, int64_t, int32_t, int64_t>>(ptr, N);
}

extern "C" void qsort_illi(void * ptr, size_t N){
    gpu_qsort<thrust::tuple<int32_t, int64_t, int64_t, int32_t>>(ptr, N);
}

extern "C" void qsort_illl(void * ptr, size_t N){
    gpu_qsort<thrust::tuple<int32_t, int64_t, int64_t, int64_t>>(ptr, N);
}

extern "C" void qsort_liii(void * ptr, size_t N){
    gpu_qsort<thrust::tuple<int64_t, int32_t, int32_t, int32_t>>(ptr, N);
}

extern "C" void qsort_liil(void * ptr, size_t N){
    gpu_qsort<thrust::tuple<int64_t, int32_t, int32_t, int64_t>>(ptr, N);
}

extern "C" void qsort_lili(void * ptr, size_t N){
    gpu_qsort<thrust::tuple<int64_t, int32_t, int64_t, int32_t>>(ptr, N);
}

extern "C" void qsort_lill(void * ptr, size_t N){
    gpu_qsort<thrust::tuple<int64_t, int32_t, int64_t, int64_t>>(ptr, N);
}

extern "C" void qsort_llii(void * ptr, size_t N){
    gpu_qsort<thrust::tuple<int64_t, int64_t, int32_t, int32_t>>(ptr, N);
}

extern "C" void qsort_llil(void * ptr, size_t N){
    gpu_qsort<thrust::tuple<int64_t, int64_t, int32_t, int64_t>>(ptr, N);
}

extern "C" void qsort_llli(void * ptr, size_t N){
    gpu_qsort<thrust::tuple<int64_t, int64_t, int64_t, int32_t>>(ptr, N);
}

extern "C" void qsort_llll(void * ptr, size_t N){
    gpu_qsort<thrust::tuple<int64_t, int64_t, int64_t, int64_t>>(ptr, N);
}