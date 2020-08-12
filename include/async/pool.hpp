
#pragma once

#include <list>
#include <tuple>
#include <deque>
#include <vector>
#include <chrono>
#include <memory>
#include <variant>
#include <cassert>
#include <functional>


namespace async
{
    class logger;

	class pool;
	using pool_ptr = std::shared_ptr<pool>;


	class pool
	{
	public:

		/** \brief Контекст исполнения.
		 *
		 * \details Используется для оптимизации распределения задачь по потокам одного и того же пула.
		 */
		using ctx_t = std::tuple<pool*, std::size_t>;

		/** \brief Тип функции описывающий задачу для выполнения.
		 */
		using task_t = std::function<void(ctx_t)>;

		using more_tasks_t = std::list<pool::task_t>;

		using task_variant_t = std::variant<task_t, more_tasks_t>;

	public:

		/** \brief Добавить задачу для исполнения.
		 * 
		 * \details Добавление новых задачь может быть вызывано в любой момент и из любых потоков процесса:
		 *          как из \a внешних так и из \a внутренних по отношению к данному пулу.
		 * 
		 * \details Если работа пула предварительно была остановлена (метод \a pool::stop_threads) - задачи будут добавляться,
		 *          но они не будет запускаться пока работа пула не будет возабнавлена (метод \a pool::resume_threads).
		 *
		 * \param [in] this_ctx - Текущий контекст исполнения. Если не определен (например новые задачи для пула),
		 *                        в этом случае передается значение \a pool::unknown_ctx.
		 *
		 * \param [in] task     - Задача для исполнения. В момент вызова в неё будет передан текущий контекст,
		 *                        который можно передавать дальше при добавлении следующих задачь в пул.
		 */
		virtual void add_task(ctx_t this_ctx, task_t task) = 0;

		/** \brief Дождаться завершения всех задач.
		 * 
		 * \details Если работа пула остановлена то метод будет завершен без какого либо ожидания.
		 *          То есть: был вызван метод \a pool::stop_threads и после этого не был вызван метод \a pool::resume_threads.
		 *
		 * \details Если в процессе ожидания в пул будут добавлены новые задачи, то текущий вызов метода будет ожидать и их заершения.
		 * 
		 * \return Если все задачи завершены возвращается \a true, иначе \a false.
		 */
		virtual bool wait_tasks_complete() = 0;

		/** \brief Дождаться завершения всех задач.
		 *
		 * \details Если в качестве времени ожидания передать ноль (константа \a pool::wait_time_zero),
		 *          то метод вернет текущее состояние без какого либо ожидания.
		 *
		 * \details Если в процессе ожидания в пул будут добавлены новые задачи, то текущий вызов
		 *          метода будет ожидать и их заершения тоже (пока не истечет указанное время).
		 *
		 * \param [in] wait_time - Время ожидания завершения всех задачь.
		 *
		 * \return Если все задачи успели завершиться за указанное время возвращается \a true, иначе \a false.
		 */
		virtual bool wait_tasks_complete_for(std::chrono::microseconds wait_time) = 0;

	public:

		/** \drief Возобновить работу потоков пула.
		 *
		 * \note Метод не дожидается запуска потоков.
		 *
		 * \details В зависимости от реализации, поднимается необходимое колличество
		 *          потоков, после чего продолжается выполнение задачь из очереди.
		 */
		virtual void resume_threads() = 0;

		/** \drief Остановить потоки пула.
		 *
		 * \note Метод не дожидается остановки потоков.
		 *
		 * \details Текущие задачи, взятые потоками на исполнение завершат
		 *          свою работу, но новые браться на исполнение уже не будут.
		 */
		virtual void stop_threads() = 0;

		/** \drief Остановить потоки пула и дождаться их завершения.
		 *
		 * \details Текущие задачи, взятые потоками на исполнение завершат
		 *          свою работу, но новые браться на исполнение уже не будут.
		 */
		virtual void stop_threads_and_wait_them_complete() = 0;

		/** \brief Получить число потоков занятых выполнением задач.
		 *
		 * \details В зависимости от реализации пула допускается, что не все потоки в конкретный момент времени будут
		 *          заниматься выполнением задачь, некоторые из них могут простаивать, а некоторые могут быть даже не запущены.
		 * 
		 * \return Число потоков занятых выполнением задач.
		 */
		virtual std::size_t busy_threads_count() const = 0;

		/** \brief Получить максимальное число потоков пула.
		 *
		 * \details В зависимости от реализации пула потоки могут как запускаться так и останавливаться в произвольные моменты времени.
		 *          Например: - все потоки запускаются при создании пула и останавливаются при его уничтожении (или вызове метода \a pool::stop_threads);
		 *                    - потоки запускаются по необходимости и при бездействии останавливаются не сразу, а по истечении какого то времени.
		 *
		 * \return Максимальное число потоков пула.
		 */
		virtual std::size_t max_threads_count() const = 0;

    public:

        virtual logger* log() const noexcept { return nullptr; }

	public:

		virtual ~pool() = default;

	public:

		static constexpr ctx_t unknown_ctx{ nullptr, 0 };
		static constexpr std::chrono::microseconds wait_time_zero{ 0 };

	protected:

		struct tasks_t;
	};


	struct pool::tasks_t
	{
		std::deque<task_t>  queue;
		std::vector<task_t> out_of_queue_by_threads;

	public:

		std::size_t queue_size() const noexcept;
		bool queue_is_empty() const noexcept;

		void add_task_in_queue(task_t task);
		task_t take_next_task(std::size_t thread_index);

		bool tasks_is_exists(std::size_t thread_index) const;
		bool tasks_is_exists() const;

		void set_task_out_of_queue(std::size_t thread_index, task_t task);
		bool out_of_queue_is_exists(std::size_t thread_index) const;

		void move_extra_tasks_in_begin_queue();
	};


} // namespace async



namespace async
{

	[[nodiscard]] inline std::size_t pool::tasks_t::queue_size() const noexcept
	{
		return queue.size();
	}
	
	[[nodiscard]] inline bool pool::tasks_t::queue_is_empty() const noexcept
	{
		return queue.empty();
	}
	
	inline void pool::tasks_t::add_task_in_queue(task_t tsk)
	{
		assert(tsk);
		queue.push_back(std::move(tsk));
	}
	
	[[nodiscard]] inline pool::task_t pool::tasks_t::take_next_task(std::size_t thread_index)
	{
		assert(thread_index < out_of_queue_by_threads.size());

		task_t& task_out_of_queue{ out_of_queue_by_threads[thread_index] };

		assert(!queue.empty() || task_out_of_queue);

		task_t result{ nullptr };

		if (task_out_of_queue)
		{
			result.swap(task_out_of_queue);
		}
		else
		{
			result.swap(queue.front());
			queue.pop_front();
		}

		assert(result);
		assert(!task_out_of_queue);

		return result;
	}
	
	[[nodiscard]] inline bool pool::tasks_t::tasks_is_exists(std::size_t thread_index) const
	{
		return !queue_is_empty() || out_of_queue_is_exists(thread_index);
	}
	
	[[nodiscard]] inline bool pool::tasks_t::tasks_is_exists() const
	{
		if (!queue_is_empty())
			return true;

		for (const task_t& task : out_of_queue_by_threads)
		{
			if (task)
				return true;
		}

		return false;
	}
	
	inline void pool::tasks_t::set_task_out_of_queue(std::size_t thread_index, task_t tsk)
	{
		assert(tsk);
		assert(thread_index < out_of_queue_by_threads.size());
		assert(!out_of_queue_by_threads[thread_index]);

		tsk.swap(out_of_queue_by_threads[thread_index]);
	}

	[[nodiscard]] inline bool pool::tasks_t::out_of_queue_is_exists(std::size_t thread_index) const
	{
		assert(thread_index < out_of_queue_by_threads.size());
		return static_cast<bool>(out_of_queue_by_threads[thread_index]);
	}

	inline void pool::tasks_t::move_extra_tasks_in_begin_queue()
	{
		for (task_t& tsk : out_of_queue_by_threads)
		{
			if (tsk)
				queue.push_front(std::move(tsk));
		}
	}
}