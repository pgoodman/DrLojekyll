// Auto-generated file; do not edit.

#pragma once

#include <drlojekyll/Runtime/Allocator.h>
#include <drlojekyll/Runtime/Hash.h>
#include <drlojekyll/Runtime/Table.h>
#include <drlojekyll/Runtime/Vec.h>

#include <cstdint>
#include <optional>
#include <tuple>
#include <vector>

struct Tup_b {
  bool c0;
  auto operator<=>(const Tup_b &) const noexcept = default;
};

struct Tup_i32 {
  int32_t c0;
  auto operator<=>(const Tup_i32 &) const noexcept = default;
};

struct Tup_i32_i32 {
  int32_t c0;
  int32_t c1;
  auto operator<=>(const Tup_i32_i32 &) const noexcept = default;
};

using pin_input = Tup_i32_i32;
using feed_input = Tup_i32;

// Rows of `table_7`.
struct Row7 {
  int32_t c0;
  bool c1;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c0, c1);
  }
  bool operator==(const Row7 &) const noexcept = default;
};

// Key of `idx_98` over `table_7`.
struct Key98 {
  bool c1;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c1);
  }
  bool operator==(const Key98 &) const noexcept = default;
};

// Rows of `__11` (/0).
struct Row11 {
  bool c0;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c0);
  }
  bool operator==(const Row11 &) const noexcept = default;
};

// Rows of `table_14`.
struct Row14 {
  int32_t a;
  int32_t c1;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(a, c1);
  }
  bool operator==(const Row14 &) const noexcept = default;
};

// Key of `idx_308` over `table_14`.
struct Key308 {
  int32_t a;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(a);
  }
  bool operator==(const Key308 &) const noexcept = default;
};

// Rows of `table_18`.
struct Row18 {
  int32_t a;
  int32_t c1;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(a, c1);
  }
  bool operator==(const Row18 &) const noexcept = default;
};

// Key of `idx_269` over `table_18`.
struct Key269 {
  int32_t a;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(a);
  }
  bool operator==(const Key269 &) const noexcept = default;
};

// Rows of `table_22`.
struct Row22 {
  int32_t c;
  int32_t c1;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c, c1);
  }
  bool operator==(const Row22 &) const noexcept = default;
};

// Key of `idx_223` over `table_22`.
struct Key223 {
  int32_t c;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c);
  }
  bool operator==(const Key223 &) const noexcept = default;
};

// Rows of `table_26`.
struct Row26 {
  bool c0;
  int32_t c1;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c0, c1);
  }
  bool operator==(const Row26 &) const noexcept = default;
};

// Key of `idx_173` over `table_26`.
struct Key173 {
  int32_t c1;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c1);
  }
  bool operator==(const Key173 &) const noexcept = default;
};

// Rows of `table_30`.
struct Row30 {
  int32_t a;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(a);
  }
  bool operator==(const Row30 &) const noexcept = default;
};

// Rows of `table_33`.
struct Row33 {
  int32_t a;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(a);
  }
  bool operator==(const Row33 &) const noexcept = default;
};

// Rows of `table_36`.
struct Row36 {
  int32_t a;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(a);
  }
  bool operator==(const Row36 &) const noexcept = default;
};

// Rows of `table_39`.
struct Row39 {
  int32_t c0;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c0);
  }
  bool operator==(const Row39 &) const noexcept = default;
};

// Rows of `table_42`.
struct Row42 {
  bool c0;
  int32_t c;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c0, c);
  }
  bool operator==(const Row42 &) const noexcept = default;
};

// Rows of `out7_46` (out7/1).
struct Row46 {
  int32_t a;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(a);
  }
  bool operator==(const Row46 &) const noexcept = default;
};

// Rows of `outdup_49` (outdup/1).
struct Row49 {
  int32_t a;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(a);
  }
  bool operator==(const Row49 &) const noexcept = default;
};

// Rows of `outflag_52` (outflag/1).
struct Row52 {
  int32_t a;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(a);
  }
  bool operator==(const Row52 &) const noexcept = default;
};

// Rows of `table_55`.
struct Row55 {
  int32_t a;
  int32_t c1;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(a, c1);
  }
  bool operator==(const Row55 &) const noexcept = default;
};

// Rows of `table_59`.
struct Row59 {
  int32_t c;
  int32_t c1;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c, c1);
  }
  bool operator==(const Row59 &) const noexcept = default;
};

// Rows of `table_63`.
struct Row63 {
  int32_t a;
  int32_t c1;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(a, c1);
  }
  bool operator==(const Row63 &) const noexcept = default;
};

// Rows of `table_67`.
struct Row67 {
  int32_t c;
  int32_t c1;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c, c1);
  }
  bool operator==(const Row67 &) const noexcept = default;
};

// Rows of `table_71`.
struct Row71 {
  int32_t c;
  int32_t c1;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c, c1);
  }
  bool operator==(const Row71 &) const noexcept = default;
};

// Rows of `table_75`.
struct Row75 {
  int32_t c;
  int32_t c1;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c, c1);
  }
  bool operator==(const Row75 &) const noexcept = default;
};

// User-provided functors. Define the declared member functions in
// your own translation unit; the generated code calls them.
struct DatabaseFunctors {
};

// Receives published messages. The default methods do nothing.
struct DatabaseLog {
};

class Database {
 public:
  explicit Database(::hyde::rt::Allocator allocator_, DatabaseLog &log_, DatabaseFunctors &functors_);

  // Message `pin/2`.
  bool pin_2(::hyde::rt::Vec<Tup_i32_i32> vec123);

  // Message `feed/1`.
  bool feed_1(::hyde::rt::Vec<Tup_i32> vec127);

  // Query `out7/1` (f).
  struct out7_f_cursor {
    Database &db;
    uint32_t pos;
    bool next(int32_t &A);
  };
  out7_f_cursor out7_f();

  // Query `outdup/1` (f).
  struct outdup_f_cursor {
    Database &db;
    uint32_t pos;
    bool next(int32_t &A);
  };
  outdup_f_cursor outdup_f();

  // Query `outflag/1` (f).
  struct outflag_f_cursor {
    Database &db;
    uint32_t pos;
    bool next(int32_t &A);
  };
  outflag_f_cursor outflag_f();

 private:
  bool init_4();
  bool proc_79(::hyde::rt::Vec<Tup_i32_i32> vec82, ::hyde::rt::Vec<Tup_i32> vec102);
  bool find_86(int32_t v87, int32_t v88);
  bool find_90(int32_t v91, int32_t v92);
  bool find_94(int32_t v95, int32_t v96);
  bool find_105(int32_t v106, int32_t v107);
  bool find_109(int32_t v110, int32_t v111);
  bool find_113(int32_t v114, int32_t v115);
  bool find_133(int32_t v134);
  bool find_135(int32_t v136);
  bool find_137(int32_t v138);
  bool find_139(int32_t v140);
  bool find_143(int32_t v144);
  bool find_146(int32_t v147);
  bool find_149(int32_t v150);
  bool find_152(int32_t v153);
  bool find_156(int32_t v157);
  bool find_159(int32_t v160);
  bool find_162(int32_t v163);
  bool find_167(int32_t v168);
  bool find_170(int32_t v171);
  bool find_177(bool v178, int32_t v179);
  bool find_181(bool v182, int32_t v183);
  bool find_186(int32_t v187, bool v188);
  bool find_190(bool v191);
  bool find_193(bool v194);
  bool find_196(bool v197);
  bool find_201(bool v202);
  bool find_207(bool v208, int32_t v209);
  bool find_211(bool v212, int32_t v213);
  bool find_216(bool v217, int32_t v218);
  bool find_220(int32_t v221);
  bool find_227(int32_t v228, int32_t v229);
  bool find_231(int32_t v232, int32_t v233);
  bool find_236(int32_t v237, int32_t v238);
  bool find_241(int32_t v242);
  bool find_244(int32_t v245);
  bool find_248(int32_t v249);
  bool find_252(int32_t v253);
  bool find_255(int32_t v256);
  bool find_258(int32_t v259);
  bool find_263(int32_t v264);
  bool find_266(int32_t v267);
  bool find_273(int32_t v274, int32_t v275);
  bool find_277(int32_t v278, int32_t v279);
  bool find_282(int32_t v283, int32_t v284);
  bool find_287(int32_t v288);
  bool find_291(int32_t v292);
  bool find_294(int32_t v295);
  bool find_297(int32_t v298);
  bool find_302(int32_t v303);
  bool find_305(int32_t v306);
  bool find_312(int32_t v313, int32_t v314);
  bool find_316(int32_t v317, int32_t v318);
  bool find_321(int32_t v322, int32_t v323);
  bool flow_326(::hyde::rt::Vec<Tup_b> vec81);

  ::hyde::rt::Allocator allocator;
  [[maybe_unused]] DatabaseLog &log;
  [[maybe_unused]] DatabaseFunctors &functors;

  ::hyde::rt::Table<Row7> table_7;
  ::hyde::rt::Index<Key98> idx_98;
  ::hyde::rt::Table<Row11> __11;
  ::hyde::rt::Table<Row14> table_14;
  ::hyde::rt::Index<Key308> idx_308;
  ::hyde::rt::Table<Row18> table_18;
  ::hyde::rt::Index<Key269> idx_269;
  ::hyde::rt::Table<Row22> table_22;
  ::hyde::rt::Index<Key223> idx_223;
  ::hyde::rt::Table<Row26> table_26;
  ::hyde::rt::Index<Key173> idx_173;
  ::hyde::rt::Table<Row30> table_30;
  ::hyde::rt::Table<Row33> table_33;
  ::hyde::rt::Table<Row36> table_36;
  ::hyde::rt::Table<Row39> table_39;
  ::hyde::rt::Table<Row42> table_42;
  ::hyde::rt::Table<Row46> out7_46;
  ::hyde::rt::Table<Row49> outdup_49;
  ::hyde::rt::Table<Row52> outflag_52;
  ::hyde::rt::Table<Row55> table_55;
  ::hyde::rt::Table<Row59> table_59;
  ::hyde::rt::Table<Row63> table_63;
  ::hyde::rt::Table<Row67> table_67;
  ::hyde::rt::Table<Row71> table_71;
  ::hyde::rt::Table<Row75> table_75;

  uint64_t g80 = 0;
};

