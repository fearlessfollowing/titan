
#include <cuda_runtime.h>
#include <cudaEGL.h>
#include <vector>

int transform_scaling_cu(const CUeglFrame& in_eglframe, const std::vector<CUeglFrame>& v_out_eglframe, std::vector<bool>& v_half);