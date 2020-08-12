
#pragma once


#include <mutex>
#include <tuple>
#include <thread>
#include <variant>
#include <cstdint>
#include <optional>
#include <algorithm>
#include <functional>
#include <condition_variable>

#include <async\pool.hpp>
#include <async\logger.hpp>
#include <async\promise_errc.hpp>


namespace async::pool_threads
{

	class always : public async::pool
	{

	public:

		static constexpr bool UseThreadReservationAlgorithm{ true };

	private:

		struct data_t
		{
			mutable std::mutex access;

			struct
			{
				std::condition_variable queue_changed;
				std::condition_variable all_threads_idle;

			} cv;

            std::unique_ptr<logger> logger;
			std::wstring pool_name;

			std::size_t max_threads_count;
			std::size_t idle_threads_count;

			tasks_t tasks;

			bool stop_working;

			std::function<void(const std::function<void()>&)> threads_wrapper;
		};

	public:

		static constexpr std::size_t threads_limits_min{ 1 };
		static constexpr std::size_t threads_limits_max{ static_cast<std::size_t>(~0) };

	public:

        always(std::unique_ptr<logger> logger, std::wstring pool_name, std::size_t threads_count, std::function<void(const std::function<void()>&)> threads_wrapper)
			: m_data{}
			, m_threads{}
		{
            if (logger)
            {
                m_data.logger = std::move(logger);
                m_data.pool_name = (pool_name.empty() ? L"pool-threads-always"s : std::move(pool_name));
            }

			m_data.stop_working = true;
			m_data.max_threads_count = normalize_threads_count(threads_count);
			m_data.idle_threads_count = 0;
			m_data.threads_wrapper = std::move(threads_wrapper);
			m_data.tasks.out_of_queue_by_threads.resize(m_data.max_threads_count);

			m_threads.storage.resize(m_data.max_threads_count);

			std::unique_lock<std::mutex> un_lk_data{ m_data.access };
			start_threads_impl(un_lk_data);
		}

        always(std::size_t threads_count, std::function<void(const std::function<void()>&)> threads_wrapper)
            : always(nullptr, std::wstring{}, threads_count, std::move(threads_wrapper))
        {}

		virtual ~always() // noexcept(false)
		{
			stop_threads_and_wait_them_complete();
		}

		always(always&& other) = delete;
		always(const always& other) = delete;

		always& operator=(always&& other) = delete;
		always& operator=(const always& other) = delete;

	public:

		virtual void add_task(ctx_t this_ctx, task_t task) override
		{
			const std::lock_guard<std::mutex> lk{ m_data.access };

			if (m_data.stop_working)
			{
				m_data.tasks.add_task_in_queue(std::move(task));
				return;
			}

			if constexpr (UseThreadReservationAlgorithm)
			{
				assert(m_data.max_threads_count == m_threads.storage.size());

				if (m_data.max_threads_count > 1 &&                           // если данный пул настроен работать в несколько потоков
					this == std::get<pool*>(this_ctx) &&                 // и текущая работа происходит в рамках этого же пула
					m_data.tasks.queue_size() <= stopped_threads_count())     // и свободных потоков больше (или равно) чем задачь в очереди
				{
					[[maybe_unused]] std::size_t thread_index{ static_cast<std::size_t>(-1) };
					assert(this_thread_of_pool(&thread_index));
					assert(thread_index == std::get<std::size_t>(this_ctx));

					// ...то мы можем выполнить эту задачу в текущем потоке вне очереди
					m_data.tasks.set_task_out_of_queue(std::get<std::size_t>(this_ctx), std::move(task));
					return;
				}
			}

			m_data.tasks.add_task_in_queue(std::move(task));

			if (m_data.idle_threads_count > 0)
				m_data.cv.queue_changed.notify_one();
		}

		virtual bool wait_tasks_complete() override
		{
			std::unique_lock<std::mutex> un_lk_data{ m_data.access };

			return m_data.stop_working
				? !m_data.tasks.tasks_is_exists()
				: wait_tasks_complete_for_impl(wait_time_infinity, un_lk_data);
		}

		virtual bool wait_tasks_complete_for(std::chrono::microseconds wait_time) override
		{
			std::unique_lock<std::mutex> un_lk_data{ m_data.access };

			if (m_data.stop_working)
				return !m_data.tasks.tasks_is_exists();

			return (wait_time == wait_time_zero)
				? (is_all_threads_idle(m_data) && !m_data.tasks.tasks_is_exists())
				: wait_tasks_complete_for_impl(wait_time, un_lk_data);
		}

		virtual void resume_threads() override
		{
			const std::lock_guard<std::mutex> lk_threads{ m_threads.access };
			std::unique_lock<std::mutex> un_lk_data{ m_data.access };

			if (m_data.stop_working)
			{
				m_data.tasks.move_extra_tasks_in_begin_queue();

				start_threads_impl(un_lk_data);
			}
		}

		virtual void stop_threads() override
		{
			std::unique_lock<std::mutex> un_lk_data{ m_data.access };

			if (!m_data.stop_working)
				stop_threads_impl(un_lk_data);
		}

		virtual void stop_threads_and_wait_them_complete() override
		{
			const std::lock_guard<std::mutex> lk_threads{ m_threads.access };
			std::unique_lock<std::mutex> un_lk_data{ m_data.access };

			if (!m_data.stop_working)
			{
				stop_threads_impl(un_lk_data);
				wait_threads_complete_impl(un_lk_data);
			}
		}

		virtual std::size_t busy_threads_count() const override
		{
			std::size_t idle_threads_count{ 0 };
			{
				const std::lock_guard<std::mutex> lk{ m_data.access };
				idle_threads_count = (m_data.stop_working ? max_threads_count() : m_data.idle_threads_count);
			}

			assert(max_threads_count() >= idle_threads_count);
			return (max_threads_count() - idle_threads_count);
		}

		virtual std::size_t max_threads_count() const override
		{
			assert(m_data.max_threads_count == m_threads.storage.size());
			return m_data.max_threads_count;
		}

    public:

        virtual logger* log() const noexcept override
        {
            return m_data.logger.get();
        }

	private:

		std::size_t stopped_threads_count() const noexcept
		{
			return (m_data.stop_working ? max_threads_count() : 0);
		}

		static std::size_t normalize_threads_count(std::size_t threads_count)
		{
			return std::max<std::size_t>(threads_limits_min, std::min<std::size_t>(threads_count, threads_limits_max));
		}

		static void thread_main(ctx_t pool_ctx)
		{
			const always* const itself{ static_cast<always*>(std::get<pool*>(pool_ctx)) };
			const always::data_t& data{ itself->m_data };

			const std::size_t thread_number{ std::get<std::size_t>(pool_ctx) + 1 };

            const log_scope log_scope_guard{ itself->log(), L'[', data.pool_name, L"] [work-thread] [number: "sv, thread_number, L']' };

			assert(1 <= thread_number && thread_number <= threads_limits_max);

			if (data.threads_wrapper)
			{
				try
				{
                    std::optional<log_scope> log_scope_guard_opt(std::in_place, itself->log(), L"[init-thread-wrapper]"sv);
					data.threads_wrapper([&]
					{
                        log_scope_guard_opt.reset();
						thread_main_impl(pool_ctx);
                        log_scope_guard_opt.emplace(itself->log(), L"[uninit-thread-wrapper]"sv);
					});
				}
				catch (...)
				{
					log_except(itself->log(), std::current_exception(), L"Work thread processing async task finished with error"sv);
				}
			}
			else
				thread_main_impl(pool_ctx);
		}

		static void thread_main_impl(ctx_t pool_ctx)
		{
			always* const itself{ static_cast<always*>(std::get<pool*>(pool_ctx)) };
			always::data_t& data{ itself->m_data };

			const std::size_t thread_index{ std::get<std::size_t>(pool_ctx) };

			for (;;)
			{
				task_t tsk{};
				{
					std::unique_lock<std::mutex> un_lk{ data.access };

					if (!is_continue_work_thread(data, thread_index))
					{
						assert(!data.stop_working);
						assert(!data.tasks.tasks_is_exists(thread_index));

						change_idle_threads_count(data, +1);
						{
							data.cv.queue_changed.wait(un_lk, std::bind(always::is_continue_work_thread, std::cref(data), thread_index));
						}
						change_idle_threads_count(data, -1);
					}

					if (data.stop_working)
						break;

					assert(data.tasks.tasks_is_exists(thread_index));

					tsk = data.tasks.take_next_task(thread_index);
				}

				try
				{
					tsk(std::as_const(pool_ctx));
				}
				catch (...)
				{
					log_except(itself->log(), std::current_exception(), L"Processing async task finished with error"sv);
				}
			}
		}

		static bool is_continue_work_thread(const data_t& data, std::size_t thread_index)
		{
			return (data.stop_working || data.tasks.tasks_is_exists(thread_index));
		}

		static bool is_all_threads_idle(const data_t& data) noexcept
		{
			return (data.idle_threads_count == data.max_threads_count);
		}

		static bool is_at_least_one_thread_works_or_already_no_tasks(const data_t& data) noexcept
		{
			return (data.idle_threads_count < data.max_threads_count) || !data.tasks.tasks_is_exists();
		}

		template <
			class diff_t = std::conditional_t<sizeof(std::int32_t) == sizeof(std::size_t), std::int32_t, std::int64_t>
		>
		static std::size_t change_idle_threads_count(data_t& data, diff_t diff)
		{
			if (diff != 0)
			{
				const std::size_t diff_abs{ static_cast<std::size_t>(std::abs(diff)) };

				const bool is_all_threads_idle_defore{ is_all_threads_idle(data) };

				if (diff < 0)
					data.idle_threads_count -= diff_abs;
				else
					data.idle_threads_count += diff_abs;

				const bool is_all_threads_idle_after{ is_all_threads_idle(data) };

				if (is_all_threads_idle_defore || is_all_threads_idle_after)
				{
					assert(is_all_threads_idle_defore != is_all_threads_idle_after);
					data.cv.all_threads_idle.notify_all();
				}
			}

			return data.idle_threads_count;
		}

		void join_thread(std::thread& thread, std::size_t thread_number) noexcept
		{
			try
			{
				thread.join();
			}
			catch (...)
			{
                log_except(m_data.logger.get(), std::current_exception(), L"Finish the thread of pool is failed [number: "sv, thread_number, L']');
			}
		}

		bool wait_tasks_complete_for_impl(std::chrono::microseconds wait_time, std::unique_lock<std::mutex>& un_lk_data)
		{
			assert(!m_data.stop_working);

			if (this_thread_of_pool(nullptr))
				throw promise_error{ promise_errc::deadlock }; // Waiting for the thread pool to finished from the thread in this pool

			if (is_all_threads_idle(m_data))
			{
				if (!m_data.tasks.tasks_is_exists())
					return true;

				// Задачу(и) вот только-только добавили, потоки просто ещё не успели её(их) подхватить...
				m_data.cv.all_threads_idle.wait(un_lk_data, std::bind(always::is_at_least_one_thread_works_or_already_no_tasks, std::cref(m_data)));

				if (is_all_threads_idle(m_data)/* && !m_data.tasks.tasks_is_exists()*/)
					return true;
			}

			if (wait_time == wait_time_infinity)
			{
				m_data.cv.all_threads_idle.wait(un_lk_data, std::bind(always::is_all_threads_idle, std::cref(m_data)));
			}
			else
			if (!m_data.cv.all_threads_idle.wait_for(un_lk_data, wait_time, std::bind(always::is_all_threads_idle, std::cref(m_data))))
			{
                log_msg(m_data.logger.get(), L"Did not wait for the completion of all flows..."sv);
				return false;
			}

			return true;
		}

		void wait_threads_complete_impl(std::unique_lock<std::mutex>& un_lk_data)
		{
			// Invoke under mutex: m_threads.access

			assert(m_data.stop_working);

			un_lk_data.unlock();
			{
				std::size_t thread_number{ 0 };
				for (auto& thread : m_threads.storage)
				{
					thread_number += 1;

					if (thread.joinable())
						join_thread(thread, thread_number);
				}
			}
			un_lk_data.lock();
		}

		void start_threads_impl(std::unique_lock<std::mutex>& un_lk_data)
		{
			// Invoke under mutex: m_threads.access

			assert(m_data.stop_working);
			m_data.stop_working = false;

			try
			{
				std::size_t thread_number{ 0 };
				for (std::thread& thread : m_threads.storage)
				{
					thread_number += 1;

					assert(!thread.joinable());

					for (std::size_t attempt = 1, max_attempt_count = 5; attempt <= max_attempt_count; ++attempt)
					{
						try
						{
							thread = std::thread(&always::thread_main, ctx_t{ this, thread_number - 1 });
							break;
						}
						catch (...)
						{
                            log_except(m_data.logger.get(), std::current_exception(), L"Start the thread of pool is failed [number: "sv, thread_number, L']');

							if (attempt >= max_attempt_count)
								throw;

							std::this_thread::yield();
						}
					}
				}
			}
			catch (...)
			{
				stop_threads_impl(un_lk_data);
				wait_threads_complete_impl(un_lk_data);
				throw;
			}
		}

		void stop_threads_impl(std::unique_lock<std::mutex>& un_lk_data)
		{
			// Invoke under mutex: m_threads.access

			(void)un_lk_data;

			m_data.stop_working = true;
			m_data.cv.queue_changed.notify_all();
		}

		bool this_thread_of_pool(std::size_t* thread_index) const
		{
			assert(!m_threads.storage.empty());

			const std::thread::id this_thread_id{ std::this_thread::get_id() };

			std::size_t thrd_index{ 0 };
			for (const std::thread& thread : m_threads.storage)
			{
				if (this_thread_id == thread.get_id())
				{
					if (thread_index)
						*thread_index = thrd_index;

					return true;
				}

				thrd_index += 1;
			}

			return false;
		}

	private:

		data_t m_data;

		struct
		{
			std::mutex access;
			std::vector<std::thread> storage;

		} m_threads;

		static constexpr std::chrono::microseconds wait_time_infinity{ std::chrono::microseconds::zero() };
	};

} // namespace async::pool_threads
