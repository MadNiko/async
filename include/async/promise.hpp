
#pragma once

#include <async\promise_types.hpp>
#include <async\value.hpp>


namespace async::details
{
	template<class _Value>
	std::shared_ptr<typename details::prom_data_t<_Value>> take_data_of_promise(promise<_Value>& promise);

} // namespace async::details


namespace async
{
	template<class _Result>
	class promise
	{

	public:

		class send;

	public:

		promise() noexcept;
		promise(promise&& other) noexcept;
		~promise();

		promise& operator=(promise&& other);

		promise(const promise& other) = delete;
		promise& operator=(const promise& other) = delete;

	public:

		explicit operator bool() const noexcept;

		void swap(promise& other) noexcept;

		multi_promise<_Result> multi();

	public:

		promise<_Result> then(std::wstring log_ctx, then_t<_Result, _Result> thn, finally_t fnly = {});
		promise<_Result> then(                      then_t<_Result, _Result> thn, finally_t fnly = {});

		promise<_Result> then(std::wstring log_ctx, success_t<_Result, _Result> scss, reject_t<_Result> rjct, finally_t fnly = {});
		promise<_Result> then(                      success_t<_Result, _Result> scss, reject_t<_Result> rjct, finally_t fnly = {});

		template<class _Result2> promise<_Result2> then(std::wstring log_ctx, then_t<_Result2, _Result> thn, finally_t fnly = {});
		template<class _Result2> promise<_Result2> then(                      then_t<_Result2, _Result> thn, finally_t fnly = {});

		template<class _Result2> promise<_Result2> then(std::wstring log_ctx, success_t<_Result2, _Result> scss, reject_t<_Result2> rjct, finally_t fnly = {});
		template<class _Result2> promise<_Result2> then(                      success_t<_Result2, _Result> scss, reject_t<_Result2> rjct, finally_t fnly = {});

		promise<_Result> success(std::wstring log_ctx, success_t<_Result, _Result> scss, finally_t fnly = {});
		promise<_Result> success(                      success_t<_Result, _Result> scss, finally_t fnly = {});

		template<class _Result2> promise<_Result2> success(std::wstring log_ctx, success_t<_Result2, _Result> scss, finally_t fnly = {});
		template<class _Result2> promise<_Result2> success(                      success_t<_Result2, _Result> scss, finally_t fnly = {});

		promise<_Result> reject(std::wstring log_ctx, reject_t<_Result> rjct, finally_t fnly = {});
		promise<_Result> reject(                      reject_t<_Result> rjct, finally_t fnly = {});

		promise<_Result> finaly(std::wstring log_ctx, finally_t fnly);
		promise<_Result> finaly(                      finally_t fnly);

	private:

		friend class manager;

		template<class _Result2>
		friend class promise;

		template<class _Value>
		using prom_data_t = details::prom_data_t<_Value>;

		template<class _Value>
		using prom_data_ptr = details::prom_data_ptr<_Value>;

		template<class _Value>
		friend std::shared_ptr<prom_data_t<_Value>> details::take_data_of_promise<_Value>(promise<_Value>& promise);

	private:

		promise(pool_ptr pool);

		bool pool_is_equal(const pool_ptr& other_pool) const;

		prom_data_ptr<_Result> take_data();

	private:

		prom_data_ptr<_Result> m_data;
	};

} // namespace async
