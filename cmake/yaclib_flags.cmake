set(YACLIB_ASAN 0)
set(YACLIB_UBSAN 0)
set(YACLIB_LSAN 0)
set(YACLIB_TSAN 0)
set(YACLIB_MEMSAN 0)

if (ASAN IN_LIST YACLIB_FLAGS)
  set(YACLIB_SAN 1)
  set(YACLIB_ASAN 1)
  include(yaclib_asan)
endif ()
if (UBSAN IN_LIST YACLIB_FLAGS)
  set(YACLIB_SAN 1)
  set(YACLIB_UBSAN 1)
  include(yaclib_ubsan)
endif ()
if (LSAN IN_LIST YACLIB_FLAGS)
  set(YACLIB_SAN 1)
  set(YACLIB_LSAN 1)
  include(yaclib_lsan)
endif ()
if (TSAN IN_LIST YACLIB_FLAGS)
  set(YACLIB_SAN 1)
  set(YACLIB_TSAN 1)
  include(yaclib_tsan)
endif ()
if (MEMSAN IN_LIST YACLIB_FLAGS)
  set(YACLIB_SAN 1)
  set(YACLIB_MEMSAN 1)
  include(yaclib_memsan)
endif ()
if (YACLIB_SAN AND NOT CMAKE_CXX_COMPILER_ID STREQUAL "MSVC" AND NOT CMAKE_CXX_SIMULATE_ID STREQUAL "MSVC")
  # Nicer stack trace and recover
  list(APPEND YACLIB_COMPILE_OPTIONS -fno-omit-frame-pointer -fsanitize-recover=all)
endif ()

set(YACLIB_COVERAGE 0)
if (COVERAGE IN_LIST YACLIB_FLAGS)
  set(YACLIB_COVERAGE 1)
  list(APPEND YACLIB_LINK_OPTIONS --coverage)
  list(APPEND YACLIB_COMPILE_OPTIONS --coverage)
  list(APPEND YACLIB_DEFINITIONS NDEBUG)
endif ()

set(YACLIB_CORO_NEED 0)
if (CORO IN_LIST YACLIB_FLAGS)
  include(yaclib_coro)
endif ()

set(YACLIB_FUTEX 0) # TODO Fucking atomic::wait bugged
#[[
if (DISABLE_FUTEX IN_LIST YACLIB_FLAGS)
  set(YACLIB_FUTEX 0)
elseif (DISABLE_UNSAFE_FUTEX IN_LIST YACLIB_FLAGS OR YACLIB_FAULT STREQUAL "FIBER")
  # Our fiber fault injection atomic rely on `*this` in `notify_one`
  set(YACLIB_FUTEX 1)
else ()
  # It's safe if we need only address for notify
  set(YACLIB_FUTEX 2)
endif ()
]]

if (DISABLE_SYMMETRIC_TRANSFER IN_LIST YACLIB_FLAGS)
  set(YACLIB_SYMMETRIC_TRANSFER 0)
  set(YACLIB_FINAL_SUSPEND_TRANSFER 0)
elseif (DISABLE_FINAL_SUSPEND_TRANSFER IN_LIST YACLIB_FLAGS)
  set(YACLIB_SYMMETRIC_TRANSFER 1)
  set(YACLIB_FINAL_SUSPEND_TRANSFER 0)
else ()
  set(YACLIB_SYMMETRIC_TRANSFER 1)
  set(YACLIB_FINAL_SUSPEND_TRANSFER 1)
endif ()

if (ERROR IN_LIST YACLIB_LOG)
  list(APPEND YACLIB_DEFINITIONS YACLIB_LOG_ERROR)
endif ()
if (WARN IN_LIST YACLIB_LOG)
  list(APPEND YACLIB_DEFINITIONS YACLIB_LOG_WARN)
endif ()
if (DEBUG IN_LIST YACLIB_LOG)
  list(APPEND YACLIB_DEFINITIONS YACLIB_LOG_DEBUG)
endif ()

