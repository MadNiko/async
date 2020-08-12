
#pragma once

#include <async\promise.hpp>


namespace async
{

	template<class _PoolImpl, class... _PoolArgs>
	manager make_manager(_PoolArgs&&... pool_args);

	std::wstring& normalize_log_ctx(std::wstring& raw_log_ctx);
	std::wstring normalize_log_ctx(std::wstring&& raw_log_ctx);
	std::wstring& add_next_log_ctx(std::wstring& log_ctx, std::wstring_view raw_next_node);
	std::wstring add_next_log_ctx(std::wstring&& log_ctx, std::wstring_view raw_next_node);


	class manager
	{
		template<class _Result>
		friend class promise;

		template<class _PoolImpl, class... _PoolArgs>
		friend manager make_manager(_PoolArgs&&... pool_args);

	public:

		manager(std::nullptr_t = nullptr);
		manager(std::unique_ptr<pool> uniq_pool_impl);
		manager(manager&& other);
		~manager();

		manager& operator=(manager&& other);

		void swap(manager& other);

	public:

		template<class _Value>
		promise<_Value> resolve(std::wstring log_ctx, _Value value);

		promise<void> resolve(std::wstring log_ctx);

		template<class _Value, class... _Args>
		promise<_Value> resolve_emplace(std::wstring log_ctx, _Args&&... args);

		template<class _Value>  promise<_Value>  resolve(promise<_Value> value);

		template<class _Value>  promise<_Value>  reject(std::wstring log_ctx, std::exception_ptr except);

		template<class _Result> promise<_Result> task(std::wstring log_ctx, task_t<_Result> tsk);

		promise<void> task(std::wstring log_ctx, task_t<void> tsk);

		template<class _Value>
		promise<_Value> task_here_and_now(std::wstring log_ctx, const std::function<void(typename promise<_Value>::send async_send)>& functor);

		promise<void> task_here_and_now(std::wstring log_ctx, const std::function<void(typename promise<void>::send async_send)>& functor);

		template<
			template<class _Item, class _Alloc = std::allocator<_Item>> class _Container,
			class _Result
		>
		promise<_Container<_Result>> all(std::wstring log_ctx, _Container<promise<_Result>> prmises);

		template<class... _Results>
		promise<std::tuple<_Results...>> all(std::wstring log_ctx, promise<_Results>... prmises);

	public:

		template<class _Value>
		promise<_Value> task_here_and_now(const std::function<void(typename promise<_Value>::send async_send)>& functor);

		promise<void> task_here_and_now(const std::function<void(typename promise<void>::send async_send)>& functor);

		template<
			template<class _Item, class _Alloc = std::allocator<_Item>> class _Container,
			class _Result
		>
		promise<_Container<_Result>> all(_Container<promise<_Result>> prmises);

		template<class... _Results>
		promise<std::tuple<_Results...>> all(promise<_Results>... prmises);

	public:

		void resume_threads();
		void stop_threads();

		void wait_tasks_complete();
		bool wait_tasks_complete_for(std::chrono::microseconds wait_time);

		void stop_threads_and_wait_them_complete();

		std::size_t busy_threads_count() const;
		std::size_t max_threads_count() const;

	private:

		template<class _Value>
		using prom_data_t = details::prom_data_t<_Value>;

		template<class _Value>
		using prom_data_ptr = details::prom_data_ptr<_Value>;

	private:

		pool& check_and_ref_pool();
		const pool& check_and_ref_pool() const;
		pool_ptr check_and_get_pool() const;

	private:

		pool_ptr m_pool;
	};

} // namespace async
