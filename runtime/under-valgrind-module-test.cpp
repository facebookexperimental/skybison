// Copyright (c) Facebook, Inc. and its affiliates. (http://www.facebook.com)
#include <cmath>

#include "gtest/gtest.h"

#include "builtins.h"
#include "test-utils.h"

namespace py {
namespace testing {

using UnderValgrindModuleTest = RuntimeFixture;

TEST_F(UnderValgrindModuleTest, UnderCallgrindDumpStatsWithNoneDoesNothing) {
  HandleScope scope(thread_);
  Object none(&scope, NoneType::object());
  runBuiltin(FUNC(_valgrind, callgrind_dump_stats), none);
}

TEST_F(UnderValgrindModuleTest, UnderCallgrindDumpStatsWithStringDoesNothing) {
  HandleScope scope(thread_);
  Object string(&scope, runtime_->newStrFromCStr("service_load"));
  runBuiltin(FUNC(_valgrind, callgrind_dump_stats), string);
}

TEST_F(UnderValgrindModuleTest, UnderCallgrindStartInstrumentationDoesNothing) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
import _valgrind
_valgrind.callgrind_start_instrumentation()
)")
                   .isError());
}

TEST_F(UnderValgrindModuleTest, UnderCallgrindStopInstrumentationDoesNothing) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
import _valgrind
_valgrind.callgrind_stop_instrumentation()
)")
                   .isError());
}

TEST_F(UnderValgrindModuleTest, UnderCallgrindZeroStatsDoesNothing) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
import _valgrind
_valgrind.callgrind_zero_stats()
)")
                   .isError());
}

}  // namespace testing
}  // namespace py
