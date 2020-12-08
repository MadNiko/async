
#pragma once


#include <cassert>
#include <variant>
#include <exception>
#include <type_traits>



namespace async
{
	template<class _Result>
	class promise;


	template<class _Value, bool _HasPromiseValue>
	class value_base_t
	{
		using this_t = value_base_t<_Value, _HasPromiseValue>;

	protected:

		using value_type = std::decay_t<_Value>;
		using store_value_type = std::conditional_t<std::is_void_v<value_type>, bool, value_type>;

		using store_t = std::conditional_t<!_HasPromiseValue,
			std::variant<std::monostate, store_value_type, std::exception_ptr>,
			std::variant<std::monostate, store_value_type, promise<value_type>, std::exception_ptr>
		>;

	public:

		static constexpr std::size_t index_store_empty{ 0 };
		static constexpr std::size_t index_store_value{ 1 };
		static constexpr std::size_t index_store_except{ std::variant_size_v<store_t>-1 };

	public:

		[[nodiscard]] inline bool has_value()  const noexcept
		{
			return (index_store_value == m_store.index());
		}

		[[nodiscard]] inline bool has_except() const noexcept
		{
			return (index_store_except == m_store.index());
		}

		[[nodiscard]] inline bool is_established() const noexcept
		{
			return (index_store_empty != m_store.index());
		}

		inline void set_except(std::exception_ptr except)
		{
			assert(!has_except());
			m_store.emplace<index_store_except>(except);
		}

		[[nodiscard]] inline std::exception_ptr get_except() const
		{
			assert(index_store_except == m_store.index());
			return std::get<index_store_except>(m_store);
		}

		[[noreturn]] inline void rethrow_except() const
		{
			std::rethrow_exception(std::get<index_store_except>(m_store));
		}

		template<class... _Args>
		inline store_value_type& emplace_value(_Args&&... args)
		{
			assert(!is_established());
			return m_store.emplace<index_store_value>(std::forward<_Args>(args)...);
		}

		inline void set_value(this_t&& other)
		{
			assert(!is_established());
			m_store = std::move(other.m_store);
		}

		template<class _Value2>
		inline void set_value(_Value2&& value2)
		{
			assert(!is_established());
			m_store.emplace<index_store_value>(std::forward<_Value2>(value2));
		}

		[[nodiscard]] inline store_value_type& get_value() &
		{
			if (index_store_value != m_store.index())
				rethrow_except();
			
			return std::get<index_store_value>(m_store);
		}

		[[nodiscard]] inline store_value_type&& get_value() &&
		{
			if (index_store_value != m_store.index())
				rethrow_except();
			
			return std::get<index_store_value>(std::move(m_store));
		}

		[[nodiscard]] inline const store_value_type& get_value() const &
		{
			if (index_store_value != m_store.index())
				rethrow_except();
			
			return std::get<index_store_value>(m_store);
		}

		[[nodiscard]] inline const store_value_type&& get_value() const &&
		{
			if (index_store_value != m_store.index())
				rethrow_except();
			
			return std::get<index_store_value>(std::move(m_store));
		}

		inline void swap(this_t& other) noexcept
		{
			m_store.swap(other.m_store);
		}

	protected:

		[[nodiscard]] inline store_t& store() & noexcept { return m_store; }
		[[nodiscard]] inline const store_t& store() const & noexcept { return m_store; }
		[[nodiscard]] inline store_t&& store() && noexcept { return std::move(m_store); }
		[[nodiscard]] inline const store_t&& store() const && noexcept { return std::move(m_store); }

		friend class value_base_t<_Value, true>;

		template<class _Value, class other_store_t = typename value_base_t<_Value, false>::store_t>
		[[nodiscard]] static other_store_t& other_store(value_base_t<_Value, false>& other) noexcept
		{
			return other.m_store;
		}

		template<class _Value, class other_store_t = typename value_base_t<_Value, false>::store_t>
		[[nodiscard]] static other_store_t&& other_store(value_base_t<_Value, false>&& other) noexcept
		{
			return std::move(other.m_store);
		}

	private:

		store_t m_store;
	};

	template<class _Value>
	class value_t : public value_base_t<_Value, false>
	{};

	template<>
	class value_t<void> : public value_base_t<void, false>
	{
		using base_t = value_base_t<void, false>;

	public:

		inline void emplace_value()
		{
			base_t::emplace_value();
		}

		inline void set_value(base_t&& other)
		{
			base_t::set_value(std::move(other));
		}

		inline void set_value()
		{
			base_t::set_value(store_value_type{});
		}

		inline void get_value() const
		{
			if (index_store_value != store().index())
				rethrow_except();
		}
	};


	template<class _Value>
	[[nodiscard]] inline value_t<_Value> make_value(_Value&& value)
	{
		static_assert(!std::is_void_v<_Value>);
		static_assert(!std::is_same_v<_Value, std::exception_ptr>);

		value_t<_Value> result{};
		result.set_value(std::move(value));

		return result;
	}
	template<class _Value>
	[[nodiscard]] inline value_t<_Value> make_value()
	{
		static_assert(std::is_void_v<_Value>);

		value_t<_Value> result{};
		result.set_value();

		return result;
	}
	template<class _Value>
	[[nodiscard]] inline value_t<_Value> make_value(value_t<_Value> val_value) noexcept
	{
		return std::move(val_value);
	}


	template<class _Value, bool _HasPromiseValue>
	inline void swap(value_base_t<_Value, _HasPromiseValue>& left, value_base_t<_Value, _HasPromiseValue>& right) noexcept
	{
		left.swap(right);
	}

} // namespace async
