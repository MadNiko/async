
#pragma once


namespace async
{

#ifndef ASYNC_LIBRARY_USED_AS_NOT_HEADERS_ONLY
    constexpr bool used_as_headers_only{ true };
#else
    constexpr bool used_as_headers_only{ false };
#endif


} // namespace async
