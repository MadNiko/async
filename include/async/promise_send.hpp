
#pragma once


#include <async\promise.hpp>


namespace async
{

	template<class _Value>
	class value_or_promise_t;


	template<class _Value>
	class promise<_Value>::send
	{
	public:

		template<class _Value>
		using prom_data_t = details::prom_data_t<_Value>;

		template<class _Value>
		using prom_data_ptr = details::prom_data_ptr<_Value>;

	public:

		send();
		send(prom_data_ptr<_Value> data);

		send(send&& other);
		send(const send& other);

		~send();

		send& operator=(send&& other);
		send& operator=(const send& other);

		void swap(send& other);

		bool is_not_yet_invoked() const noexcept;

	public:

		void resolve();
		bool resolve_if_not_yet_invoked();

		template<class _Value2> void resolve(_Value2&& raw_value);
		template<class _Value2> bool resolve_if_not_yet_invoked(_Value2&& raw_value);

		bool resolve_if_not_yet_invoked(value_t<_Value> val_value);

		void resolve(promise<_Value> value_promise);
		bool resolve_if_not_yet_invoked(promise<_Value> value_promise);

		void reject(std::exception_ptr except);
		bool reject_if_not_yet_invoked(std::exception_ptr except);

	private:

		void set_val_value(prom_data_ptr<_Value> data, value_or_promise_t<_Value> value_or_promise) const;
		
	private:

		struct data_t
		{
			prom_data_ptr<_Value> promise_data;
		};

		std::shared_ptr<data_t> m_data;
	};

} // namespace async
