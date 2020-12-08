
#pragma once


#include <async\value.hpp>
#include <async\promise.hpp>


namespace async
{
	template<class _Value>
	class value_or_promise_base_t : public value_base_t<_Value, true>
	{
		using base_t = value_base_t<_Value, true>;
		using this_t = value_or_promise_base_t<_Value>;

		static constexpr std::size_t index_store_promise{ base_t::index_store_value + 1 };

		static_assert(base_t::index_store_empty == 0);
		static_assert(base_t::index_store_value == base_t::index_store_empty + 1);
		static_assert(this_t::index_store_promise == base_t::index_store_value + 1);
		static_assert(base_t::index_store_except == this_t::index_store_promise + 1);
		static_assert(std::variant_size_v<base_t::store_t> == 4);

	public:

		using base_t::set_value;

		inline void set_value(value_t<_Value>&& val_value)
		{
			assert(!is_established());

			if (val_value.has_value())
				store().emplace<index_store_value>(std::get<value_t<_Value>::index_store_value>(other_store(std::move(val_value))));
			else if (val_value.has_except())
				store().emplace<index_store_except>(std::get<value_t<_Value>::index_store_except>(other_store(std::move(val_value))));
			else
				store().emplace<index_store_empty>();
		}

		inline value_t<_Value> take_value_t()
		{
			assert(is_established() && !has_promise());

			value_t<_Value> result{};

			if (has_value())
				other_store(result).emplace<value_t<_Value>::index_store_value>(std::get<index_store_value>(std::move(*this).store()));
			else if (has_except())
				other_store(result).emplace<value_t<_Value>::index_store_except>(std::get<index_store_except>(std::move(*this).store()));
			else
				other_store(result).emplace<value_t<_Value>::index_store_empty>(std::get<index_store_empty>(std::move(*this).store()));

			return result;
		}

	public:

		inline bool has_promise() const noexcept
		{
			return (index_store_promise == store().index());
		}

		inline void set_promise(promise<_Value>&& promise_value)
		{
			assert(!is_established());
			store().emplace<index_store_promise>(std::move(promise_value));
		}

		[[nodiscard]] inline promise<_Value>& get_promise() &
		{
			if (index_store_promise != store().index())
				rethrow_except();
			
			return std::get<index_store_promise>(store());
		}

		[[nodiscard]] inline promise<_Value>&& get_promise() &&
		{
			if (index_store_promise != store().index())
				rethrow_except();
			
			return std::get<index_store_promise>(std::move(store()));
		}

		[[nodiscard]] inline const promise<_Value>& get_promise() const &
		{
			if (index_store_promise != store().index())
				rethrow_except();
			
			return std::get<index_store_promise>(store());
		}

		[[nodiscard]] inline const promise<_Value>&& get_promise() const &&
		{
			if (index_store_promise != store().index())
				rethrow_except();
			
			return std::get<index_store_promise>(std::move(store()));
		}
	};

	template<class _Value>
	class value_or_promise_t : public value_or_promise_base_t<_Value>
	{};

	template<>
	class value_or_promise_t<void> : public value_or_promise_base_t<void>
	{
		using base_t = value_or_promise_base_t<void>;

	public:

		inline void emplace_value()
		{
			base_t::emplace_value();
		}

		inline void set_value(value_t<void>&& val_value)
		{
			base_t::set_value(std::move(val_value));
		}

		inline void set_value()
		{
			base_t::set_value(store_value_type{});
		}

		inline void get_value() const
		{
			[[maybe_unused]] const store_value_type value{ base_t::get_value() };
		}
	};



	template<class _Value>
	[[nodiscard]] inline value_t<_Value> make_value(value_or_promise_t<_Value> value)
	{
		return value.take_value_t();
	}

	template<class _Value>
	[[nodiscard]] inline value_or_promise_t<_Value> make_value_or_promise(_Value&& value)
	{
		static_assert(!std::is_void_v<_Value>);
		static_assert(!std::is_same_v<_Value, std::exception_ptr>);

		value_or_promise_t<_Value> result{};
		result.set_value(std::move(value));

		return result;
	}
	template<class _Value>
	[[nodiscard]] inline value_or_promise_t<_Value> make_value_or_promise()
	{
		static_assert(std::is_void_v<_Value>);

		value_or_promise_t<_Value> result{};
		result.set_value();

		return result;
	}
	template<class _Value>
	[[nodiscard]] inline value_or_promise_t<_Value> make_value_or_promise(value_t<_Value> value)
	{
		value_or_promise_t<_Value> result{};
		result.set_value(std::move(value));

		return result;
	}
	template<class _Value>
	[[nodiscard]] inline value_or_promise_t<_Value> make_value_or_promise(value_or_promise_t<_Value> value) noexcept
	{
		return std::move(value);
	}

} // namespace async
