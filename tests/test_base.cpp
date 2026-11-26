#include "test_support.h"
#include "base/arena.hpp"
#include "base/log.hpp"
#include "base/result.hpp"
#include "base/scheduler.hpp"
#include "base/signal.hpp"
#include "base/strings.hpp"
#include <atomic>
#include <future>
#include <mutex>

using namespace HB;

TEST(Result, ValueAndError) {
    Result<int> ok(42);
    ASSERT_TRUE(ok.ok());
    EXPECT_EQ(ok.value(), 42);
    Result<int> bad(makeError(ErrorCode::NotFound, "nope"));
    ASSERT_FALSE(bad.ok());
    EXPECT_EQ(bad.error().message, std::string("nope"));
    EXPECT_TRUE(bad.error().code == ErrorCode::NotFound);
    EXPECT_TRUE(okStatus().ok());
    EXPECT_EQ(std::string(errorCodeName(ErrorCode::Unsupported)), std::string("Unsupported"));
}

TEST(Logger, SinkAndLevelFiltering) {
    std::vector<std::string> lines;
    Logger::instance().setSink([&](LogLevel, const std::string& m) { lines.push_back(m); });
    Logger::instance().setLevel(LogLevel::Warning);
    HB_LOG_INFO("hidden");
    HB_LOG_WARN("shown " << 7);
    HB_LOG_ERROR("err");
    Logger::instance().setSink(nullptr);
    Logger::instance().setLevel(LogLevel::Off);
    ASSERT_EQ(lines.size(), size_t(2));
    EXPECT_EQ(lines[0], std::string("shown 7"));
}

TEST(Signal, ConnectEmitDisconnect) {
    Signal<int> sig;
    int sum = 0;
    auto id = sig.connect([&](int v) { sum += v; });
    sig.connect([&](int v) { sum += v * 10; });
    sig.emit(1);
    EXPECT_EQ(sum, 11);
    EXPECT_TRUE(sig.disconnect(id));
    EXPECT_FALSE(sig.disconnect(id));
    sig.emit(1);
    EXPECT_EQ(sum, 21);
    EXPECT_EQ(sig.size(), size_t(1));
}

TEST(Scheduler, RunsAllTasksAndReturnsFutures) {
    TaskScheduler s(3);
    std::atomic<int> n{0};
    for (int i = 0; i < 200; ++i) s.post([&] { ++n; });
    auto f = s.submit([] { return 6 * 7; });
    EXPECT_EQ(f.get(), 42);
    s.waitIdle();
    EXPECT_EQ(n.load(), 200);
}

TEST(Scheduler, PriorityOrderOnSingleWorker) {
    TaskScheduler s(1);
    std::mutex m;
    std::vector<int> order;
    std::promise<void> gate;
    auto gateFuture = gate.get_future().share();
    s.post([gateFuture] { gateFuture.wait(); }, TaskPriority::High);  // occupies the only worker
    s.post([&] { std::lock_guard<std::mutex> l(m); order.push_back(3); }, TaskPriority::Low);
    s.post([&] { std::lock_guard<std::mutex> l(m); order.push_back(1); }, TaskPriority::High);
    s.post([&] { std::lock_guard<std::mutex> l(m); order.push_back(2); }, TaskPriority::Normal);
    gate.set_value();
    s.waitIdle();
    ASSERT_EQ(order.size(), size_t(3));
    EXPECT_EQ(order[0], 1);
    EXPECT_EQ(order[1], 2);
    EXPECT_EQ(order[2], 3);
}

namespace { int g_alive = 0; struct Tracked { Tracked() { ++g_alive; } ~Tracked() { --g_alive; } }; }

TEST(Arena, AllocatesAlignedAndRunsDestructors) {
    {
        Arena a(64);
        for (int i = 0; i < 100; ++i) a.create<Tracked>();
        EXPECT_EQ(g_alive, 100);
        double* d = a.create<double>(1.5);
        EXPECT_EQ(reinterpret_cast<uintptr_t>(d) % alignof(double), uintptr_t(0));
        EXPECT_GT(a.blockCount(), size_t(1));
        auto* big = static_cast<char*>(a.allocate(10000, 16));
        big[9999] = 1;
    }
    EXPECT_EQ(g_alive, 0);
}

TEST(Strings, Helpers) {
    EXPECT_EQ(toLowerAscii("AbC"), std::string("abc"));
    EXPECT_TRUE(equalsIgnoreCase("DocType", "doctype"));
    EXPECT_EQ(std::string(trim("  x y \n")), std::string("x y"));
    EXPECT_EQ(splitWhitespace(" a  b\tc ").size(), size_t(3));
    EXPECT_TRUE(startsWith("file://x", "file://"));
    EXPECT_TRUE(endsWith("a.png", ".png"));
}

TEST(Utf8, RoundTripAndMalformed) {
    std::string s;
    appendUtf8(s, 0x161);   // s with caron
    appendUtf8(s, 0x20AC);
    appendUtf8(s, 0x1F600);
    auto u = utf8ToUtf32(s);
    ASSERT_EQ(u.size(), size_t(3));
    EXPECT_EQ(static_cast<uint32_t>(u[0]), 0x161u);
    EXPECT_EQ(static_cast<uint32_t>(u[2]), 0x1F600u);
    auto bad = utf8ToUtf32(std::string("a\xC3") + "b\xFF");
    EXPECT_EQ(bad.size(), size_t(4));
    EXPECT_EQ(static_cast<uint32_t>(bad[1]), 0xFFFDu);
}
