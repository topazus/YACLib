#include <util/time.hpp>

#include <yaclib/algo/wait_group.hpp>
#include <yaclib/async/contract.hpp>
#include <yaclib/async/future.hpp>
#include <yaclib/async/promise.hpp>
#include <yaclib/async/run.hpp>
#include <yaclib/config.hpp>
#include <yaclib/executor/submit.hpp>
#include <yaclib/executor/thread_pool.hpp>
#include <yaclib/fault/thread.hpp>
#include <yaclib/util/intrusive_ptr.hpp>
#include <yaclib/util/result.hpp>

#include <algorithm>
#include <array>
#include <chrono>
#include <iosfwd>
#include <iterator>
#include <ratio>
#include <thread>
#include <tuple>
#include <type_traits>
#include <utility>

#include <gtest/gtest.h>

namespace {

using namespace std::chrono_literals;

template <typename T>
class WaitGroupT : public testing::Test {
 public:
  using Type = T;
};

class TypesNames {
 public:
  template <typename T>
  static std::string GetName(int i) {
    switch (i) {
      case 0:
        return "void";
      case 1:
        return "int";
      default:
        return "unknown";
    }
  }
};

using MyTypes = ::testing::Types<void, int>;
TYPED_TEST_SUITE(WaitGroupT, MyTypes, TypesNames);

void TestJustWorks() {
  auto tp = yaclib::MakeThreadPool(1);
  auto [f, p] = yaclib::MakeContract<void>();

  yaclib::WaitGroup wg;
  test::util::StopWatch timer;

  Submit(tp, [p = std::move(p)]() mutable {
    yaclib_std::this_thread::sleep_for(150ms * YACLIB_CI_SLOWDOWN);
    std::move(p).Set();
  });

  EXPECT_FALSE(f.Ready());
  wg.Add(f);
  EXPECT_LE(timer.Elapsed(), 50ms * YACLIB_CI_SLOWDOWN);
  wg.Wait();
  EXPECT_TRUE(f.Ready());

  EXPECT_LE(timer.Elapsed(), 200ms * YACLIB_CI_SLOWDOWN);

  tp->Stop();
  tp->Wait();
}

TEST(WaitGroup, JustWorks) {
  TestJustWorks();
}

template <typename T>
void TestManyWorks() {
  auto tp = yaclib::MakeThreadPool(1);

  constexpr int kSize = 10;
  std::array<yaclib::Promise<T>, kSize> promises;
  std::array<yaclib::Future<T>, kSize> futures;
  for (int i = 0; i < kSize; ++i) {
    std::tie(futures[i], promises[i]) = yaclib::MakeContract<T>();
  }

  yaclib::WaitGroup wg;
  test::util::StopWatch timer;

  for (int i = 0; i < kSize; ++i) {
    Submit(tp, [p = std::move(promises[i])]() mutable {
      yaclib_std::this_thread::sleep_for(150ms * YACLIB_CI_SLOWDOWN);
      if constexpr (std::is_void_v<T>) {
        std::move(p).Set();
      } else {
        std::move(p).Set(5);
      }
    });
  }

  for (int i = 0; i < kSize; ++i) {
    EXPECT_FALSE(futures[i].Ready());
  }
  for (int i = 0; i < kSize / 2; ++i) {
    wg.Add(futures[i]);
  }
  wg.Wait();
  for (int i = 0; i < kSize / 2; ++i) {
    EXPECT_TRUE(futures[i].Ready());
  }
  for (int i = kSize / 2; i < kSize; ++i) {
    EXPECT_FALSE(futures[i].Ready());
  }

  for (int i = kSize / 2; i < kSize; ++i) {
    wg.Add(futures[i]);
  }

  wg.Wait();

  for (int i = kSize / 2; i < kSize; ++i) {
    EXPECT_TRUE(futures[i].Ready());
  }

  EXPECT_LE(timer.Elapsed(), kSize * 200ms * YACLIB_CI_SLOWDOWN);

  tp->Stop();
  tp->Wait();
}

TYPED_TEST(WaitGroupT, ManyWorks) {
  TestManyWorks<typename TestFixture::Type>();
}

template <typename T>
void TestGetWorks() {
  auto tp = yaclib::MakeThreadPool(1);
  auto [f, p] = yaclib::MakeContract<T>();

  yaclib::WaitGroup wg;
  test::util::StopWatch timer;

  Submit(tp, [p = std::move(p)]() mutable {
    yaclib_std::this_thread::sleep_for(150ms * YACLIB_CI_SLOWDOWN);
    if constexpr (std::is_void_v<T>) {
      std::move(p).Set();
    } else {
      std::move(p).Set(5);
    }
  });

  EXPECT_FALSE(f.Ready());
  wg.Add(f);
  EXPECT_LE(timer.Elapsed(), 50ms * YACLIB_CI_SLOWDOWN);
  wg.Wait();
  EXPECT_TRUE(f.Ready());

  if constexpr (!std::is_void_v<T>) {
    EXPECT_EQ(std::move(f).Get().Ok(), 5);
  }

  EXPECT_LE(timer.Elapsed(), 200ms * YACLIB_CI_SLOWDOWN);

  tp->Stop();
  tp->Wait();
}

TYPED_TEST(WaitGroupT, GetWorksVoid) {
  TestGetWorks<typename TestFixture::Type>();
}

template <typename T>
void TestCallbackWorks() {
  auto tp = yaclib::MakeThreadPool(1);
  auto [f, p] = yaclib::MakeContract<T>();

  yaclib::WaitGroup wg;
  test::util::StopWatch timer;

  Submit(tp, [p = std::move(p)]() mutable {
    yaclib_std::this_thread::sleep_for(150ms * YACLIB_CI_SLOWDOWN);
    if constexpr (std::is_void_v<T>) {
      std::move(p).Set();
    } else {
      std::move(p).Set(5);
    }
  });

  EXPECT_FALSE(f.Ready());
  wg.Add(f);
  EXPECT_LE(timer.Elapsed(), 50ms * YACLIB_CI_SLOWDOWN);
  wg.Wait();
  EXPECT_TRUE(f.Ready());

  bool called = false;

  if constexpr (std::is_void_v<T>) {
    std::move(f).SubscribeInline([&called]() {
      called = true;
    });
  } else {
    std::move(f).SubscribeInline([&called](int result) {
      EXPECT_EQ(result, 5);
      called = true;
    });
  }

  EXPECT_TRUE(called);

  EXPECT_LE(timer.Elapsed(), 200ms * YACLIB_CI_SLOWDOWN);

  tp->Stop();
  tp->Wait();
}

TYPED_TEST(WaitGroupT, CallbackWorksVoid) {
  TestCallbackWorks<typename TestFixture::Type>();
}

void TestMultithreaded() {
  constexpr int kThreads = 4;
  auto tp = yaclib::MakeThreadPool(kThreads);
  yaclib::Future<int> fs[kThreads];

  for (int i = 0; i < kThreads; ++i) {
    fs[i] = Run(tp, [i] {
      yaclib_std::this_thread::sleep_for(50ms * YACLIB_CI_SLOWDOWN);
      return i;
    });
  }

  test::util::StopWatch timer;
  yaclib::WaitGroup wg;

  for (auto& f : fs) {
    wg.Add(f);
  }
  wg.Wait();

  EXPECT_LE(timer.Elapsed(), 100ms * YACLIB_CI_SLOWDOWN);
  EXPECT_TRUE(std::all_of(std::begin(fs), std::end(fs), [](auto& f) {
    return f.Ready();
  }));

  for (int i = 0; i < kThreads; ++i) {
    EXPECT_EQ(std::move(fs[i]).Get().Value(), i);
  }
  tp->Stop();
  tp->Wait();
}

TEST(WaitGroup, Multithreaded) {
  TestMultithreaded();
}

TEST(WaitGroup, Empty) {
  yaclib::WaitGroup wg;
  wg.Wait();
}

}  // namespace
