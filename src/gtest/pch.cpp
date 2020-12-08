//
// pch.cpp
// Include the standard header and generate the precompiled header.
//


#include "pch.h"

#include <thread>


const std::size_t hardware_thread_count{ std::thread::hardware_concurrency() };