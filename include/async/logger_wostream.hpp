
#pragma once

#include <iostream>

#include <async\logger.hpp>


namespace async
{

    class ASYNC_LIB_API logger_wostream : public logger
    {

    public:

        logger_wostream(std::wostream& stream) noexcept;

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
    };

} // namespace async


#ifdef ASYNC_LIB_HEADERS_ONLY
#   include <async\logger_wostream_impl.hpp>
#endif
