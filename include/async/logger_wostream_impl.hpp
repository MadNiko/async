
#pragma once

#include <ctime>
#include <thread>
#include <cassert>

#include <async\logger_wostream.hpp>


namespace async
{

#ifndef ASYNC_LIBRARY_USED_AS_NOT_HEADERS_ONLY
#else
    thread_local std::size_t logger_wostream::m_scope_level{ 0 };
#endif

    inline logger_wostream::logger_wostream(std::wostream& stream) noexcept
        : logger{}
        , m_str_ref{ stream }
    {}

    inline void logger_wostream::message(std::wstring_view text) noexcept
    {
        try
        {
            print_multiline(print_head(), text) << std::endl << std::flush;
        }
        catch (...) {}
    }

    inline void logger_wostream::exception(std::exception_ptr except, std::wstring_view failed_action) noexcept
    {
        (void)except;
        try
        {
            print_head();
            m_str_ref << L"[except]"sv << failed_action << std::endl;
        }
        catch (...) {}
    }

    inline void logger_wostream::enter_to_scope(std::wstring_view scope_name) noexcept
    {
        try
        {
            print_head();
            m_str_ref << (!std::uncaught_exceptions() ? L"--->"sv : L"exc>"sv) << scope_name << std::endl << std::flush;
        }
        catch (...) {}

#ifndef ASYNC_LIBRARY_USED_AS_NOT_HEADERS_ONLY
#else
            m_scope_level += 1;
#endif
    }

    inline void logger_wostream::leave_scope(std::wstring_view scope_name) noexcept
    {
#ifndef ASYNC_LIBRARY_USED_AS_NOT_HEADERS_ONLY
#else
            m_scope_level -= 1;
#endif

        try
        {
            print_head();
            m_str_ref << (!std::uncaught_exceptions() ? L"<---"sv : L"<exc"sv) << scope_name << std::endl << std::flush;
        }
        catch (...) {}
    }

    inline std::size_t logger_wostream::print_head()
    {
        const std::time_t t(std::time(nullptr));
        const std::tm tm(*std::localtime(&t));

        wchar_t buffer[22] = { L'\0' };
        const std::size_t head_size = std::swprintf(
            buffer, std::size(buffer),
            L"[%.2d.%.2d.%.2d] [%.2d:%.2d:%.2d]",
            tm.tm_year % 100,
            tm.tm_mon + 1,
            tm.tm_mday,
            tm.tm_hour,
            tm.tm_min,
            tm.tm_sec % 60);

        m_str_ref << buffer;

#ifndef ASYNC_LIBRARY_USED_AS_NOT_HEADERS_ONLY
        m_str_ref << L' ';
        return head_size;
#else
        const std::size_t space_size{ 4 * m_scope_level + 1 };
        const std::streamsize save_width = m_str_ref.width(space_size);
        m_str_ref << L' ';
        m_str_ref.width(save_width);
        return head_size + space_size;
#endif
    }

    inline std::wostream& logger_wostream::print_multiline(std::size_t head_size, std::wstring_view text)
    {
        constexpr std::wstring_view new_line_markers{ L"\r\n"sv };

        std::size_t pos{ 0 };
        std::size_t end_pos{ text.find_first_of(new_line_markers, pos) };

        while (end_pos != text.npos)
        {
            m_str_ref << text.substr(pos, end_pos - pos);
            pos = text.find_first_not_of(new_line_markers, end_pos + 1);
            if (pos == text.npos)
                break;

            end_pos = text.find_first_of(new_line_markers, pos + 1);

            m_str_ref.width(head_size);
            m_str_ref << std::endl << L' ';
        }

        if (pos != text.npos)
            m_str_ref << text.substr(pos);

        return m_str_ref;
    }

} // namespace async
