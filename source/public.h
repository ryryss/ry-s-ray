#pragma once
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <unordered_map>
#include <memory>
#include <array>
#include <chrono>
#include <random>
#include <numeric>
#include <thread>
#include <condition_variable>
#include <future>
#include <functional>
#include <atomic>
#include <queue>
#include <filesystem>

#ifdef DUSE_CUDA
#include <cuda_runtime.h>
#endif // DUSE_CUDA

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>

#include "math.hpp"