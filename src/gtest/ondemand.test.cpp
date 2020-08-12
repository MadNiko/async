

#include "pch.h"

#include <async.hpp>


namespace
{
    const std::chrono::microseconds waiting_time_new_tasks{ async::pool_threads::ondemand::waiting_time_new_tasks_default() };
}


struct ondemand : testing::Test
{
protected:

    virtual void SetUp() override
    {
        m_manager = async::make_manager<async::pool_threads::ondemand>(hardware_thread_count, waiting_time_new_tasks, nullptr);
    }
    virtual void TearDown() override
    {
        m_manager = async::manager{};
    }

protected:

    async::manager m_manager;
};
TEST_F(ondemand, 1)
{
    EXPECT_EQ(hardware_thread_count, m_manager.max_threads_count());
    EXPECT_EQ(0, m_manager.busy_threads_count());
}

TEST_F(ondemand, 2)
{
    EXPECT_EQ(hardware_thread_count, m_manager.max_threads_count());
    EXPECT_EQ(0, m_manager.busy_threads_count());
}