
#pragma once

#include <iostream>

#include <async\config.hpp>
#include <async\logger.hpp>


namespace async
{

    class logger_wostream : public logger
    {

    public:

        inline logger_wostream(std::wostream& stream) noexcept;

    public:

        virtual void message(std::wstring_view text) noexcept override;

        virtual void exception(std::exception_ptr except, std::wstring_view failed_action) noexcept override;

        virtual void enter_to_scope(std::wstring_view scope_name) noexcept override;

        virtual void leave_scope(std::wstring_view scope_name) noexcept override;

    private:

        std::size_t print_head();

        std::wostream& print_multiline(std::size_t head_size, std::wstring_view text);

    private:

        std::wostream& m_str_ref;

#ifndef ASYNC_LIBRARY_USED_AS_NOT_HEADERS_ONLY
#else
        static thread_local std::size_t m_scope_level;
#endif
    };

} // namespace async


#ifndef ASYNC_LIBRARY_USED_AS_NOT_HEADERS_ONLY
#   include <async\logger_wostream_impl.hpp>
#endif
