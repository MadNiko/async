
#pragma once

#include <string>
#include <cassert>
#include <string_view>

#include <async\logger.hpp>
#include <async\promise_types.hpp>
#include <async\value_or_promise.hpp>


namespace async::details
{
	template<class _Result>
	struct traits_value_or_promise_base
	{
		template<class _Arg, std::enable_if_t<!std::is_void_v<_Arg>, int> = 0>
		static inline value_or_promise_t<_Result> apply(const std::function<promise<_Result>(_Arg)>& functor, value_t<_Arg> arg)
		{
			assert(arg.has_value());

			value_or_promise_t<_Result> result{};
			result.set_promise(functor(std::move(arg).get_value()));
			return result;
		}

		template<class _Arg, std::enable_if_t<!std::is_void_v<_Arg>, int> = 0>
		static inline value_or_promise_t<_Result> apply(const std::function<promise<_Result>(_Arg)>& functor, _Arg arg)
		{
			value_or_promise_t<_Result> result{};
			result.set_promise(functor(std::move(arg)));
			return result;
		}

		template<class _Arg, std::enable_if_t<std::is_void_v<_Arg>, int> = 0>
		static inline value_or_promise_t<_Result> apply(const std::function<promise<_Result>()>& functor, value_t<_Arg> arg)
		{
			assert(arg.has_value());
			arg.get_value();

			value_or_promise_t<_Result> result{};
			result.set_promise(functor());
			return result;
		}
	};

	template<class _Result>
	struct traits_value_or_promise : traits_value_or_promise_base<_Result>
	{
		using traits_value_or_promise_base<_Result>::apply;

		template<class _Arg, std::enable_if_t<!std::is_void_v<_Arg>, int> = 0>
		static inline value_or_promise_t<_Result> apply(const std::function<_Result(_Arg)>& functor, value_t<_Arg> arg)
		{
			assert(arg.has_value());

			value_or_promise_t<_Result> result{};
			result.set_value(functor(std::move(arg).get_value()));
			return result;
		}

		template<class _Arg, std::enable_if_t<std::is_void_v<_Arg>, int> = 0>
		static inline value_or_promise_t<_Result> apply(const std::function<_Result()>& functor, value_t<_Arg> arg)
		{
			assert(arg.has_value());
			arg.get_value();

			value_or_promise_t<_Result> result{};
			result.set_value(functor());
			return result;
		}

		template<class _Arg, std::enable_if_t<!std::is_void_v<_Arg>, int> = 0>
		static inline value_or_promise_t<_Result> apply(const std::function<_Result(_Arg)>& functor, _Arg arg)
		{
			value_or_promise_t<_Result> result{};
			result.set_value(functor(std::move(arg)));
			return result;
		}
	};

	template<>
	struct traits_value_or_promise<void> : traits_value_or_promise_base<void>
	{
		using _Result = void;
		using traits_value_or_promise_base<_Result>::apply;

		template<class _Arg, std::enable_if_t<!std::is_void_v<_Arg>, int> = 0>
		static inline value_or_promise_t<_Result> apply(const std::function<_Result(_Arg)>& functor, value_t<_Arg> arg)
		{
			assert(arg.has_value());

			functor(std::move(arg).get_value());
			value_or_promise_t<_Result> result{};
			result.set_value();
			return result;
		}

		template<class _Arg, std::enable_if_t<std::is_void_v<_Arg>, int> = 0>
		static inline value_or_promise_t<_Result> apply(const std::function<_Result()>& functor, value_t<_Arg> arg)
		{
			assert(arg.has_value());
			arg.get_value();

			functor();
			value_or_promise_t<_Result> result{};
			result.set_value();
			return result;
		}

		template<class _Arg, std::enable_if_t<!std::is_void_v<_Arg>, int> = 0>
		static inline value_or_promise_t<_Result> apply(const std::function<_Result(_Arg)>& functor, _Arg arg)
		{
			functor(std::move(arg));
			value_or_promise_t<_Result> result{};
			result.set_value();
			return result;
		}
	};

} // namespace async::details

namespace async::details
{
	using namespace std::string_view_literals;

	constexpr std::wstring::value_type log_node_separator{ L'/' };
	constexpr std::wstring::value_type log_node_replace_separator{ L'\\' };

	inline std::wstring head_title(std::wstring&& title)
	{
		title.reserve(title.length() + 2);
		title.insert(title.begin(), L'[');
		title.insert(title.end(), L']');
		return title;
	}
	inline std::wstring head_title(std::wstring_view title_view)
	{
		std::wstring title;
		title.reserve(title_view.length() + 2);
		title += L'[';
		title += title_view;
		title += L']';
		return title;
	}

	inline std::wstring& add_details_log_ctx(logger* log, std::wstring& log_ctx, std::wstring_view details_view)
	{
		assert(!details_view.empty());

        if (log)
        {
            log_ctx.reserve(log_ctx.size() + details_view.size() + 2);

            if (log_ctx.empty() || log_ctx.back() != L'}')
                log_ctx += L'{';
            else
                log_ctx.back() = L'|';

            log_ctx += details_view;
            log_ctx += L'}';
        }

		return log_ctx;
	}
	inline std::wstring add_details_log_ctx(logger* log, std::wstring_view log_ctx_view, std::wstring_view details_view)
	{
		std::wstring log_ctx;

        if (log)
        {
            log_ctx.reserve(log_ctx_view.size() + details_view.size() + 2);
            log_ctx += log_ctx_view;
        }

		return std::move(add_details_log_ctx(log, log_ctx, details_view));
	}
	[[nodiscard]] inline std::wstring normalize_log_ctx(std::wstring&& log_ctx)
	{
		for (std::wstring::value_type& ch : log_ctx)
		{
			if (ch == log_node_separator)
				ch = log_node_replace_separator;
		}
		
		return std::move(log_ctx);
	}
	[[nodiscard]] inline std::wstring normalize_log_ctx(std::wstring_view log_ctx_view)
	{
		return normalize_log_ctx(std::wstring(log_ctx_view));
	}
	[[nodiscard]] inline std::wstring normalize_log_ctx(logger* log, std::wstring&& log_ctx, std::wstring_view details_view)
	{
		log_ctx = normalize_log_ctx(std::move(log_ctx));
		add_details_log_ctx(log, log_ctx, details_view);

		return log_ctx;
	}
	inline std::wstring& append_log_ctx(std::wstring& log_ctx, std::wstring_view log_ctx_next_node_view)
	{
		using namespace std::string_view_literals;

		log_ctx.reserve(log_ctx.size() + 1/*log_node_separator*/ + log_ctx_next_node_view.size());
		log_ctx += log_node_separator;
		log_ctx += log_ctx_next_node_view;

		return log_ctx;
	}
	inline std::wstring& append_log_ctx(logger* log, std::wstring& log_ctx, std::wstring_view log_ctx_next_node_view, std::wstring_view details_view)
	{
		return add_details_log_ctx(log, append_log_ctx(log_ctx, log_ctx_next_node_view), details_view);
	}

	template<class _Value>
	inline prom_data_ptr<_Value> take_data_of_promise(promise<_Value>& promise)
	{
		return promise.take_data();
	}

	inline void add_task_in_pool(pool::ctx_t pool_ctx, pool& pool_impl, task_variant_t&& task_v) noexcept
	{
		if (pool::task_t* const one_task = std::get_if<pool::task_t>(&task_v))
		{
			pool_impl.add_task(std::move(pool_ctx), std::move(*one_task));
		}
		else
		{
			pool::more_tasks_t& more_tasks{ std::get<pool::more_tasks_t>(task_v) };
			assert(!more_tasks.empty());

			auto it_task{ begin(more_tasks) };
			const auto it_task_end{ end(more_tasks) };

			pool_impl.add_task(std::move(pool_ctx), std::move(*it_task++));

			while (it_task != it_task_end)
			{
				pool_impl.add_task(pool::unknown_ctx, std::move(*it_task++));
			}
		}
	}

	template<class _Value>
	struct api
	{
		static void set_result_impl(pool::ctx_t pool_ctx, pool& pool, result_t<_Value>& result, value_t<_Value>&& value)
		{
			using namespace std::literals;

			[[maybe_unused]] const result_state_t _debug_state{ result.state.load() };
			assert(_debug_state == result_state_t::none || _debug_state == result_state_t::next_step);
			assert(!result.value.is_established());
			assert(value.is_established());

			swap(result.value, value);

			assert(result.value.is_established());

			result_state_t expected_state{ result_state_t::none };
			if (result.state.compare_exchange_strong(expected_state, result_state_t::value))
			{
				//Ok: new_state = result_state_t::value;
			}
			else
			if (expected_state == result_state_t::next_step &&
				result.state.compare_exchange_strong(expected_state, result_state_t::executable))
			{
				//Ok: new_state = result_state_t::executable;

				details::add_task_in_pool(std::move(pool_ctx), pool, std::move(result.next_task_v));
			}
			else
			{
				assert(expected_state == result_state_t::value || expected_state == result_state_t::executable);
				throw promise_error{ expected_state == result_state_t::value ? promise_errc::value_already_retrieved : promise_errc::promise_already_satisfied };
			}
		}

	public:

		static inline value_or_promise_t<_Value> apply_task(logger* log, std::wstring_view log_ctx_view, const task_t<_Value>& tsk_v) noexcept
		{
			using _Arg = void;

			try
			{
                const log_scope log_scope_guard{ log, L'[', log_ctx_view, L']' };
				
				value_t<_Arg> arg{};
				arg.set_value();

				if (std::get_if<function_1_t<_Value, _Arg>>(&tsk_v))
					return traits_value_or_promise<_Value>::apply<_Arg>(std::get<function_1_t<_Value, _Arg>>(tsk_v), std::move(arg));
				else
					return traits_value_or_promise<_Value>::apply<_Arg>(std::get<function_2_t<_Value, _Arg>>(tsk_v), std::move(arg));
			}
			catch (...)
			{
                // tarce
                log_except(log, std::current_exception(), L'[', log_ctx_view, L']');

				value_or_promise_t<_Value> result{};
				result.set_except(std::current_exception());
				return result;
			}
			__assume(false);
		}

		template<class _Arg>
		static inline value_or_promise_t<_Value> apply_then(logger* log, std::wstring& log_ctx, const then_t<_Value, _Arg>& thn, value_t<_Arg>& arg) noexcept
		{
			try
			{
                const log_scope log_scope_guard{ log, L'[', log_ctx_view, L']' };
				
				if (std::get_if<function_1_t<_Value, value_t<_Arg>>>(&thn))
					return traits_value_or_promise<_Value>::apply<value_t<_Arg>>(std::get<function_1_t<_Value, value_t<_Arg>>>(thn), std::move(arg));
				else
					return traits_value_or_promise<_Value>::apply<value_t<_Arg>>(std::get<function_2_t<_Value, value_t<_Arg>>>(thn), std::move(arg));
			}
			catch (...)
			{
				::logger::log_exception(std::current_exception(), ::logger::level::trace, head_title(log_ctx));

				value_or_promise_t<_Value> result{};
				result.set_except(std::current_exception());
				return result;
			}
			__assume(false);
		}

		template<class _Arg>
		static inline value_or_promise_t<_Value> apply_success(logger* log, std::wstring_view log_ctx_view, const success_t<_Value, _Arg>& scss, value_t<_Arg>& arg) noexcept
		{
			assert(arg.has_value());

			try
			{
                const log_scope log_scope_guard{ log, L'[', log_ctx_view, L']' };

				if (std::get_if<function_1_t<_Value, _Arg>>(&scss))
					return traits_value_or_promise<_Value>::apply<_Arg>(std::get<function_1_t<_Value, _Arg>>(scss), std::move(arg));
				else
					return traits_value_or_promise<_Value>::apply<_Arg>(std::get<function_2_t<_Value, _Arg>>(scss), std::move(arg));
			}
			catch (...)
			{
                // trace
                log_except(log, std::current_exception(), L'[', log_ctx_view, L']');

				value_or_promise_t<_Value> result{};
				result.set_except(std::current_exception());
				return result;
			}
			__assume(false);
		}

		static inline value_or_promise_t<_Value> apply_reject(logger* log, std::wstring_view log_ctx_view, const reject_t<_Value>& rjct, std::exception_ptr except) noexcept
		{
			try
			{
                const log_scope log_scope_guard{ log, L'[', log_ctx_view, L']' };
				
				if (std::get_if<function_1_t<_Value, std::exception_ptr>>(&rjct))
					return traits_value_or_promise<_Value>::apply<std::exception_ptr>(std::get<function_1_t<_Value, std::exception_ptr>>(rjct), except);
				else
					return traits_value_or_promise<_Value>::apply<std::exception_ptr>(std::get<function_2_t<_Value, std::exception_ptr>>(rjct), except);
			}
			catch (...)
			{
                // trace
                log_except(log, std::current_exception(), L'[', log_ctx_view, L']');

				value_or_promise_t<_Value> result{};
				result.set_except(std::current_exception());
				return result;
			}
			__assume(false);
		}

	public:

		static void bind_next_step(pool::ctx_t pool_ctx, result_t<_Value>& result, pool& pool, pool::task_t&& next_task)
		{
			using namespace std::literals;

			assert(next_task);

			if (pool::task_t* const result_next_task = std::get_if<pool::task_t>(&result.next_task_v))
			{
				[[maybe_unused]] const result_state_t _debug_state{ result.state.load() };
				assert(_debug_state == result_state_t::none || _debug_state == result_state_t::value);
				assert(!*result_next_task);

				result_next_task->swap(next_task);

				result_state_t expected_state{ result_state_t::none };
				if (result.state.compare_exchange_strong(expected_state, result_state_t::next_step))
				{
					//Ok: new_state = result_state_t::next_step;
				}
				else
					if (expected_state == result_state_t::value &&
						result.state.compare_exchange_strong(expected_state, result_state_t::executable))
					{
						//Ok: new_state = result_state_t::executable;

						details::add_task_in_pool(std::move(pool_ctx), pool, std::move(result.next_task_v));
						assert(!std::get<pool::task_t>(result.next_task_v));
					}
					else
					{
						assert(expected_state == result_state_t::next_step || expected_state == result_state_t::executable);
						throw promise_error{ expected_state == result_state_t::next_step ? promise_errc::next_step_already_retrieved : promise_errc::promise_already_satisfied };
					}
			}
			else
			{
				pool::more_tasks_t& more_tasks{ std::get<pool::more_tasks_t>(result.next_task_v) };

				//under mutex...
				more_tasks.push_back(std::move(next_task));

				result_state_t expected_state{ result_state_t::none };
				if (result.state.compare_exchange_strong(expected_state, result_state_t::next_step))
				{
					//Ok: new_state = result_state_t::next_step;
				}
				else
					if (expected_state == result_state_t::value &&
						result.state.compare_exchange_strong(expected_state, result_state_t::executable))
					{
						//Ok: new_state = result_state_t::executable;

						details::add_task_in_pool(std::move(pool_ctx), pool, std::move(result.next_task_v));
						assert(!std::get<pool::task_t>(result.next_task_v));
					}
					else
					{

					}
			}
		}

		static void set_result(pool::ctx_t pool_ctx, const prom_data_ptr<_Value>& res_data, value_or_promise_t<_Value>&& value_or_promise)
		{
			assert(!res_data->result.value.is_established());
			assert(value_or_promise.is_established());

			if (value_or_promise.has_promise())
			{
				const prom_data_ptr<_Value> arg_data{ take_data_of_promise(value_or_promise.get_promise()) };

				bind_next_step(
					pool_ctx,
					arg_data->result,
					*arg_data->pool,
					[res_data, arg_data](pool::ctx_t pool_ctx)
				{
					res_data->log_ctx = std::move(arg_data->log_ctx);
					set_result_impl(std::move(pool_ctx), *res_data->pool, res_data->result, std::move(arg_data->result.value));
				});
			}
			else
			{
				assert(value_or_promise.has_value() || value_or_promise.has_except());

				set_result_impl(std::move(pool_ctx), *res_data->pool, res_data->result, value_or_promise.take_value_t());
			}
		}
	};


	template<class _Result, class _Arg>
	value_or_promise_t<_Result> then_then(logger* log, std::wstring& log_ctx, value_t<_Arg>& arg, const then_t<_Result, _Arg>& thn)
	{
		assert(arg.is_established());
	
        add_details_log_ctx(log, log_ctx, (arg.has_value() ? L"scss"sv : L"rjct"sv));
        
        return api<_Result>::apply_then<_Arg>(log, log_ctx, thn, arg);
	}

	template<class _Result, class _Arg>
	value_or_promise_t<_Result> then_success_reject(logger* log, std::wstring& log_ctx, value_t<_Arg>& arg, const success_t<_Result, _Arg>& scss, const reject_t<_Result>& rjct)
	{
		assert(arg.is_established());

		if (arg.has_value())
		{
			add_details_log_ctx(log, log_ctx, L"scss"sv);
			return api<_Result>::apply_success<_Arg>(log, log_ctx, scss, arg);
		}
		
		assert(arg.has_except());

		add_details_log_ctx(log, log_ctx, L"rjct"sv);
		return api<_Result>::apply_reject(log, log_ctx, rjct, arg.get_except());
	}

	template<class _Result, class _Arg>
	value_or_promise_t<_Result> then_success(logger* log, std::wstring& log_ctx, value_t<_Arg>& arg, const success_t<_Result, _Arg>& scss)
	{
		assert(arg.is_established());

		if (arg.has_value())
		{
			add_details_log_ctx(log, log_ctx, L"scss"sv);
			return api<_Result>::apply_success<_Arg>(log, log_ctx, scss, arg);
		}

		assert(arg.has_except());

		add_details_log_ctx(log, log_ctx, L"scss-skip"sv);

		value_or_promise_t<_Result> result{};
		result.set_except(arg.get_except());
		return result;
	}

	template<class _Value>
	value_or_promise_t<_Value> then_reject(logger* log, std::wstring& log_ctx, value_t<_Value>&& val_value, const reject_t<_Value>& rjct)
	{
		assert(val_value.is_established());

		if (val_value.has_value())
		{
			add_details_log_ctx(log, log_ctx, L"rjct-skip"sv);

			return make_value_or_promise(std::move(val_value));
		}
		
		assert(val_value.has_except());

		add_details_log_ctx(log, log_ctx, L"rjct"sv);
		return api<_Value>::apply_reject(log, log_ctx, rjct, val_value.get_except());
	}

	template<class _Value>
	value_or_promise_t<_Value> then_finaly(logger* log, std::wstring& log_ctx, value_or_promise_t<_Value>&& result, const finally_t& fnly) noexcept
	{
		add_details_log_ctx(log, log_ctx, L"fnly"sv);

		try
		{
            const log_scope log_scope_guard{ log, L'[', log_ctx, L']' };
			fnly();
		}
		catch (...)
		{
			result.set_except(std::current_exception());

            // trace
            log_except(log, result.get_except(), L'[', log_ctx, L']');
		}

		return std::move(result);
	}

	template<class... _Values>
	struct api_all
	{
	public:

		using typle_values_t = std::tuple<_Values...>;
		using res_data_t = prom_data_ptr<typle_values_t>;
		using tuple_arg_datas_t = std::tuple<prom_data_ptr<_Values>...>;


		struct shared_data_t
		{
			std::atomic<std::size_t> state; // If more api_all<...>::size then an exception was set
			
			value_or_promise_t<typle_values_t> res_values;
		};

		static constexpr std::size_t size{ sizeof...(_Values) };
		
	private:

		using _TupleDatas = std::tuple<prom_data_ptr<_Values>...>;

		static constexpr std::size_t state__except{ api_all::size + 1 };

		static_assert(api_all::size < (static_cast<std::size_t>(~0) - 1));

		template<std::size_t arg_mumber>
		static void bind_next_steps_impl(const res_data_t& res_data, const std::shared_ptr<shared_data_t>& shared_data, tuple_arg_datas_t& tuple_arg_datas)
		{
			constexpr std::size_t arg_index{ arg_mumber - 1 };
			using arg_value_t = std::tuple_element_t<arg_index, typle_values_t>;
			using arg_data_t = prom_data_ptr<arg_value_t>;

			const arg_data_t& arg_data{ std::get<arg_index>(tuple_arg_datas) };

			details::api<arg_value_t>::bind_next_step(
				pool::unknown_ctx,
				arg_data->result,
				*arg_data->pool,
				[res_data, arg_data, shared_data](pool::ctx_t pool_ctx)
			{
				assert(shared_data->res_values.has_value());

				std::size_t previous_state{ shared_data->state.load() };
				assert(previous_state > 0);

				std::size_t new_state{ previous_state - 1 };

				if (previous_state == state__except)
				{
					new_state = state__except;
				}
				else
				{
					value_t<arg_value_t>& arg_value{ arg_data->result.value };
					assert(arg_value.is_established());

					if (arg_value.has_value())
					{
						typle_values_t& typle_values{ shared_data->res_values.get_value() };

						std::get<arg_mumber - 1>(typle_values) = std::move(arg_value).get_value();

						while (!shared_data->state.compare_exchange_strong(previous_state, new_state) && previous_state != state__except)
						{
							assert(0 < previous_state && previous_state < state__except);
							new_state = previous_state - 1;
						}

						if (previous_state == state__except)
							new_state = state__except;
					}
					else
					{
						for (; !shared_data->state.compare_exchange_strong(previous_state, state__except) && previous_state != state__except; );

						assert(previous_state > 0);

						if (previous_state == state__except)
						{
							new_state = state__except;
						}
						else
						{
							assert(!shared_data->res_values.has_except());
							shared_data->res_values.set_except(arg_value.get_except());

							new_state = 0;
							if (previous_state > 1)
								std::this_thread::yield();
						}
					}
				}

				if (new_state == state__except)
				{
                    // trace
                    log_msg(res_data->pool->log(), L'[', res_data->log_ctx, L"] [number: "sv, arg_mumber, L" of "sv, size, L"] Ignoring result, after of exception"sv);
				}
				else
				if (new_state == 0)
				{
                    // trace
                    log_msg(res_data->pool->log(), L'[', res_data->log_ctx, L"] [number: "sv, arg_mumber, L" of "sv, size, ((shared_data->res_values.has_value()) ? L"] Received value"sv : L"] Received exception"sv));

					details::api<typle_values_t>::set_result(std::move(pool_ctx), res_data, std::move(shared_data->res_values));
				}
			});
		}
		template<std::size_t arg_mumber>
		static inline void bind_next_steps_recursive(const res_data_t& res_data, const std::shared_ptr<shared_data_t>& shared_data, tuple_arg_datas_t& tuple_arg_datas)
		{
			bind_next_steps_impl<arg_mumber>(res_data, shared_data, tuple_arg_datas);
			bind_next_steps_recursive<arg_mumber + 1>(res_data, shared_data, tuple_arg_datas);
		}
		template<>
		static inline void bind_next_steps_recursive<size>(const res_data_t& res_data, const std::shared_ptr<shared_data_t>& shared_data, tuple_arg_datas_t& tuple_arg_datas)
		{
			bind_next_steps_impl<size>(res_data, shared_data, tuple_arg_datas);
		}

	public:

		static inline void bind_next_steps(const res_data_t& res_data, const std::shared_ptr<shared_data_t>& shared_data, tuple_arg_datas_t& tuple_arg_datas)
		{
			bind_next_steps_recursive<1>(res_data, shared_data, tuple_arg_datas);
		}

		struct one
		{
			using value_t = std::tuple_element_t<0, std::tuple<_Values...>>;

			static inline promise<value_t>& get_ref(promise<value_t>& one_promise)
			{
				return one_promise;
			}
		};
	};

} // namespace async::details
