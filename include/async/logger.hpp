
#pragma once

#include <tuple>
#include <cwchar>
#include <string>
#include <exception>
#include <string_view>

#include <async\config.hpp>


namespace async
{
    using namespace std::literals::string_literals;
    using namespace std::literals::string_view_literals;
    
    class ASYNC_LIB_API logger
    {

    public:

        logger() noexcept = default;
        virtual ~logger() = default;

    public:

        virtual void message(std::wstring_view text) noexcept = 0;
        virtual void exception(std::exception_ptr except, std::wstring_view failed_action) noexcept = 0;
        virtual void enter_to_scope(std::wstring_view scope_name) noexcept = 0;
        virtual void leave_scope(std::wstring_view scope_name) noexcept = 0;
    };

} // namespace async


namespace async // impl
{
    namespace log_details
    {
        template<std::size_t> constexpr std::size_t max_dec_unsigned_chars_count{ 0 };
        template<> constexpr std::size_t max_dec_unsigned_chars_count<1>{  3 };
        template<> constexpr std::size_t max_dec_unsigned_chars_count<2>{  5 };
        template<> constexpr std::size_t max_dec_unsigned_chars_count<4>{ 10 };
        template<> constexpr std::size_t max_dec_unsigned_chars_count<8>{ 20 };
        template<class number_t> constexpr std::size_t max_dec_chars_count{ max_dec_unsigned_chars_count<sizeof(number_t)> + (std::is_signed_v<number_t> ? 1 : 0) };

        template<class number_t> constexpr const wchar_t* format{ L"" };
        template<> constexpr const wchar_t* format<  signed char>     { L"%hhi" };
        template<> constexpr const wchar_t* format<  signed short>    { L"%hi"  };
        template<> constexpr const wchar_t* format<  signed int>      { L"%i"   };
        template<> constexpr const wchar_t* format<  signed long long>{ L"%lli" };
        template<> constexpr const wchar_t* format<unsigned char>     { L"%hhu" };
        template<> constexpr const wchar_t* format<unsigned short>    { L"%hu"  };
        template<> constexpr const wchar_t* format<unsigned int>      { L"%u"   };
        template<> constexpr const wchar_t* format<unsigned long long>{ L"%llu" };

        template<class arg_t>
        inline void append(std::wstring& str, arg_t&& arg)
        {
            using decay_arg_t = std::decay_t<arg_t>;

            if constexpr ( std::is_same_v<decay_arg_t, char>
                || std::is_same_v<decay_arg_t, char*>
                || std::is_same_v<decay_arg_t, const char*>
                || std::is_same_v<decay_arg_t, std::string>
                || std::is_same_v<decay_arg_t, std::string_view>)
            {
                static_assert(false, "The ANSI chars not supported!");
            }
            else
            if constexpr (std::is_same_v<decay_arg_t, bool>)
            {
                str.append(arg ? L"yes"sv : L"no"sv);
            }
            else
            if constexpr (std::is_same_v<decay_arg_t, wchar_t>)
            {
                str.append(1, arg);
            }
            else
            if constexpr (std::is_same_v<decay_arg_t, wchar_t*>
                || std::is_same_v<decay_arg_t, const wchar_t*>)
            {
                str.append(arg);
            }
            else
            if constexpr (std::is_same_v<decay_arg_t, std::wstring_view>)
            {
                str.append(arg);
            }
            else
            if constexpr (std::is_same_v<decay_arg_t, std::wstring>)
            {
                str.append(arg);
            }
            else
            if constexpr (std::is_pointer_v<decay_arg_t>)
            {
                constexpr std::size_t hex_chars_count{ 2 * sizeof(arg) };
                const std::size_t old_size{ str.size() };
                str.resize(old_size + hex_chars_count);

                [[maybe_unused]] const std::size_t write_chars_count = std::swprintf(std::next(str.data(), old_size), (hex_chars_count + 1/*null-terminated*/), L"%p", arg);
                assert(write_chars_count == hex_chars_count);
            }
            else
            if constexpr (std::is_integral_v<decay_arg_t>)
            {
                const std::size_t old_size{ str.size() };
                str.resize(old_size + max_dec_chars_count<decay_arg_t>);

                const std::size_t new_size{ old_size + std::swprintf(std::next(str.data(), old_size), (max_dec_chars_count<decay_arg_t> +1/*null-terminated*/), format<decay_arg_t>, arg) };
                assert(new_size > old_size);

                str.resize(new_size);
            }
            else
            {
                static_assert(false, "Not supported type the argument");
            }
        }
        
        template<class... args_t>
        inline std::wstring make_string(args_t&&... args)
        {
            std::wstring result{};
            [[maybe_unused]] const bool _[]{ (append(result, std::forward<args_t>(args)), true)... };
            return result;
        }


        template<class... args_t>
        struct args_api
        {
            static constexpr std::size_t count{ sizeof...(args_t) };
            static_assert(count > 0);

            using decay_first_arg_t = std::decay_t<std::tuple_element_t<0, std::tuple<args_t...>>>;

            static constexpr bool one_arg__str{ count == 1 && std::is_same_v<decay_first_arg_t, std::wstring> };

            static constexpr bool one_arg__str_or_view{ count == 1 && (std::is_same_v<decay_first_arg_t, std::wstring> || std::is_same_v<decay_first_arg_t, std::wstring_view>) };

            static constexpr auto&& move_first_arg(args_t&&... args) noexcept
            {
                return std::move(*std::get<0>(std::tuple{ &args... }));
            }
        };

    } // namespace log_details

    template<
        class... args_t,
        class args_api = typename log_details::args_api<args_t...>
    >
    inline void log_msg(logger* log, args_t&&... args) noexcept(args_api::one_arg__str_or_view)
    {
        if (log)
        {
            if constexpr (args_api::one_arg__str_or_view)
            {
                log->message(std::forward<args_t>(args)...);
            }
            else
            {
                log->message(log_details::make_string(std::forward<args_t>(args)...));
            }
        }
    }

    template<
        class... args_t,
        class args_api = typename log_details::args_api<args_t...>
    >
    inline void log_except(logger* log, std::exception_ptr except, args_t&&... args) noexcept(args_api::one_arg__str_or_view)
    {
        if (log)
        {
            if constexpr (args_api::one_arg__str_or_view)
            {
                log->exception(except, std::forward<args_t>(args)...);
            }
            else
            {
                log->exception(except, log_details::make_string(std::forward<args_t>(args)...));
            }
        }
    }

    class log_scope
    {
    public:

        template<
            class... args_t,
            class args_api = typename log_details::args_api<args_t...>
        >
        inline log_scope(logger* log, args_t&&... args) noexcept(args_api::one_arg__str)
            : m_log{ log }
            , m_name{}
        {
            if (m_log)
            {
                if constexpr (args_api::one_arg__str)
                {
                    static_assert(std::is_nothrow_move_assignable_v<std::wstring>);
                    m_name = args_api::move_first_arg(std::forward<args_t>(args)...);
                }
                else
                {
                    m_name = log_details::make_string(std::forward<args_t>(args)...);
                }
                
                m_log->enter_to_scope(m_name);
            }
        }

        inline log_scope(log_scope&& other) noexcept
            : m_log{ other.m_log }
            , m_name{ std::move(other.m_name) }
        {
            other.m_log = nullptr;
        }
        
        inline ~log_scope()
        {
            if (m_log)
                m_log->leave_scope(m_name);
        }

    private:

        logger* m_log;
        std::wstring m_name;
    };

} // namespace async
