
#pragma once

#include <async\promise.hpp>
#include <async\details__impl.hpp>


namespace async
{
	template<class _Result>
	promise<_Result>::promise() noexcept
		: m_data(nullptr)
	{}

	template<class _Result>
	promise<_Result>::promise(pool_ptr pool)
		: m_data(std::make_shared<prom_data_t<_Result>>())
	{
		m_data->pool = std::move(pool);
	}

	template<class _Result>
	promise<_Result>::promise(promise&& other) noexcept
		: promise()
	{
		this->swap(other);
	}

	template<class _Result>
	promise<_Result>::~promise()
	{}

	template<class _Result>
	bool promise<_Result>::pool_is_equal(const pool_ptr& other_pool) const
	{
		if (const prom_data_ptr<_Result> data = std::atomic_load(&m_data))
			return (data->pool == other_pool);

		return false;
	}

	template<class _Result>
	details::prom_data_ptr<_Result> promise<_Result>::take_data()
	{
		prom_data_ptr<_Result> data{ std::atomic_exchange(&m_data, prom_data_ptr<_Result>()) };

		if (!data)
			throw promise_error{ promise_errc::no_state };

		assert(&data->pool);

		return data;
	}

	template<class _Result>
	promise<_Result>& promise<_Result>::operator=(promise&& other)
	{
		promise(std::move(other)).swap(*this);
		return *this;
	}

	template<class _Result>
	promise<_Result>::operator bool() const noexcept
	{
		return (m_data != nullptr);
	}

	template<class _Result>
	void promise<_Result>::swap(promise& other) noexcept
	{
		m_data.swap(other.m_data);
	}

	template<class _Result>
	multi_promise<_Result> promise<_Result>::multi()
	{

	}

	template<class _Result>
	template<class _Result2>
	inline promise<_Result2> promise<_Result>::then(std::wstring log_ctx, then_t<_Result2, _Result> thn, finally_t fnly)
	{
		using namespace std::literals;

		const prom_data_ptr<_Result> arg_data{ this->take_data() };

		promise<_Result2> res_promise(arg_data->pool);

		const prom_data_ptr<_Result2> res_data{ res_promise.m_data };

		details::api<_Result>::bind_next_step(
			pool::unknown_ctx,
			arg_data->result,
			*arg_data->pool,
			[log_ctx = details::normalize_log_ctx(std::move(log_ctx)), res_data, arg_data, fnc_thn = std::move(thn), fnc_fnly = std::move(fnly)](pool::ctx_t pool_ctx)
		{
			res_data->log_ctx.swap(arg_data->log_ctx);
			details::append_log_ctx(res_data->log_ctx, log_ctx);

			value_or_promise_t<_Result2> result{ details::then_then<_Result2, _Result>(res_data->log_ctx, arg_data->result.value, fnc_thn) };

			if (fnc_fnly)
				result = details::then_finaly(res_data->log_ctx, std::move(result), fnc_fnly);

			details::api<_Result2>::set_result(std::move(pool_ctx), res_data, std::move(result));
		});

		return res_promise;
	}
	
	template<class _Result>
	template<class _Result2>
	inline promise<_Result2> promise<_Result>::then(then_t<_Result2, _Result> thn, finally_t fnly)
	{
		return this->then<_Result2>(std::wstring(), std::move(thn), std::move(fnly));
	}

	template<class _Result>
	inline promise<_Result> promise<_Result>::then(std::wstring log_ctx, then_t<_Result, _Result> thn, finally_t fnly)
	{
		return this->then<_Result>(std::move(log_ctx), std::move(thn), std::move(fnly));
	}
	
	template<class _Result>
	inline promise<_Result> promise<_Result>::then(then_t<_Result, _Result> thn, finally_t fnly)
	{
		return this->then<_Result>(std::wstring(), std::move(thn), std::move(fnly));
	}

	template<class _Result>
	template<class _Result2>
	inline promise<_Result2> promise<_Result>::then(std::wstring log_ctx, success_t<_Result2, _Result> scss, reject_t<_Result2> rjct, finally_t fnly)
	{
		const prom_data_ptr<_Result> arg_data{ this->take_data() };

		promise<_Result2> res_promise(arg_data->pool);

		const prom_data_ptr<_Result2> res_data{ res_promise.m_data };

		details::api<_Result>::bind_next_step(
			pool::unknown_ctx,
			arg_data->result,
			*arg_data->pool,
			[log_ctx = details::normalize_log_ctx(std::move(log_ctx)), res_data, arg_data, fnc_scss = std::move(scss), fnc_rjct = std::move(rjct), fnc_fnly = std::move(fnly)](pool::ctx_t pool_ctx)
		{
			res_data->log_ctx.swap(arg_data->log_ctx);
			details::append_log_ctx(res_data->log_ctx, log_ctx);

			value_or_promise_t<_Result2> result{ details::then_success_reject<_Result2, _Result>(res_data->log_ctx, arg_data->result.value, fnc_scss, fnc_rjct) };

			if (fnc_fnly)
				result = details::then_finaly(res_data->log_ctx, std::move(result), fnc_fnly);

			details::api<_Result2>::set_result(std::move(pool_ctx), res_data, std::move(result));
		});

		return res_promise;
	}

	template<class _Result>
	template<class _Result2>
	inline promise<_Result2> promise<_Result>::then(success_t<_Result2, _Result> scss, reject_t<_Result2> rjct, finally_t fnly)
	{
		return this->then<_Result2>(std::wstring(), std::move(scss), std::move(rjct), std::move(fnly));
	}

	template<class _Result>
	inline promise<_Result> promise<_Result>::then(std::wstring log_ctx, success_t<_Result, _Result> scss, reject_t<_Result> rjct, finally_t fnly)
	{
		return this->then<_Result>(std::move(log_ctx), std::move(scss), std::move(rjct), std::move(fnly));
	}

	template<class _Result>
	inline promise<_Result> promise<_Result>::then(success_t<_Result, _Result> scss, reject_t<_Result> rjct, finally_t fnly)
	{
		return this->then<_Result>(std::wstring(), std::move(scss), std::move(rjct), std::move(fnly));
	}

	template<class _Result>
	template<class _Result2>
	inline promise<_Result2> promise<_Result>::success(std::wstring log_ctx, success_t<_Result2, _Result> scss, finally_t fnly)
	{
		const prom_data_ptr<_Result> arg_data{ this->take_data() };

		promise<_Result2> res_promise(arg_data->pool);

		const prom_data_ptr<_Result2> res_data{ res_promise.m_data };

		details::api<_Result>::bind_next_step(
			pool::unknown_ctx,
			arg_data->result,
			*arg_data->pool,
			[log_ctx = details::normalize_log_ctx(std::move(log_ctx)), res_data, arg_data, fnc_scss = std::move(scss), fnc_fnly = std::move(fnly)](pool::ctx_t pool_ctx)
		{
			res_data->log_ctx.swap(arg_data->log_ctx);
			details::append_log_ctx(res_data->log_ctx, log_ctx);

			value_or_promise_t<_Result2> result{ details::then_success<_Result2, _Result>(res_data->log_ctx, arg_data->result.value, fnc_scss) };

			if (fnc_fnly)
				result = details::then_finaly(res_data->log_ctx, std::move(result), fnc_fnly);

			details::api<_Result2>::set_result(std::move(pool_ctx), res_data, std::move(result));
		});

		return res_promise;
	}

	template<class _Result>
	template<class _Result2>
	inline promise<_Result2> promise<_Result>::success(success_t<_Result2, _Result> scss, finally_t fnly)
	{
		return this->success<_Result2>(std::wstring(), std::move(scss), std::move(fnly));
	}

	template<class _Result>
	inline promise<_Result> promise<_Result>::success(std::wstring log_ctx, success_t<_Result, _Result> scss, finally_t fnly)
	{
		return this->success<_Result>(std::move(log_ctx), std::move(scss), std::move(fnly));
	}

	template<class _Result>
	inline promise<_Result> promise<_Result>::success(success_t<_Result, _Result> scss, finally_t fnly)
	{
		return this->success<_Result>(std::wstring(), std::move(scss), std::move(fnly));
	}

	template<class _Result>
	inline promise<_Result> promise<_Result>::reject(std::wstring log_ctx, reject_t<_Result> rjct, finally_t fnly)
	{
		const prom_data_ptr<_Result> arg_data{ this->take_data() };

		promise<_Result> res_promise(arg_data->pool);

		const prom_data_ptr<_Result> res_data{ res_promise.m_data };

		details::api<_Result>::bind_next_step(
			pool::unknown_ctx,
			arg_data->result,
			*arg_data->pool,
			[log_ctx = details::normalize_log_ctx(std::move(log_ctx)), res_data, arg_data, fnc_rjct = std::move(rjct), fnc_fnly = std::move(fnly)](pool::ctx_t pool_ctx)
		{
			res_data->log_ctx.swap(arg_data->log_ctx);
			details::append_log_ctx(res_data->log_ctx, log_ctx);

			value_or_promise_t<_Result> result{ details::then_reject<_Result>(res_data->log_ctx, std::move(arg_data->result.value), fnc_rjct) };

			if (fnc_fnly)
				result = details::then_finaly(res_data->log_ctx, std::move(result), fnc_fnly);

			details::api<_Result>::set_result(std::move(pool_ctx), res_data, std::move(result));
		});

		return res_promise;
	}

	template<class _Result>
	inline promise<_Result> promise<_Result>::reject(reject_t<_Result> rjct, finally_t fnly)
	{
		return this->reject(std::wstring(), std::move(rjct), std::move(fnly));
	}

	template<class _Result>
	inline promise<_Result> promise<_Result>::finaly(std::wstring log_ctx, finally_t fnly)
	{
		const prom_data_ptr<_Result> arg_data{ this->take_data() };

		promise<_Result> res_promise(arg_data->pool);

		const prom_data_ptr<_Result> res_data{ res_promise.m_data };

		details::api<_Result>::bind_next_step(
			pool::unknown_ctx,
			arg_data->result,
			*arg_data->pool,
			[log_ctx = details::normalize_log_ctx(std::move(log_ctx)), res_data, arg_data, fnc_fnly = std::move(fnly)](pool::ctx_t pool_ctx)
		{
			res_data->log_ctx.swap(arg_data->log_ctx);
			details::append_log_ctx(res_data->log_ctx, log_ctx);

			value_or_promise_t<_Result> arg_data_result;
			arg_data_result.set_value(std::move(arg_data->result.value));

			details::api<_Result>::set_result(std::move(pool_ctx), res_data, details::then_finaly<_Result>(res_data->log_ctx, std::move(arg_data_result), fnc_fnly));
		});

		return res_promise;
	}

	template<class _Result>
	inline promise<_Result> promise<_Result>::finaly(finally_t fnly)
	{
		return this->finaly(std::wstring(), std::move(fnly));
	}

} // namespace async
