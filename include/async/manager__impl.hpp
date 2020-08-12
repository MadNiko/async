
#pragma once

#include <async\logger.hpp>
#include <async\manager.hpp>
#include <async\promise_errc.hpp>
#include <async\details__impl.hpp>


namespace async
{

	template<class _PoolImpl, class... _PoolArgs>
	[[nodiscard]] inline manager make_manager(_PoolArgs&&... pool_args)
	{
		static_assert(std::is_base_of_v<pool, _PoolImpl>);

		manager result{ nullptr };
		result.m_pool = std::make_shared<_PoolImpl>(std::forward<_PoolArgs>(pool_args)...);

		return result;
	}


	inline std::wstring& normalize_log_ctx(std::wstring& raw_log_ctx)
	{
		raw_log_ctx = details::normalize_log_ctx(std::move(raw_log_ctx));
		return raw_log_ctx;
	}
	[[nodiscard]] inline std::wstring normalize_log_ctx(std::wstring&& raw_log_ctx)
	{
		return std::move(normalize_log_ctx(raw_log_ctx));
	}
	inline std::wstring& add_next_log_ctx(std::wstring& log_ctx, std::wstring_view raw_next_node)
	{
		log_ctx.reserve(log_ctx.size() + raw_next_node.size() + 1/*details::log_node_separator*/);
		return details::append_log_ctx(log_ctx, details::normalize_log_ctx(raw_next_node));
	}
	[[nodiscard]] inline std::wstring add_next_log_ctx(std::wstring&& log_ctx, std::wstring_view raw_next_node)
	{
		return std::move(add_next_log_ctx(log_ctx, raw_next_node));
	}


	inline manager::manager(std::nullptr_t)
		: m_pool{ nullptr }
	{}

	inline manager::manager(std::unique_ptr<pool> uniq_pool_impl)
		: m_pool{ std::move(uniq_pool_impl) }
	{}

	inline manager::manager(manager&& other)
		: m_pool{ nullptr }
	{
		this->swap(other);
	}

	inline manager::~manager()
	{
		if (m_pool)
			m_pool->stop_threads_and_wait_them_complete();
	}

	inline manager& manager::operator=(manager&& other)
	{
		manager(std::move(other)).swap(*this);
		return *this;
	}

	inline void manager::swap(manager& other)
	{
		m_pool.swap(other.m_pool);
	}

	inline void manager::resume_threads()
	{
		check_and_ref_pool().resume_threads();
	}

	inline void manager::stop_threads()
	{
		check_and_ref_pool().stop_threads();
	}

	inline void manager::wait_tasks_complete()
	{
		check_and_ref_pool().wait_tasks_complete();
	}

	inline bool manager::wait_tasks_complete_for(std::chrono::microseconds wait_time)
	{
		return check_and_ref_pool().wait_tasks_complete_for(wait_time);
	}

	inline void manager::stop_threads_and_wait_them_complete()
	{
		check_and_ref_pool().stop_threads_and_wait_them_complete();
	}

	[[nodiscard]] inline std::size_t manager::busy_threads_count() const
	{
		return check_and_ref_pool().busy_threads_count();
	}

	[[nodiscard]] inline std::size_t manager::max_threads_count() const
	{
		return check_and_ref_pool().max_threads_count();
	}

	[[nodiscard]] inline pool& manager::check_and_ref_pool()
	{
		if (pool* const result = m_pool.get())
			return *result;

		throw promise_error{ promise_errc::no_state };
	}

	[[nodiscard]] inline const pool& manager::check_and_ref_pool() const
	{
		if (const pool* const result = m_pool.get())
			return *result;

		throw promise_error{ promise_errc::no_state };
	}

	[[nodiscard]] inline pool_ptr manager::check_and_get_pool() const
	{
		if (pool_ptr result = m_pool)
			return result;

		throw promise_error{ promise_errc::no_state };
	}

	template<class _Value>
	inline promise<_Value> manager::resolve(std::wstring log_ctx, _Value value)
	{
		promise<_Value> res_promise{ check_and_get_pool() };
		res_promise.m_data->result.value.set_value(std::move(value));
		res_promise.m_data->result.state = details::result_state_t::value;

		assert(res_promise.m_data->result.value.is_established());

        if (logger* const log = m_pool->log())
        {
            res_promise.m_data->log_ctx = details::normalize_log_ctx(std::move(log_ctx));

            // trace
            log_msg(log, L'[', res_promise.m_data->log_ctx, L"] Resolve"sv);

            details::add_details_log_ctx(res_promise.m_data->log_ctx, L"scss"sv);
        }

		return res_promise;
	}

	inline promise<void> manager::resolve(std::wstring log_ctx)
	{
		promise<void> res_promise{ check_and_get_pool() };
		res_promise.m_data->result.value.set_value();
		res_promise.m_data->result.state = details::result_state_t::value;

		assert(res_promise.m_data->result.value.is_established());

        if (logger* const log = m_pool->log())
        {
		    res_promise.m_data->log_ctx = details::normalize_log_ctx(std::move(log_ctx));

            // trace
            log_msg(log, L'[', res_promise.m_data->log_ctx, L"] Resolve"sv);

		    details::add_details_log_ctx(log, res_promise.m_data->log_ctx, L"scss"sv);
        }

		return res_promise;
	}

	template<class _Value, class... _Args>
	inline promise<_Value> manager::resolve_emplace(std::wstring log_ctx, _Args&&... args)
	{
		promise<_Value> res_promise{ check_and_get_pool() };
		res_promise.m_data->result.value.emplace_value(std::forward<_Args>(args)...);
		res_promise.m_data->result.state = details::result_state_t::value;

		assert(res_promise.m_data->result.value.is_established());

        if (logger* const log = m_pool->log())
        {
            res_promise.m_data->log_ctx = details::normalize_log_ctx(std::move(log_ctx));

            // trace
            log_msg(log, L'[', res_promise.m_data->log_ctx, L"] Resolve"sv);

            details::add_details_log_ctx(res_promise.m_data->log_ctx, L"scss"sv);
        }

		return res_promise;
	}

	template<class _Value>
	inline promise<_Value> manager::resolve(promise<_Value> value)
	{
		pool_ptr res_pool{ check_and_get_pool() };

		if (value.pool_is_equal(res_pool))
			return std::move(value);

		const prom_data_ptr<_Value> arg_data{ value.take_data() };

		promise<_Value> res_promise{ std::move(res_pool) };

		details::api<_Value>::bind_next_step(
			pool::unknown_ctx,
			arg_data->result,
			*arg_data->pool,
			[res_data = res_promise.m_data, arg_data](pool::ctx_t pool_ctx) mutable
		{
			res_data->log_ctx = std::move(arg_data->log_ctx);

			value_or_promise_t<_Value> result{};
			result.set_value(std::move(arg_data->result.value));

			details::api<_Value>::set_result(std::move(pool_ctx), res_data, std::move(result));
		});

		return res_promise;
	}

	template<class _Value>
	inline promise<_Value> manager::reject(std::wstring log_ctx, std::exception_ptr except)
	{
		promise<_Value> res_promise{ check_and_get_pool() };
		res_promise.m_data->result.value.set_except(except);
		res_promise.m_data->result.state = details::result_state_t::value;

		assert(res_promise.m_data->result.value.is_established());

        if (logger* const log = m_pool->log())
        {
            res_promise.m_data->log_ctx = details::normalize_log_ctx(std::move(log_ctx));

            // trace
            log_msg(log, L'[', res_promise.m_data->log_ctx, L"] Reject"sv);

            details::add_details_log_ctx(res_promise.m_data->log_ctx, L"rjct"sv);
        }

		return res_promise;
	}

	template<class _Result>
	inline promise<_Result> manager::task(std::wstring log_ctx, task_t<_Result> tsk_v)
	{
		promise<_Result> res_promise{ check_and_get_pool() };

		const prom_data_ptr<_Result>& res_data{ res_promise.m_data };
		
        if (logger* const log = m_pool->log())
		    res_data->log_ctx = details::normalize_log_ctx(log, std::move(log_ctx), L"task"sv);
		
		details::add_task_in_pool(
			pool::unknown_ctx,
			*res_data->pool,
			[res_data, tsk_v = std::move(tsk_v)](pool::ctx_t this_ctx)
		{
			details::api<_Result>::set_result(std::move(this_ctx), res_data, details::api<_Result>::apply_task(res_data->pool->log(), res_data->log_ctx, tsk_v));
		});

		return res_promise;
	}

	inline promise<void> manager::task(std::wstring log_ctx, task_t<void> tsk)
	{
		return this->task<void>(std::move(log_ctx), std::move(tsk));
	}

	template<class _Value>
	inline promise<_Value> manager::task_here_and_now(const std::function<void(typename promise<_Value>::send async_send)>& functor)
	{
		return this->task_here_and_now(std::wstring{}, functor);
	}
	template<class _Value>
	inline promise<_Value> manager::task_here_and_now(std::wstring log_ctx, const std::function<void(typename promise<_Value>::send async_send)>& functor)
	{
		promise<_Value> res_promise{ check_and_get_pool() };

		typename promise<_Value>::send res_send{ res_promise.m_data };

        std::optional<log_scope> log_scope_guard_opt;

        if (logger* const log = m_pool->log())
        {
            res_promise.m_data->log_ctx = details::normalize_log_ctx(log, std::move(log_ctx), L"task"sv);
            log_scope_guard_opt.emplace(log, L'[', details::add_details_log_ctx(log, std::as_const(res_promise.m_data->log_ctx), L"sync"sv), L']');
        }

		try
		{
			functor(res_send);
		}
		catch (...)
		{
			res_send.reject_if_not_yet_invoked(std::current_exception());
		}

		return res_promise;
	}

	inline promise<void> manager::task_here_and_now(const std::function<void(typename promise<void>::send async_send)>& functor)
	{
		return this->task_here_and_now<void>(std::wstring{}, functor);
	}
	inline promise<void> manager::task_here_and_now(std::wstring log_ctx, const std::function<void(typename promise<void>::send async_send)>& functor)
	{
		return this->task_here_and_now<void>(std::move(log_ctx), functor);
	}

	template<
		template<class _Item, class _Alloc = std::allocator<_Item>> class _Container,
		class _Result
	>
	inline promise<_Container<_Result>> manager::all(_Container<promise<_Result>> promises)
	{
		return this->all(std::wstring{}, promises...);
	}
	template<
		template<class _Item, class _Alloc = std::allocator<_Item>> class _Container,
		class _Result
	>
	inline promise<_Container<_Result>> manager::all(std::wstring log_ctx, _Container<promise<_Result>> promises)
	{
        if (m_pool->get())
		    log_ctx = details::normalize_log_ctx(std::move(log_ctx), L"all"sv);

		if (std::empty(promises))
		{
			return this->resolve_emplace<_Container<_Result>>(std::move(log_ctx));
		}

		promise<_Container<_Result>> res_promise{ check_and_get_pool() };

		const prom_data_ptr<_Container<_Result>> res_data{ res_promise.m_data };

		res_data->log_ctx = std::move(log_ctx);

        const std::size_t promises_count{ std::size(promises) };

		if (1 == promises_count)
		{
			const prom_data_ptr<_Result> arg_data{ promises.front().take_data() };

			details::api<_Result>::bind_next_step(
				pool::unknown_ctx,
				arg_data->result,
				*arg_data->pool,
				[res_data, arg_data](pool::ctx_t pool_ctx)
			{
				value_or_promise_t<_Container<_Result>> result{};
				if (arg_data->result.value.has_value())
				{
					result.emplace_value().push_back(std::move(arg_data->result.value).get_value());
				}
				else
				{
					result.set_except(arg_data->result.value.get_except());
				}

				details::api<_Container<_Result>>::set_result(std::move(pool_ctx), res_data, std::move(result));
			});

			return res_promise;
		}

		_Container<prom_data_ptr<_Result>> data_chain;
		{
			try
			{
				for (promise<_Result>& item_promise : promises)
				{
					prom_data_ptr<_Result> item_data{ item_promise.take_data() };
					assert(item_data != nullptr);
					assert(item_data->pool != nullptr);

					data_chain.push_back(std::move(item_data));
				}
			}
			catch (...)
			{
				return this->reject<_Container<_Result>>(std::move(log_ctx), std::current_exception());
			}
		}

        assert(promises_count == data_chain.size());
		assert(promises_count < (static_cast<std::size_t>(~0) - 1));
		const std::size_t state__except{ promises_count + 1 };

		struct shared_data_t
		{
			std::atomic<std::size_t> state; // If more res_values.size() then an exception was set

			value_or_promise_t<_Container<_Result>> res_values;
		};
		const std::shared_ptr<shared_data_t> shared_data{ std::make_shared<shared_data_t>() };
		shared_data->res_values.emplace_value().resize(promises_count);
		shared_data->state = promises_count;

		assert(shared_data->state == shared_data->res_values.get_value().size());
		assert(1 < shared_data->state && shared_data->state < state__except);

		std::size_t number{ 0 };
		for (const prom_data_ptr<_Result>& item_data : data_chain)
		{
			number += 1;

			details::api<_Result>::bind_next_step(
				pool::unknown_ctx,
				item_data->result,
				*item_data->pool,
				[res_data, number, state__except, item_data, shared_data](pool::ctx_t pool_ctx)
			{
				assert(shared_data->res_values.has_value());
				assert(number <= shared_data->res_values.get_value().size());

				std::size_t previous_state{ shared_data->state.load() };
				assert(previous_state > 0);

                std::size_t new_state{ previous_state - 1 };
                
                if (previous_state == state__except)
                {
                    new_state = state__except;
                }
                else
				{
					value_t<_Result>& res_value{ item_data->result.value };
					assert(res_value.is_established());

					if (res_value.has_value())
					{
						(*std::next(shared_data->res_values.get_value().begin(), (number - 1))) = std::move(res_value).get_value();

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
							shared_data->res_values.set_except(res_value.get_except());

							new_state = 0;
							if (previous_state > 1)
								std::this_thread::yield();
						}
					}
				}

				if (new_state == state__except)
				{
                    // trace
                    log_msg(res_data->pool->log(), L'[', res_data->log_ctx, L"] [number: "sv, number, L" of "sv, promises_count, L"] Ignoring result, after of exception"sv);
				}
				else
					if (new_state == 0)
					{
                        // trace
                        log_msg(res_data->pool->log(), L'[', res_data->log_ctx, L"] [number: "sv, number, L" of "sv, promises_count, ((shared_data->res_values.has_value()) ? L"] Received value"sv : L"] Received exception"sv));

						details::api<_Container<_Result>>::set_result(std::move(pool_ctx), res_data, std::move(shared_data->res_values));
					}
			});
		}

		return res_promise;
	}

	template<class... _Results>
	inline promise<std::tuple<_Results...>> manager::all(promise<_Results>... promises)
	{
		return this->all(std::wstring{}, promises...);
	}
	template<class... _Results>
	inline promise<std::tuple<_Results...>> manager::all(std::wstring log_ctx, promise<_Results>... promises)
	{
		using api_all = details::api_all<_Results...>;
		using res_data_t = typename api_all::res_data_t;
		using tuple_results_t = typename api_all::typle_values_t;

		static_assert(api_all::size >= 1);

		promise<tuple_results_t> res_promise{ check_and_get_pool() };

		const res_data_t res_data{ res_promise.m_data };

        if (m_pool->log())
		    res_data->log_ctx = details::normalize_log_ctx(std::move(log_ctx), L"all"sv);

		if constexpr (api_all::size == 1)
		{
			using api_one = typename api_all::one;
			using one_result_t = typename api_one::value_t;
			using tuple_one_result_t = std::tuple<one_result_t>;

			static_assert(std::is_same_v<tuple_results_t, tuple_one_result_t>);

			const prom_data_ptr<one_result_t> arg_data{ api_one::get_ref(promises...).take_data() };

			details::api<one_result_t>::bind_next_step(
				pool::unknown_ctx,
				arg_data->result,
				*arg_data->pool,
				[res_data, arg_data](pool::ctx_t pool_ctx)
			{
				value_or_promise_t<tuple_one_result_t> result{};
				if (arg_data->result.value.has_value())
				{
					result.emplace_value(std::move(arg_data->result.value).get_value());
				}
				else
				{
					result.set_except(arg_data->result.value.get_except());
				}

				details::api<tuple_one_result_t>::set_result(std::move(pool_ctx), res_data, std::move(result));
			});
		}
		else
		{
			using tuple_arg_datas_t = typename api_all::tuple_arg_datas_t;
			using shared_data_t = typename api_all::shared_data_t;

			tuple_arg_datas_t tuple_arg_datas{};
			{
				try
				{
					tuple_arg_datas = tuple_arg_datas_t(promises.take_data()...);
				}
				catch (...)
				{
					return this->reject<tuple_results_t>(std::move(log_ctx), std::current_exception());
				}
			}

			const std::shared_ptr<shared_data_t> shared_data{ std::make_shared<shared_data_t>() };
			shared_data->res_values.emplace_value();
			shared_data->state = api_all::size;

			api_all::bind_next_steps(res_data, shared_data, tuple_arg_datas);
		}

		return res_promise;
	}

} // namespace async
