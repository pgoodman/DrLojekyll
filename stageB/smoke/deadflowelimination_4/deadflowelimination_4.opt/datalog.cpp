// Auto-generated file; do not edit.

#include "datalog.h"

#include <tuple>
#include <utility>

Database::Database(::hyde::rt::Allocator allocator_, DatabaseLog &log_, DatabaseFunctors &functors_)
    : allocator(allocator_),
      log(log_),
      functors(functors_),
      out_neg_5(allocator_),
      table_18(allocator_),
      table_21(allocator_) {
  init_4();
}

bool Database::init_4() {
  ::hyde::rt::Vec<Tup_i32> vec17(allocator);
  proc_8(std::move(vec17));
  return false;
}

bool Database::proc_8(::hyde::rt::Vec<Tup_i32> vec10) {
  for (auto [v12] : vec10) {
    out_neg_5.TryChangeAbsentToPresent({v12});
  }
  vec10.Clear();
  flow_24();
  return false;
}

bool Database::in_1(::hyde::rt::Vec<Tup_i32> vec14) {
  proc_8(std::move(vec14));
  return true;
}

bool Database::flow_24() {
  g9 += 1;
  return true;
}

Database::out_neg_f_cursor Database::out_neg_f() {
  return {*this, 0};
}

bool Database::out_neg_f_cursor::next(int32_t &A) {
  while (pos < db.out_neg_5.NumRows()) {
    const uint32_t id = pos++;
    const auto row = db.out_neg_5.RowAt(id);
    if (db.out_neg_5.StateAt(id) != ::hyde::rt::TupleState::kPresent) {
      continue;
    }
    A = row.a;
    return true;
  }
  return false;
}

Database::out_pos_f_cursor Database::out_pos_f() {
  return {*this, 0};
}

bool Database::out_pos_f_cursor::next(int32_t &A) {
  while (pos < db.table_18.NumRows()) {
    const uint32_t id = pos++;
    const auto row = db.table_18.RowAt(id);
    if (db.table_18.StateAt(id) != ::hyde::rt::TupleState::kPresent) {
      continue;
    }
    A = row.c0;
    return true;
  }
  return false;
}

Database::out_chain_f_cursor Database::out_chain_f() {
  return {*this, 0};
}

bool Database::out_chain_f_cursor::next(int32_t &A) {
  while (pos < db.table_21.NumRows()) {
    const uint32_t id = pos++;
    const auto row = db.table_21.RowAt(id);
    if (db.table_21.StateAt(id) != ::hyde::rt::TupleState::kPresent) {
      continue;
    }
    A = row.c0;
    return true;
  }
  return false;
}

