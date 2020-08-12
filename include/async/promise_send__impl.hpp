
#pragma once


#include <optional>

#include <async\logger.hpp>
#include <async\promise_send.hpp>
#include <async\details__impl.hpp>


namespace async
{

	template<class _Value>
	inline promise<_Value>::send::send()
		: m_data{ nullptr }
	{}

	template<class _Value>
	inline promise<_Value>::send::send(prom_data_ptr<_Value> data)
		: m_data{ data ? std::make_shared<data_t>() : nullptr }
	{
		if (m_data)
			m_data->promise_data = std::move(data);
	}

	template<class _Value>
	inline promise<_Value>::send::send(send&& other)
		: m_data{ std::move(other.m_data) }
	{}

	template<class _Value>
	inline promise<_Value>::send::send(const send& other)
		: m_data{ other.m_data }
	{}

	template<class _Value>
	inline void promise<_Value>::send::set_val_value(prom_data_ptr<_Value> data, value_or_promise_t<_Value> value_or_promise) const
	{
		assert(data);

        std::optional<log_scope> log_scope_guard_opt;

        if (logger * const log = data->pool->log())
        {
		    details::add_details_log_ctx(log, data->log_ctx, (value_or_promise.has_value() ? L"scss"sv : (value_or_promise.has_promise() ? L"prom"sv : L"rjct"sv)));
            log_scope_guard_opt.emplace(log, L'[', data->log_ctx, L']');
        }

		details::api<_Value>::set_result(pool::unknown_ctx, data, std::move(value_or_promise));
	}

	template<class _Value>
	inline promise<_Value>::send::~send()
	{
		if (m_data)
		{
			if (1 == m_data.use_count())
			{
				if (m_data->promise_data)
				{
					try
					{
						value_or_promise_t<_Value> result{};
						result.set_except(std::make_exception_ptr(promise_error{ promise_errc::broken_promise }));

						this->set_val_value(std::move(m_data->promise_data), std::move(result));
					}
					catch (...)
					{
					}
				}
			}
		}
	}

	template<class _Value>
	inline typename promise<_Value>::send& promise<_Value>::send::operator=(send&& other)
	{
		send{ std::move(other) }.swap(*this);
		return *this;
	}

	template<class _Value>
	inline typename promise<_Value>::send& promise<_Value>::send::operator=(const send& other)
	{
		send{ other }.swap(*this);
		return *this;
	}

	template<class _Value>
	inline void promise<_Value>::send::swap(send& other)
	{
		m_data.swap(other.m_data);
	}

	template<class _Value>
	inline bool promise<_Value>::send::is_not_yet_invoked() const noexcept
	{
		return m_data && m_data->promise_data;
	}

	template<class _Value>
	inline void promise<_Value>::send::resolve()
	{
		static_assert(std::is_same_v<std::decay_t<_Value>, void>);

		if (m_data)
		{
			if (prom_data_ptr<_Value> promise_data = std::atomic_exchange(&m_data->promise_data, prom_data_ptr<_Value>{}))
			{
				value_or_promise_t<_Value> value{};
				value.set_value();

				this->set_val_value(std::move(promise_data), std::move(value));

				return;
			}
		}

		throw promise_error{ promise_errc::value_already_retrieved };
	}
	template<class _Value>
	inline bool promise<_Value>::send::resolve_if_not_yet_invoked()
	{
		static_assert(std::is_same_v<std::decay_t<_Value>, void>);

		try
		{
			this->resolve();
			
			return true;
		}
		catch (...)
		{
		}

		return false;
	}

	template<class _Value>
	template<class _Value2>
	inline void promise<_Value>::send::resolve(_Value2&& raw_value)
	{
		static_assert(!std::is_same_v<std::decay_t<_Value>, void>);
		static_assert(std::is_same_v< std::decay_t<_Value>, std::decay_t<_Value2>>);

		if (m_data)
		{
			if (prom_data_ptr<_Value> promise_data = std::atomic_exchange(&m_data->promise_data, prom_data_ptr<_Value>{}))
			{
				value_or_promise_t<_Value> value{};
				value.set_value(std::forward<_Value2>(raw_value));

				this->set_val_value(std::move(promise_data), std::move(value));

				return;
			}
		}

		throw promise_error{ promise_errc::value_already_retrieved };
	}
	template<class _Value>
	template<class _Value2>
	inline bool promise<_Value>::send::resolve_if_not_yet_invoked(_Value2&& raw_value)
	{
		static_assert(!std::is_same_v<std::decay_t<_Value>, void>);
		static_assert(std::is_same_v< std::decay_t<_Value>, std::decay_t<_Value2>>);

		try
		{
			this->resolve(std::forward<_Value2>(raw_value));

			return true;
		}
		catch (...)
		{
		}

		return false;
	}

	template<class _Value>
	inline bool promise<_Value>::send::resolve_if_not_yet_invoked(value_t<_Value> val_value)
	{
		if (m_data)
		{
			if (prom_data_ptr<_Value> promise_data = std::atomic_exchange(&m_data->promise_data, prom_data_ptr<_Value>{}))
			{
				try
				{
					value_or_promise_t<_Value> value{};
					value.set_value(std::move(val_value));

					this->set_val_value(std::move(promise_data), std::move(value));

					return true;
				}
				catch (...)
				{
				}
			}
		}

		return false;
	}

	template<class _Value>
	inline void promise<_Value>::send::resolve(promise<_Value> promise_value)
	{
		if (m_data)
		{
			if (prom_data_ptr<_Value> promise_data = std::atomic_exchange(&m_data->promise_data, prom_data_ptr<_Value>{}))
			{
				value_or_promise_t<_Value> value{};
				value.set_promise(std::move(promise_value));

				this->set_val_value(std::move(promise_data), std::move(value));

				return;
			}
		}

		throw promise_error{ promise_errc::value_already_retrieved };
	}

	template<class _Value>
	inline bool promise<_Value>::send::resolve_if_not_yet_invoked(promise<_Value> promise_value)
	{
		try
		{
			this->resolve(std::move(promise_value));

			return true;
		}
		catch (...)
		{
		}

		return false;
	}

	template<class _Value>
	inline void promise<_Value>::send::reject(std::exception_ptr except)
	{
		if (m_data)
		{
			if (prom_data_ptr<_Value> promise_data = std::atomic_exchange(&m_data->promise_data, prom_data_ptr<_Value>{}))
			{
				value_or_promise_t<_Value> value{};
				value.set_except(except);

				this->set_val_value(std::move(promise_data), std::move(value));

				return;
			}
		}

		throw promise_error{ promise_errc::value_already_retrieved };
	}

	template<class _Value>
	inline bool promise<_Value>::send::reject_if_not_yet_invoked(std::exception_ptr except)
	{
		try
		{
			this->reject(except);

			return true;
		}
		catch (...)
		{
		}

		return false;
	}

} // namespace async
