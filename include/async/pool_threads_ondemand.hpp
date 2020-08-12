
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
    class ondemand : public async::pool
    {
    public:

        static constexpr bool UseThreadReservationAlgorithm{ true };

    private:

        using threads_mask_t = std::uint32_t;

        struct data_t
        {
            mutable std::mutex access;

            struct
            {
                std::condition_variable queue_changed;
                std::condition_variable state_threads_changed;

            } cv;

            std::unique_ptr<logger> logger;
            std::wstring pool_name;

            std::size_t idle_threads_count;

            tasks_t tasks;

            threads_mask_t mask_stopped;

            bool stop_working;
            bool wake_up_sleep_threads;

            std::chrono::microseconds waiting_time_new_tasks;
            std::function<void(const std::function<void()>&)> threads_wrapper;
        };

    public:

        static const std::size_t threads_limits_min{ 1 };
        static const std::size_t threads_limits_max{ 8 * sizeof(threads_mask_t) };

        static std::chrono::microseconds waiting_time_new_tasks_default() noexcept
        {
            using namespace std::chrono_literals;
            return 30s;
        }

        static constexpr std::chrono::microseconds waiting_time_new_tasks__none{ std::chrono::microseconds::zero() };

    private:

        static const threads_mask_t mask_all_threads_of_stopped{ static_cast<threads_mask_t>(~0) };

    public:

        ondemand(std::unique_ptr<logger> logger, std::wstring pool_name, std::size_t threads_count, std::chrono::microseconds waiting_time_new_tasks, std::function<void(const std::function<void()>&)> threads_wrapper)
            : m_data{}
            , m_threads(normalize_threads_count(threads_count))
        {
            if (logger)
            {
                m_data.logger = std::move(logger);
                m_data.pool_name = (pool_name.empty() ? L"pool-threads-ondemand"s : std::move(pool_name));
            }

            m_data.stop_working = false;
            m_data.idle_threads_count = 0;
            m_data.mask_stopped = mask_all_threads_of_stopped;
            m_data.wake_up_sleep_threads = false;
            m_data.waiting_time_new_tasks = std::move(waiting_time_new_tasks);
            m_data.threads_wrapper = std::move(threads_wrapper);
            m_data.tasks.out_of_queue_by_threads.resize(m_threads.size());
        }

        ondemand(std::size_t threads_count, std::chrono::microseconds waiting_time_new_tasks, std::function<void(const std::function<void()>&)> threads_wrapper)
            : ondemand(nullptr, std::wstring{}, threads_count, waiting_time_new_tasks, std::move(threads_wrapper))
        {}

		ondemand(ondemand&& other) = delete;
		ondemand(const ondemand& other) = delete;

		virtual ~ondemand() // noexcept(false)
		{
			stop_threads_and_wait_them_complete();
		}

		ondemand& operator=(ondemand&& other) = delete;
		ondemand& operator=(const ondemand& other) = delete;

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
				if (m_threads.size() > 1 &&                                   // если данный пул настроен работать в несколько потоков
					this == std::get<pool*>(this_ctx) &&                      // и текущая работа происходит в рамках этого же пула
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
			resume_threads_impl(1);
		}

		virtual void resume_threads() override
		{
			const std::lock_guard<std::mutex> lk{ m_data.access };

			if (m_data.stop_working)
			{
				m_data.stop_working = false;

				m_data.tasks.move_extra_tasks_in_begin_queue();

				if (!m_data.tasks.queue_is_empty())
					resume_threads_impl(m_data.tasks.queue_size());
			}
		}

		virtual void stop_threads() override
		{
			std::unique_lock<std::mutex> un_lk{ m_data.access };

			if (!m_data.stop_working)
			{
				m_data.stop_working = true;
				m_data.cv.queue_changed.notify_all();
			}
		}

		virtual bool wait_tasks_complete() override
		{
			std::unique_lock<std::mutex> un_lk{ m_data.access };

			return wait_tasks_completion_for_impl(waiting_time_new_tasks__none, un_lk);
		}

		virtual bool wait_tasks_complete_for(std::chrono::microseconds wait_time) override
		{
			std::unique_lock<std::mutex> un_lk{ m_data.access };

			return wait_tasks_completion_for_impl(std::move(wait_time), un_lk);
		}

		virtual void stop_threads_and_wait_them_complete() override
		{
			std::unique_lock<std::mutex> un_lk{ m_data.access };

			m_data.stop_working = true;
			wait_tasks_completion_for_impl(waiting_time_new_tasks__none, un_lk);
		}

		virtual std::size_t busy_threads_count() const override
		{
			threads_mask_t mask_running{ 0 };
			std::size_t idle_threads_count{ 0 };
			{
				const std::lock_guard<std::mutex> lk{ m_data.access };

				mask_running = static_cast<threads_mask_t>(~m_data.mask_stopped);
				idle_threads_count = m_data.idle_threads_count;
			}

			std::size_t running_threads_count{ 0 };
			for (; mask_running != 0; mask_running >>= 1)
			{
				if ((mask_running & 1) != 0)
					running_threads_count += 1;
			}

			assert(running_threads_count >= idle_threads_count);
			return (running_threads_count - idle_threads_count);
		}

		virtual std::size_t max_threads_count() const override
		{
			return m_threads.size();
		}

	private:

		std::size_t stopped_threads_count() const noexcept
		{
			assert(threads_limits_min <= m_threads.size() && m_threads.size() <= threads_limits_max);

			const threads_mask_t mask_all_stopped{ static_cast<threads_mask_t>(m_threads.size() < threads_limits_max ? ((1 << m_threads.size())-1) : ~0) };

			threads_mask_t mask{ m_data.mask_stopped & mask_all_stopped };

			std::size_t result{ 0 };

			for (; mask != 0; mask >>= 1)
			{
				if ((mask & 1) != 0)
					result += 1;
			}

			return result;
		}

		static std::wstring mask_to_wstring(threads_mask_t threads_mask, std::size_t threads_count, wchar_t bit_set, wchar_t bit_notset)
		{
			std::wstring result(threads_count, bit_notset);

			std::size_t char_index{ 0 };
			threads_mask_t index_by_mask{ static_cast<threads_mask_t>(1 << (threads_count - 1)) };
			for (; char_index < threads_count; char_index += 1, index_by_mask >>= 1)
			{
				if ((threads_mask & index_by_mask) != 0)
					result[char_index] = bit_set;
			}

			return result;
		}

		static std::size_t normalize_threads_count(std::size_t threads_count)
		{
			return std::max<std::size_t>(threads_limits_min, std::min<std::size_t>(threads_count, threads_limits_max));
		}

		static void thread_main(ctx_t pool_ctx)
		{
			assert(dynamic_cast<ondemand*>(std::get<pool*>(pool_ctx)));

			const ondemand* const itself{ static_cast<ondemand*>(std::get<pool*>(pool_ctx)) };
			const ondemand::data_t& data{ itself->m_data };

			const std::size_t thread_number{ std::get<std::size_t>(pool_ctx) + 1 };

            const log_scope log_scope_guard{ data.logger.get(), L'[', data.pool_name, L"] [work-thread] [number: "sv, thread_number, L']' };

			assert(1 <= thread_number && thread_number <= threads_limits_max);

			if (data.threads_wrapper)
			{
				try
				{
                    std::optional<log_scope> log_scope_guard_opt{ std::in_place, data.logger.get(), L"[init-thread-wrapper]"sv };
					data.threads_wrapper([&]
					{
                        log_scope_guard_opt.reset();
						thread_main_impl(pool_ctx);
                        log_scope_guard_opt.emplace(data.logger.get(), L"[uninit-thread-wrapper]"sv);
					});
				}
				catch (...)
				{
                    log_except(data.logger.get(), std::current_exception(), L"Work thread processing async task finished with error"sv);
				}
			}
			else
				thread_main_impl(pool_ctx);
		}

		static void thread_main_impl(ctx_t pool_ctx)
		{
			assert(dynamic_cast<ondemand*>(std::get<pool*>(pool_ctx)));

			ondemand* const itself{ static_cast<ondemand*>(std::get<pool*>(pool_ctx)) };
			ondemand::data_t& data{ itself->m_data };

			const std::size_t thread_index{ std::get<std::size_t>(pool_ctx) };

			//bool first_iteration{ true };
			for (;;)
			{
				task_t tsk{};
				{
					std::unique_lock<std::mutex> un_lk{ data.access };

					//if (first_iteration)
					//{
					//	first_iteration = false;
					//	data.idle_threads_count -= 1;
					//}

					bool waited{ data.waiting_time_new_tasks == waiting_time_new_tasks__none || is_continue_work_thread(data, thread_index) };
					if (!waited)
					{
						assert(data.waiting_time_new_tasks != waiting_time_new_tasks__none);
						assert(!data.stop_working);
						assert(!data.tasks.tasks_is_exists(thread_index));

						data.idle_threads_count += 1;
						{
							if (is_all_running_threads_idle(data))
								data.cv.state_threads_changed.notify_all();

							waited = data.cv.queue_changed.wait_for(un_lk, data.waiting_time_new_tasks, [&] { return data.wake_up_sleep_threads || is_continue_work_thread(data, thread_index); });
						}
						data.idle_threads_count -= 1;
					}

					bool stopped_work_thread{ false };

					if (waited)
					{
						// or there is data in the queue, or need stopped this thread

						if (!data.stop_working && data.tasks.tasks_is_exists(thread_index))
						{
							tsk = data.tasks.take_next_task(thread_index);
						}
						else
						{
							// stop working this thread because queue the tasks is empty

							stopped_work_thread = true;
						}
					}
					else
					{
						// timeout: queue the tasks is empty

						assert(!data.stop_working);
						assert(!data.tasks.tasks_is_exists(thread_index));
						stopped_work_thread = true;
					}

					if (stopped_work_thread)
					{
						data.mask_stopped |= (1 << thread_index);

						if (is_all_threads_stopped(data))
							data.cv.state_threads_changed.notify_all();

						break;
					}
				}

				try
				{
					tsk(std::as_const(pool_ctx));
				}
				catch (...)
				{
                    log_except(data.logger.get(), std::current_exception(), L"Processing async task finished with error"sv);
				}
			}
		}

		static bool is_continue_work_thread(const data_t& data, std::size_t thread_index)
		{
			return (data.stop_working || data.tasks.tasks_is_exists(thread_index));
		}

		static bool is_all_threads_stopped(const data_t& data)
		{
			return (data.mask_stopped == mask_all_threads_of_stopped);
		}

		static bool is_all_running_threads_idle(const data_t& data)
		{
			std::size_t idle_threads_count{ data.idle_threads_count };

			for (threads_mask_t mask_running{ static_cast<threads_mask_t>(~data.mask_stopped) }; mask_running != 0; mask_running >>= 1)
			{
				if ((mask_running & 1) == 0)
					continue;

				if (idle_threads_count == 0)
					return false;

				idle_threads_count -= 1;
			}

			if (idle_threads_count != 0)
			{
				assert(idle_threads_count == 0);
			}

			return true;
		}

		void join_thread(std::thread& thread, std::size_t thread_number) noexcept
		{
			try
			{
				thread.join();
			}
			catch (...)
			{
                // log-error
                log_except(m_data.logger.get(), std::current_exception(), L"Finish the thread of pool is failed [number: "sv, thread_number, L']');
			}
		}

		bool wait_tasks_completion_for_impl(std::chrono::microseconds wait_time, std::unique_lock<std::mutex>& un_lk)
		{
			if (this_thread_of_pool(nullptr))
				throw promise_error{ promise_errc::deadlock }; // Waiting for the thread pool to finished from the thread in this pool

			m_data.cv.queue_changed.notify_all();

			bool waited = (wait_time == waiting_time_new_tasks__none)
				? (m_data.cv.state_threads_changed.wait(un_lk, [&] { return (is_all_threads_stopped(m_data) || is_all_running_threads_idle(m_data)); }), true)
				: (m_data.cv.state_threads_changed.wait_for(un_lk, wait_time, [&] { return (is_all_threads_stopped(m_data) || is_all_running_threads_idle(m_data)); }));

			if (!waited)
			{
                log_msg(m_data.logger.get(), L"Did not wait for the completion of all flows..."sv);
				return false;
			}

			m_data.wake_up_sleep_threads = true;
			m_data.cv.queue_changed.notify_all();
			un_lk.unlock();
			{
				bool is_repeat{ false };
				do
				{
					is_repeat = false;

					std::size_t thread_number{ 0 };
					for (auto&[thread_access, thread] : m_threads)
					{
						const std::lock_guard<std::mutex> lk{ thread_access };

						thread_number += 1;

						if (thread.joinable())
						{
							join_thread(thread, thread_number);
							is_repeat = true;
						}
					}

				} while (is_repeat);
			}
			un_lk.lock();
			m_data.wake_up_sleep_threads = false;

			return true;
		}

		void resume_threads_impl(std::size_t tasks_count)
		{
			for (std::size_t index = std::min<std::size_t>(m_data.idle_threads_count, tasks_count); index > 0; --index, --tasks_count)
			{
				m_data.cv.queue_changed.notify_one();
			}

			if (tasks_count > 0)
			{
				const std::size_t thread_count{ m_threads.size() };

				for (std::size_t index = std::min<std::size_t>(stopped_threads_count(), tasks_count); index > 0; --index)
				{
					std::size_t thread_index{ 0 };
					threads_mask_t index_by_mask{ 1 };
					for (; thread_index < thread_count; thread_index += 1, index_by_mask <<= 1)
					{
						if ((m_data.mask_stopped & index_by_mask) == 0)
							continue;

						auto&[thread_access, thread] = m_threads[thread_index];
						const std::lock_guard<std::mutex> lk{ thread_access };

						if (thread.joinable())
							join_thread(thread, thread_index + 1);

						for (std::size_t attempt = 1, attempt_count = 5; attempt <= attempt_count; ++attempt)
						{
							try
							{
								//m_data.idle_threads_count += 1;
								m_data.mask_stopped &= ~index_by_mask;
								thread = std::thread(&ondemand::thread_main, ctx_t{ this, thread_index });
								break;
							}
							catch (...)
							{
								//m_data.idle_threads_count -= 1;
								m_data.mask_stopped |= index_by_mask;

                                log_except(m_data.logger.get(), std::current_exception(), L"Start the thread of pool is failed [number: "sv, (thread_index + 1), L"] [mask-running: "sv, mask_to_wstring(static_cast<threads_mask_t>(~m_data.mask_stopped), thread_count, L'+', L'-'), L']');

                                if (attempt < attempt_count)
									std::this_thread::yield();
							}
						}
						break;
					}

					assert(thread_index < thread_count);
				}
			}
		}

		bool this_thread_of_pool(std::size_t* thread_index) const
		{
			assert(!m_threads.empty());

			threads_mask_t mask_running{ static_cast<threads_mask_t>(~m_data.mask_stopped) };
			if (0 != mask_running)
			{
				const std::thread::id this_thread_id{ std::this_thread::get_id() };

				std::size_t thrd_index{ 0 };
				threads_mask_t index_by_mask{ 1 };
				for (;; thrd_index += 1, index_by_mask <<= 1)
				{
					assert(thrd_index < m_threads.size());

					if ((mask_running & index_by_mask) != 0)
					{
						auto&[thread_access, thread] = m_threads[thrd_index];
						if (this_thread_id == thread.get_id())
						{
							if (thread_index)
								*thread_index = thrd_index;

							return true;
						}

						if (0 == (mask_running &= ~index_by_mask))
							break;
					}
				}
			}

			return false;
		}

	private:

		data_t m_data;

		std::vector<std::pair<std::mutex, std::thread>> m_threads;
	};

} // namespace async::pool_threads
