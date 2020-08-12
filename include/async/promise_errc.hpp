
#pragma once


#include <string>
#include <stdexcept>
#include <string_view>
#include <system_error>


namespace async
{
	enum class promise_errc
	{
		deadlock,
		broken_promise,
		promise_already_satisfied,
		next_step_already_retrieved,
		value_already_retrieved,
		no_state,
	};

	const std::error_category& error_category() noexcept;
	std::error_code make_error_code(promise_errc errc) noexcept;
	constexpr std::string_view to_static_message_view(int errc) noexcept;

	class promise_error : public std::logic_error
	{
	public:

		inline explicit promise_error::promise_error(promise_errc errc)
			: std::logic_error("")
			, m_code{ make_error_code(errc) }
		{}
		[[nodiscard]] inline std::error_code promise_error::code() const noexcept
		{
			return m_code;
		}
		[[nodiscard]] virtual const char* promise_error::what() const override
		{
			return to_static_message_view(m_code.value()).data();
		}

	private:

		std::error_code m_code;
	};

	constexpr std::string_view to_static_message_view(int errc) noexcept
	{
		using namespace std::string_view_literals;
		switch (static_cast<promise_errc>(errc))
		{
		case promise_errc::deadlock:                    return "deadlock"sv;
		case promise_errc::broken_promise:              return "broken promise"sv;
		case promise_errc::promise_already_satisfied:   return "promise already satisfied"sv;
		case promise_errc::next_step_already_retrieved: return "next step already retrieved"sv;
		case promise_errc::value_already_retrieved:     return "value already retrieved"sv;
		case promise_errc::no_state:                    return "no state"sv;
		default:                                        return ""sv;
		}
		__assume(false);
	}
	inline std::error_code make_error_code(promise_errc errc) noexcept
	{
		return std::error_code{ static_cast<int>(errc), error_category() };
	}
	inline const std::error_category& error_category() noexcept
	{
		static const class error_category_promise : public std::error_category
		{
			virtual const char* name() const noexcept override
			{
				return "async-promise";
			}

			virtual std::string message(int code_value) const override
			{
				using namespace std::string_view_literals;
				const std::string_view msg{ to_static_message_view(code_value) };
				return std::string{ msg.empty() ? "<unknown-exception>"sv : msg };
			}

		} category_instance{};

		return category_instance;
	}

} // namespace async


namespace std
{
	template<>
	struct is_error_code_enum<async::promise_errc> : std::true_type {};

} //namespace std
