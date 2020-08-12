
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

		/** \brief �������� ����������.
		 *
		 * \details ������������ ��� ����������� ������������� ������ �� ������� ������ � ���� �� ����.
		 */
		using ctx_t = std::tuple<pool*, std::size_t>;

		/** \brief ��� ������� ����������� ������ ��� ����������.
		 */
		using task_t = std::function<void(ctx_t)>;

		using more_tasks_t = std::list<pool::task_t>;

		using task_variant_t = std::variant<task_t, more_tasks_t>;

	public:

		/** \brief �������� ������ ��� ����������.
		 * 
		 * \details ���������� ����� ������ ����� ���� �������� � ����� ������ � �� ����� ������� ��������:
		 *          ��� �� \a ������� ��� � �� \a ���������� �� ��������� � ������� ����.
		 * 
		 * \details ���� ������ ���� �������������� ���� ����������� (����� \a pool::stop_threads) - ������ ����� �����������,
		 *          �� ��� �� ����� ����������� ���� ������ ���� �� ����� ������������ (����� \a pool::resume_threads).
		 *
		 * \param [in] this_ctx - ������� �������� ����������. ���� �� ��������� (�������� ����� ������ ��� ����),
		 *                        � ���� ������ ���������� �������� \a pool::unknown_ctx.
		 *
		 * \param [in] task     - ������ ��� ����������. � ������ ������ � �� ����� ������� ������� ��������,
		 *                        ������� ����� ���������� ������ ��� ���������� ��������� ������ � ���.
		 */
		virtual void add_task(ctx_t this_ctx, task_t task) = 0;

		/** \brief ��������� ���������� ���� �����.
		 * 
		 * \details ���� ������ ���� ����������� �� ����� ����� �������� ��� ������ ���� ��������.
		 *          �� ����: ��� ������ ����� \a pool::stop_threads � ����� ����� �� ��� ������ ����� \a pool::resume_threads.
		 *
		 * \details ���� � �������� �������� � ��� ����� ��������� ����� ������, �� ������� ����� ������ ����� ������� � �� ���������.
		 * 
		 * \return ���� ��� ������ ��������� ������������ \a true, ����� \a false.
		 */
		virtual bool wait_tasks_complete() = 0;

		/** \brief ��������� ���������� ���� �����.
		 *
		 * \details ���� � �������� ������� �������� �������� ���� (��������� \a pool::wait_time_zero),
		 *          �� ����� ������ ������� ��������� ��� ������ ���� ��������.
		 *
		 * \details ���� � �������� �������� � ��� ����� ��������� ����� ������, �� ������� �����
		 *          ������ ����� ������� � �� ��������� ���� (���� �� ������� ��������� �����).
		 *
		 * \param [in] wait_time - ����� �������� ���������� ���� ������.
		 *
		 * \return ���� ��� ������ ������ ����������� �� ��������� ����� ������������ \a true, ����� \a false.
		 */
		virtual bool wait_tasks_complete_for(std::chrono::microseconds wait_time) = 0;

	public:

		/** \drief ����������� ������ ������� ����.
		 *
		 * \note ����� �� ���������� ������� �������.
		 *
		 * \details � ����������� �� ����������, ����������� ����������� �����������
		 *          �������, ����� ���� ������������ ���������� ������ �� �������.
		 */
		virtual void resume_threads() = 0;

		/** \drief ���������� ������ ����.
		 *
		 * \note ����� �� ���������� ��������� �������.
		 *
		 * \details ������� ������, ������ �������� �� ���������� ��������
		 *          ���� ������, �� ����� ������� �� ���������� ��� �� �����.
		 */
		virtual void stop_threads() = 0;

		/** \drief ���������� ������ ���� � ��������� �� ����������.
		 *
		 * \details ������� ������, ������ �������� �� ���������� ��������
		 *          ���� ������, �� ����� ������� �� ���������� ��� �� �����.
		 */
		virtual void stop_threads_and_wait_them_complete() = 0;

		/** \brief �������� ����� ������� ������� ����������� �����.
		 *
		 * \details � ����������� �� ���������� ���� �����������, ��� �� ��� ������ � ���������� ������ ������� �����
		 *          ���������� ����������� ������, ��������� �� ��� ����� �����������, � ��������� ����� ���� ���� �� ��������.
		 * 
		 * \return ����� ������� ������� ����������� �����.
		 */
		virtual std::size_t busy_threads_count() const = 0;

		/** \brief �������� ������������ ����� ������� ����.
		 *
		 * \details � ����������� �� ���������� ���� ������ ����� ��� ����������� ��� � ��������������� � ������������ ������� �������.
		 *          ��������: - ��� ������ ����������� ��� �������� ���� � ��������������� ��� ��� ����������� (��� ������ ������ \a pool::stop_threads);
		 *                    - ������ ����������� �� ������������� � ��� ����������� ��������������� �� �����, � �� ��������� ������ �� �������.
		 *
		 * \return ������������ ����� ������� ����.
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