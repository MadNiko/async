
#pragma once

#include <tuple>
#include <cwchar>
#include <string>
#include <exception>
#include <string_view>


namespace async
{
    using namespace std::literals::string_literals;
    using namespace std::literals::string_view_literals;
    
    class logger
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

        template<class number_t> constexpr wchar_t* const format{ L"" };
        template<> constexpr wchar_t* const format<  signed char>     { L"%hhi" };
        template<> constexpr wchar_t* const format<  signed short>    { L"%hi"  };
        template<> constexpr wchar_t* const format<  signed int>      { L"%i"   };
        template<> constexpr wchar_t* const format<  signed long long>{ L"%lli" };
        template<> constexpr wchar_t* const format<unsigned char>     { L"%hhu" };
        template<> constexpr wchar_t* const format<unsigned short>    { L"%hu"  };
        template<> constexpr wchar_t* const format<unsigned int>      { L"%u"   };
        template<> constexpr wchar_t* const format<unsigned long long>{ L"%llu" };

        template<class arg_t>
        inline std::wstring append(std::wstring&& string, arg_t&& arg)
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
                return std::move(string.append(arg ? L"yes"sv : L"no"sv));
            }
            else
            if constexpr (std::is_same_v<decay_arg_t, wchar_t>)
            {
                return std::move(string.append(1, arg));
            }
            else
            if constexpr (std::is_same_v<decay_arg_t, wchar_t*>
                || std::is_same_v<decay_arg_t, const wchar_t*>)
            {
                return std::move(string.append(arg));
            }
            else
            if constexpr (std::is_same_v<decay_arg_t, std::wstring_view>)
            {
                return std::move(string.append(arg));
            }
            else
            if constexpr (std::is_same_v<decay_arg_t, std::wstring>)
            {
                return std::move(string.append(arg));
            }
            else
            if constexpr (std::is_pointer_v<decay_arg_t>)
            {
                constexpr std::size_t hex_chars_count{ 2 * sizeof(arg) };
                const std::size_t old_size{ string.size() };
                string.resize(old_size + hex_chars_count);

                [[maybe_unused]] const std::size_t write_chars_count = std::swprintf(std::next(string.data(), old_size), (hex_chars_count + 1/*null-terminated*/), L"%p", arg);
                assert(write_chars_count == hex_chars_count);

                return std::move(string);
            }
            else
            if constexpr (std::is_integral_v<decay_arg_t>)
            {
                const std::size_t old_size{ string.size() };
                string.resize(old_size + max_dec_chars_count<decay_arg_t>);

                const std::size_t new_size{ old_size + std::swprintf(std::next(string.data(), old_size), (max_dec_chars_count<decay_arg_t> +1/*null-terminated*/), format<decay_arg_t>, arg) };
                assert(new_size > old_size);

                string.resize(new_size);

                return std::move(string);
            }
            else
            {
                static_assert(false, "Not supported type the argument");
            }
        }
        template<class arg_t>
        inline std::wstring append_ex(std::wstring&& string, arg_t&& arg)
        {
            return append(std::move(string), std::forward<arg_t>(arg));
        }
        template<class first_arg_t, class... next_args_t>
        inline std::wstring append_ex(std::wstring&& string, first_arg_t&& first_arg, next_args_t&&... next_args)
        {
            return append_ex(append(std::move(string), std::forward<first_arg_t>(first_arg)), std::forward<next_args_t>(next_args)...);
        }
        
        template<class... args_t>
        inline std::wstring make_string(args_t&&... args)
        {
            return append_ex(L""s, std::forward<args_t>(args)...);
        }

        template<class... args_t>
        struct args_api
        {
            static constexpr std::size_t count{ sizeof...(args_t) };
            static_assert(count > 0);

            using decay_first_arg_t = std::decay_t<std::tuple_element_t<0, std::tuple<args_t...>>>;

            static inline decay_first_arg_t move_first_arg(args_t&&... args) noexcept(std::is_nothrow_move_assignable_v<decay_first_arg_t>)
            {
                std::tuple<args_t*...> args_ptrs{ &args... };
                return std::move(*std::get<0>(args_ptrs));
            }

            template<class... cmps_t>
            static constexpr bool is_only_one_and_as() noexcept
            {
                if constexpr (count != 1)
                {
                    return false;
                }
                else
                {
                    bool result{ false };
                    
                    [[maybe_unused]] const bool flags[] { ((std::is_same_v<decay_first_arg_t, std::decay_t<cmps_t>> ? (result = true) : false))... };
                    
                    return result;
                }
            }
        };

    } // namespace log_details

    template<class... args_t>
    void log_msg(logger* log, args_t&&...args) noexcept(log_details::args_api<args_t...>::is_only_one_and_as<std::wstring, std::wstring_view>())
    {
        if (log)
        {
            if constexpr (log_details::args_api<args_t...>::is_only_one_and_as<std::wstring, std::wstring_view>())
            {
                log->message(std::forward<args_t>(args)...);
            }
            else
            {
                log->message(log_details::make_string(std::forward<args_t>(args)...));
            }
        }
    }

    template<class... args_t>
    void log_except(logger* log, std::exception_ptr except, args_t&&...args) noexcept(log_details::args_api<args_t...>::is_only_one_and_as<std::wstring, std::wstring_view>())
    {
        if (log)
        {
            if constexpr (log_details::args_api<args_t...>::is_only_one_and_as<std::wstring, std::wstring_view>())
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

        template<class... args_t>
        log_scope(logger* log, args_t&&...args) noexcept(log_details::args_api<args_t...>::is_only_one_and_as<std::wstring>())
            : m_log{ log }
            , m_name{}
        {
            if (m_log)
            {
                if constexpr (log_details::args_api<args_t...>::is_only_one_and_as<std::wstring>())
                {
                    static_assert(std::is_nothrow_move_assignable_v<std::wstring>);
                    m_name = log_details::args_api<args_t...>::move_first_arg(std::forward<args_t>(args)...);
                }
                else
                {
                    m_name = log_details::make_string(std::forward<args_t>(args)...);
                }
                
                m_log->enter_to_scope(m_name);
            }
        }

        log_scope(log_scope&& other) noexcept
            : m_log{ other.m_log }
            , m_name{ std::move(other.m_name) }
        {
            other.m_log = nullptr;
        }
        
        ~log_scope()
        {
            if (m_log)
                m_log->leave_scope(m_name);
        }

    private:

        logger* m_log;
        std::wstring m_name;
    };

} // namespace async


//namespace async
//{
//    inline namespace std_ext
//    {
//        using std::string_literals::operator ""s;
//        using std::string_view_literals::operator ""sv;
//
//        template<
//            class _elem,
//            class _traits = std::char_traits<_elem>,
//            class _alloc = std::allocator<_elem>
//        >
//        std::basic_string<_elem, _traits, _alloc> operator+(std::basic_string<_elem, _traits, _alloc>&& _left, std::basic_string_view<_elem, _traits> _right)
//        {
//            return std::move(_left.append(_right));
//        }
//
//        template<
//            class _elem,
//            class _traits = std::char_traits<_elem>,
//            class _alloc = std::allocator<_elem>
//        >
//        inline std::basic_string<_elem, _traits, _alloc> operator+(const std::basic_string<_elem, _traits, _alloc>& _left, std::basic_string_view<_elem, _traits> _right)
//        {
//            std::basic_string<_elem, _traits, _alloc> result;
//            result.reserve(_left.length() + _right.length());
//            return std::move(result.append(_left).append(_right));
//        }
//
//        template<
//            class _elem,
//            class _traits = std::char_traits<_elem>,
//            class _alloc = std::allocator<_elem>
//        >
//        inline std::basic_string<_elem, _traits, _alloc> operator+(std::basic_string_view<_elem, _traits> _left, std::basic_string<_elem, _traits, _alloc>&& _right)
//        {
//            return _right.insert(0, _left), std::move(_right);
//        }
//
//        template<
//            class _elem,
//            class _traits = std::char_traits<_elem>,
//            class _alloc = std::allocator<_elem>
//        >
//        inline std::basic_string<_elem, _traits, _alloc> operator+(std::basic_string_view<_elem, _traits> _left, const std::basic_string<_elem, _traits, _alloc>& _right)
//        {
//            std::basic_string<_elem, _traits, _alloc> result;
//            result.reserve(_left.length() + _right.length());
//            return std::move(result.append(_left).append(_right));
//        }
//
//        template<
//            class _elem,
//            class _traits = std::char_traits<_elem>,
//            class _alloc = std::allocator<_elem>
//        >
//        inline std::basic_string<_elem, _traits, _alloc> operator+(std::basic_string_view<_elem, _traits> _left, std::basic_string_view<_elem, _traits> _right)
//        {
//            std::basic_string<_elem, _traits, _alloc> result;
//            result.reserve(_left.length() + _right.length());
//            return std::move(result.append(_left).append(_right));
//        }
//
//        template<
//            class _elem,
//            class _traits = std::char_traits<_elem>,
//            class _alloc = std::allocator<_elem>
//        >
//        inline std::basic_string<_elem, _traits, _alloc> operator+(std::basic_string_view<_elem, _traits> _left, const _elem* _right)
//        {
//            return _left + std::basic_string_view<_elem, _traits>(_right);
//        }
//
//        template<
//            class _elem,
//            class _traits = std::char_traits<_elem>,
//            class _alloc = std::allocator<_elem>
//        >
//        inline std::basic_string<_elem, _traits, _alloc> operator+(const _elem* _left, std::basic_string_view<_elem, _traits> _right)
//        {
//            return std::basic_string_view<_elem, _traits>(_left) + _right;
//        }
//
//        template<
//            class _elem,
//            class _traits = std::char_traits<_elem>,
//            class _alloc = std::allocator<_elem>
//        >
//        inline std::basic_string<_elem, _traits, _alloc> operator+(std::basic_string_view<_elem, _traits> _left, _elem _right)
//        {
//            std::basic_string<_elem, _traits, _alloc> result;
//            result.reserve(_left.length() + 1);
//            return std::move(result.append(_left).append(1, _right));
//        }
//
//        template<
//            class _elem,
//            class _traits = std::char_traits<_elem>,
//            class _alloc = std::allocator<_elem>
//        >
//        inline std::basic_string<_elem, _traits, _alloc> operator+(_elem _left, std::basic_string_view<_elem, _traits> _right)
//        {
//            std::basic_string<_elem, _traits, _alloc> result;
//            result.reserve(_right.length() + 1);
//            return std::move(result.append(1, _left).append(_right));
//        }
//
//    } // inline namespace std_ext
//
//} // namespace async
