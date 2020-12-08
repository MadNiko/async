
#pragma once


#include <tuple>
#include <atomic>
#include <string>
#include <memory>
#include <cstdint>
#include <variant>
#include <optional>
#include <exception>
#include <functional>

#include <async\pool.hpp>


namespace async
{
	template<class _Result>
	class promise;

	template<class _Result>
	class multi_promise;

	template<class _Value>
	class value_t;

	class manager;

	template<class _Result, class _Arg>
	struct function
	{
		using type_1 = std::function<        _Result (_Arg)>;
		using type_2 = std::function<promise<_Result>(_Arg)>;
	};
	template<class _Result>
	struct function<_Result, void>
	{
		using type_1 = std::function<        _Result ()>;
		using type_2 = std::function<promise<_Result>()>;
	};

	template<class _Result, class _Arg>
	using function_1_t = typename function<_Result, _Arg>::type_1;

	template<class _Result, class _Arg>
	using function_2_t = typename function<_Result, _Arg>::type_2;

	template<class _Result, class _Arg>
	using function_variant_t = std::variant<function_1_t<_Result, _Arg>, function_2_t<_Result, _Arg>>;

	template<class _Result>             using task_t    = function_variant_t<_Result, void>;
	template<class _Result, class _Arg> using then_t    = function_variant_t<_Result, value_t<_Arg>>;
	template<class _Result, class _Arg> using success_t = function_variant_t<_Result, _Arg>;
	template<class _Result>             using reject_t  = function_variant_t<_Result, std::exception_ptr>;

	using finally_t = std::function<void()>;

} // namespace async


namespace async::details
{
	enum class result_state_t
	{
		none = 0,
		value,
		next_step,
		executable
	};
	static inline std::string to_string(result_state_t state)
	{
		using namespace std::string_literals;
		switch (state)
		{
		case result_state_t::none:       return "none"s;
		case result_state_t::value:      return "value"s;
		case result_state_t::next_step:  return "next_step"s;
		case result_state_t::executable: return "executable"s;
		default:                         return "<unknown>"s;
		}
	}

	using task_variant_t = std::variant<pool::task_t, pool::more_tasks_t>;

	template<class _Value>
	struct result_t
	{
		std::atomic<result_state_t> state = result_state_t::none;
		value_t<_Value> value = {};
		
		task_variant_t next_task_v;
	};


	template<class _Value>
	struct prom_data_t
	{
		pool_ptr pool;

		result_t<_Value> result;

		std::wstring log_ctx;
	};
	template<class _Value>
	using prom_data_ptr = std::shared_ptr<prom_data_t<_Value>>;


} // namespace async::details
