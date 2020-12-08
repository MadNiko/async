
#pragma once


namespace async
{

#ifdef ASYNC_LIB_HEADERS_ONLY
#   define ASYNC_INLINE inline
    constexpr bool used_as_headers_only{ true };
#else
#   define ASYNC_INLINE
    constexpr bool used_as_headers_only{ false };
#endif


#ifndef ASYNC_LIB_API
#   ifdef ASYNC_LIB_DYNAMIC
#       ifdef ASYNC_LIB_BUILD
#           define ASYNC_LIB_API __declspec(dllexport)
#       else
#           define ASYNC_LIB_API __declspec(dllimport)
#       endif // ASYNC_LIB_BUILD
#   else
#       define ASYNC_LIB_API
#   endif // ASYNC_LIB_DYNAMIC
#endif // !ASYNC_LIB_API


} // namespace async
