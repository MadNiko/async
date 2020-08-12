
#pragma once

#include <async\promise_types.hpp>
#include <async\value.hpp>


namespace async::details
{
	template<class _Value>
	std::shared_ptr<typename multi_promise<_Value>::data_t> take_data_of_multi_promise(multi_promise<_Value>& mlt_promise);

} // namespace async::details


namespace async
{
	template<class _Result>
	class multi_promise
	{

	public:

		multi_promise() noexcept;
		multi_promise(multi_promise&& other) noexcept;
		multi_promise(const multi_promise& other) noexcept;
		~multi_promise();

		multi_promise& operator=(multi_promise&& other);
		multi_promise& operator=(const multi_promise& other);

	public:

		explicit operator bool() const noexcept;

		void swap(multi_promise& other) noexcept;

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

	public:

		struct data_t
		{
			pool_ptr pool;

			details::multi_result_t<_Result> mlt_result;

			std::wstring log_ctx;
		};

	private:

		//friend class manager;

		//template<class _Result2>
		//friend class promise;

		template<class _Value>
		friend std::shared_ptr<typename multi_promise<_Value>::data_t> details::take_data_of_multi_promise<_Value>(multi_promise<_Value>& mlt_promise);

	private:

		//multi_promise(pool_ptr pool);

		bool pool_is_equal(const pool_ptr& other_pool) const;

		std::shared_ptr<data_t> take_data();

	private:

		std::shared_ptr<data_t> m_data;
	};

} // namespace async
