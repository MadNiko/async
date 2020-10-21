

#include "pch.h"

#include <async.hpp>


struct logger : testing::Test
{
protected: // testing::Test

    logger()
        : m_log_impl{ std::wcout }
    {}

    virtual void SetUp() override
    {
        m_log = &m_log_impl;
    }
    virtual void TearDown() override
    {
        m_log = nullptr;
    }

    void test_1()
    {}

protected:

    async::logger_wostream m_log_impl;
    async::logger* m_log;
};
TEST_F(logger, compiletime_msg_1_param)
{
    async::log_msg(m_log, L"string-view"sv);
    async::log_msg(m_log, L"string"s);
    async::log_msg(m_log, L'c');
    async::log_msg(m_log, true);
}
TEST_F(logger, compiletime_msg_2_params)
{
    async::log_msg(m_log, L"1:", L"string-view"sv);
    async::log_msg(m_log, L"2:"sv, L"string"s);
    async::log_msg(m_log, L"3:"s, L'c');
    async::log_msg(m_log, L"4:"s.data(), false);
}
TEST_F(logger, compiletime_scope_1_param)
{
    const async::log_scope log_scope_guard_0(m_log, m_log);
    const async::log_scope log_scope_guard_1(m_log, L"scope"s);
    const async::log_scope log_scope_guard_2(m_log, L"scope"sv);
}
TEST_F(logger, compiletime_scope_3_params)
{
    const async::log_scope log_scope_guard_1(m_log, L"scope"sv, L'-', ((         int)1));
    const async::log_scope log_scope_guard_2(m_log, L"scope"sv, L'-', ((  signed int)2));
    const async::log_scope log_scope_guard_3(m_log, L"scope"sv, L'-', ((unsigned int)3));
    const async::log_scope log_scope_guard_4(m_log, L"scope"sv, L'-', (  signed char)127);
    const async::log_scope log_scope_guard_5(m_log, L"scope"sv, L'-', (         short)100);
    const async::log_scope log_scope_guard_6(m_log, L"scope"sv, L'-', (  signed short)101);
    const async::log_scope log_scope_guard_7(m_log, L"scope"sv, L'-', (unsigned short)102);
    const async::log_scope log_scope_guard_8(m_log, L"scope"sv, L'-', m_log);
}
TEST_F(logger, multistrings)
{
    async::log_msg(m_log, L"string1\nstring2\nstring3"sv);
    {
        const async::log_scope log_scope_guard_1(m_log, L"scope-1"sv);
        async::log_msg(m_log, L"string4\n\tstring5\nstring6"sv);
        {
            const async::log_scope log_scope_guard_2(m_log, L"scope-2"sv);
            async::log_msg(m_log, L"string4\rstring5\r\nstring6"sv);
        }
    }
    async::log_msg(m_log, L"string7\nstring8\nstring9"sv);
}
TEST_F(logger, except_nullptr)
{
    async::log_except(m_log, nullptr, L"except"sv);
}
