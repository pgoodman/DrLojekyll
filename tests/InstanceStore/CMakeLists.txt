# Copyright 2026, Peter Goodman. All rights reserved.

add_executable(instance_store_test
  InstanceStoreTest.cpp)

target_link_libraries(instance_store_test
  PRIVATE
    settings_private
    settings_public
    drtest
    Runtime)   # Allocator.cpp: MallocAllocator / Arena / ArenaAllocator.

add_test(
  NAME InstanceStore
  COMMAND instance_store_test)
