//
// pch.cpp
// Include the standard header and generate the precompiled header.
//


#include <thread>

#include "pch.h"


const std::size_t hardware_thread_count{ std::thread::hardware_concurrency() };