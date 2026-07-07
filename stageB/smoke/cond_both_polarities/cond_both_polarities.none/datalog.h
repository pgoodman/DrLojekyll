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

using add_r_input = Tup_i32;
using add_s_input = Tup_i32;

// Rows of `table_5`.
struct Row5 {
  int32_t x;
  bool c1;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(x, c1);
  }
  bool operator==(const Row5 &) const noexcept = default;
};

// Key of `idx_133` over `table_5`.
struct Key133 {
  bool c1;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c1);
  }
  bool operator==(const Key133 &) const noexcept = default;
};

// Rows of `__9` (/0).
struct Row9 {
  bool c0;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c0);
  }
  bool operator==(const Row9 &) const noexcept = default;
};

// Rows of `table_12`.
struct Row12 {
  int32_t x;
  bool c1;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(x, c1);
  }
  bool operator==(const Row12 &) const noexcept = default;
};

// Key of `idx_137` over `table_12`.
struct Key137 {
  bool c1;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c1);
  }
  bool operator==(const Key137 &) const noexcept = default;
};

// Rows of `table_16`.
struct Row16 {
  int32_t x;
  bool c1;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(x, c1);
  }
  bool operator==(const Row16 &) const noexcept = default;
};

// Key of `idx_141` over `table_16`.
struct Key141 {
  bool c1;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c1);
  }
  bool operator==(const Key141 &) const noexcept = default;
};

// Rows of `table_20`.
struct Row20 {
  int32_t x;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(x);
  }
  bool operator==(const Row20 &) const noexcept = default;
};

// Rows of `table_23`.
struct Row23 {
  int32_t x;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(x);
  }
  bool operator==(const Row23 &) const noexcept = default;
};

// Rows of `table_26`.
struct Row26 {
  int32_t x;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(x);
  }
  bool operator==(const Row26 &) const noexcept = default;
};

// Rows of `table_29`.
struct Row29 {
  int32_t x;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(x);
  }
  bool operator==(const Row29 &) const noexcept = default;
};

// Rows of `table_32`.
struct Row32 {
  bool c0;
  int32_t x;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c0, x);
  }
  bool operator==(const Row32 &) const noexcept = default;
};

// Key of `idx_527` over `table_32`.
struct Key527 {
  int32_t x;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(x);
  }
  bool operator==(const Key527 &) const noexcept = default;
};

// Rows of `table_36`.
struct Row36 {
  int32_t x;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(x);
  }
  bool operator==(const Row36 &) const noexcept = default;
};

// Rows of `table_39`.
struct Row39 {
  bool c0;
  int32_t x;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c0, x);
  }
  bool operator==(const Row39 &) const noexcept = default;
};

// Key of `idx_490` over `table_39`.
struct Key490 {
  int32_t x;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(x);
  }
  bool operator==(const Key490 &) const noexcept = default;
};

// Rows of `table_43`.
struct Row43 {
  bool c0;
  int32_t x;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c0, x);
  }
  bool operator==(const Row43 &) const noexcept = default;
};

// Key of `idx_440` over `table_43`.
struct Key440 {
  int32_t x;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(x);
  }
  bool operator==(const Key440 &) const noexcept = default;
};

// Rows of `table_47`.
struct Row47 {
  bool c0;
  int32_t x;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c0, x);
  }
  bool operator==(const Row47 &) const noexcept = default;
};

// Key of `idx_294` over `table_47`.
struct Key294 {
  int32_t x;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(x);
  }
  bool operator==(const Key294 &) const noexcept = default;
};

// Rows of `table_51`.
struct Row51 {
  bool c0;
  int32_t x;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c0, x);
  }
  bool operator==(const Row51 &) const noexcept = default;
};

// Key of `idx_262` over `table_51`.
struct Key262 {
  int32_t x;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(x);
  }
  bool operator==(const Key262 &) const noexcept = default;
};

// Rows of `table_55`.
struct Row55 {
  int32_t x;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(x);
  }
  bool operator==(const Row55 &) const noexcept = default;
};

// Rows of `table_58`.
struct Row58 {
  int32_t x;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(x);
  }
  bool operator==(const Row58 &) const noexcept = default;
};

// Rows of `table_61`.
struct Row61 {
  int32_t x;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(x);
  }
  bool operator==(const Row61 &) const noexcept = default;
};

// Rows of `table_64`.
struct Row64 {
  int32_t x;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(x);
  }
  bool operator==(const Row64 &) const noexcept = default;
};

// Rows of `table_67`.
struct Row67 {
  bool c0;
  int32_t c1;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c0, c1);
  }
  bool operator==(const Row67 &) const noexcept = default;
};

// Rows of `table_71`.
struct Row71 {
  bool c0;
  int32_t c1;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c0, c1);
  }
  bool operator==(const Row71 &) const noexcept = default;
};

// Rows of `pos_neg_75` (pos_neg/1).
struct Row75 {
  int32_t x;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(x);
  }
  bool operator==(const Row75 &) const noexcept = default;
};

// Rows of `both_pos_78` (both_pos/1).
struct Row78 {
  int32_t x;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(x);
  }
  bool operator==(const Row78 &) const noexcept = default;
};

// Rows of `mixed_81` (mixed/1).
struct Row81 {
  int32_t x;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(x);
  }
  bool operator==(const Row81 &) const noexcept = default;
};

// Rows of `table_84`.
struct Row84 {
  int32_t x;
  bool c1;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(x, c1);
  }
  bool operator==(const Row84 &) const noexcept = default;
};

// Key of `idx_98` over `table_84`.
struct Key98 {
  bool c1;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c1);
  }
  bool operator==(const Key98 &) const noexcept = default;
};

// Rows of `table_88`.
struct Row88 {
  int32_t x;
  bool c1;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(x, c1);
  }
  bool operator==(const Row88 &) const noexcept = default;
};

// Key of `idx_106` over `table_88`.
struct Key106 {
  bool c1;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c1);
  }
  bool operator==(const Key106 &) const noexcept = default;
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

  // Message `set_c1/1`.
  bool set_c1_1(::hyde::rt::Vec<Tup_i32> vec202, ::hyde::rt::Vec<Tup_i32> vec203);

  // Message `set_c2/1`.
  bool set_c2_1(::hyde::rt::Vec<Tup_i32> vec209, ::hyde::rt::Vec<Tup_i32> vec210);

  // Message `add_r/1`.
  bool add_r_1(::hyde::rt::Vec<Tup_i32> vec216);

  // Message `add_s/1`.
  bool add_s_1(::hyde::rt::Vec<Tup_i32> vec222);

  // Query `pos_neg/1` (f).
  struct pos_neg_f_cursor {
    Database &db;
    uint32_t pos;
    bool next(int32_t &X);
  };
  pos_neg_f_cursor pos_neg_f();

  // Query `both_pos/1` (f).
  struct both_pos_f_cursor {
    Database &db;
    uint32_t pos;
    bool next(int32_t &X);
  };
  both_pos_f_cursor both_pos_f();

  // Query `mixed/1` (f).
  struct mixed_f_cursor {
    Database &db;
    uint32_t pos;
    bool next(int32_t &X);
  };
  mixed_f_cursor mixed_f();

 private:
  bool init_4();
  bool proc_92(::hyde::rt::Vec<Tup_i32> vec94, ::hyde::rt::Vec<Tup_i32> vec95, ::hyde::rt::Vec<Tup_i32> vec145, ::hyde::rt::Vec<Tup_i32> vec146, ::hyde::rt::Vec<Tup_i32> vec178, ::hyde::rt::Vec<Tup_i32> vec181);
  bool find_102(int32_t v103, bool v104);
  bool find_110(int32_t v111, bool v112);
  bool find_123(bool v124);
  bool find_130(bool v131);
  bool find_232(int32_t v233);
  bool find_234(int32_t v235);
  bool find_236(int32_t v237);
  bool find_238(int32_t v239);
  bool find_242(int32_t v243);
  bool find_245(int32_t v246);
  bool find_248(int32_t v249);
  bool find_251(int32_t v252);
  bool find_256(int32_t v257);
  bool find_259(int32_t v260);
  bool find_266(bool v267, int32_t v268);
  bool find_270(bool v271, int32_t v272);
  bool find_275(int32_t v276, bool v277);
  bool find_280(int32_t v281);
  bool find_283(int32_t v284);
  bool find_288(int32_t v289);
  bool find_291(int32_t v292);
  bool find_298(bool v299, int32_t v300);
  bool find_302(bool v303, int32_t v304);
  bool find_307(int32_t v308, bool v309);
  bool find_311(bool v312);
  bool find_314(bool v315);
  bool find_317(bool v318);
  bool find_322(bool v323);
  bool find_328(bool v329, int32_t v330);
  bool find_335(bool v336, int32_t v337);
  bool find_339(bool v340, int32_t v341);
  bool find_344(bool v345, int32_t v346);
  bool find_348(int32_t v349);
  bool find_351(int32_t v352);
  bool find_354(int32_t v355);
  bool find_357(int32_t v358);
  bool find_361(int32_t v362);
  bool find_364(int32_t v365);
  bool find_367(int32_t v368);
  bool find_372(int32_t v373);
  bool find_375(int32_t v376);
  bool find_379(bool v380, int32_t v381);
  bool find_384(bool v385, int32_t v386);
  bool find_388(int32_t v389);
  bool find_391(int32_t v392);
  bool find_394(int32_t v395);
  bool find_397(int32_t v398);
  bool find_401(int32_t v402);
  bool find_404(int32_t v405);
  bool find_407(int32_t v408);
  bool find_412(int32_t v413);
  bool find_415(int32_t v416);
  bool find_419(int32_t v420);
  bool find_423(int32_t v424);
  bool find_426(int32_t v427);
  bool find_429(int32_t v430);
  bool find_434(int32_t v435);
  bool find_437(int32_t v438);
  bool find_444(bool v445, int32_t v446);
  bool find_448(bool v449, int32_t v450);
  bool find_453(int32_t v454, bool v455);
  bool find_457(bool v458);
  bool find_460(bool v461);
  bool find_463(bool v464);
  bool find_469(int32_t v470);
  bool find_473(int32_t v474);
  bool find_476(int32_t v477);
  bool find_479(int32_t v480);
  bool find_484(int32_t v485);
  bool find_487(int32_t v488);
  bool find_494(bool v495, int32_t v496);
  bool find_498(bool v499, int32_t v500);
  bool find_503(int32_t v504, bool v505);
  bool find_508(int32_t v509, bool v510);
  bool find_513(int32_t v514);
  bool find_516(int32_t v517);
  bool find_519(int32_t v520);
  bool find_524(int32_t v525);
  bool find_531(bool v532, int32_t v533);
  bool find_535(bool v536, int32_t v537);
  bool find_540(int32_t v541, bool v542);
  bool find_544(bool v545);
  bool find_547(bool v548);
  bool find_550(bool v551);
  bool find_556(bool v557);
  bool find_561(bool v562);
  bool flow_568(::hyde::rt::Vec<Tup_b> vec114, ::hyde::rt::Vec<Tup_b> vec115, ::hyde::rt::Vec<Tup_b> vec116);

  ::hyde::rt::Allocator allocator;
  [[maybe_unused]] DatabaseLog &log;
  [[maybe_unused]] DatabaseFunctors &functors;

  ::hyde::rt::Table<Row5> table_5;
  ::hyde::rt::Index<Key133> idx_133;
  ::hyde::rt::Table<Row9> __9;
  ::hyde::rt::Table<Row12> table_12;
  ::hyde::rt::Index<Key137> idx_137;
  ::hyde::rt::Table<Row16> table_16;
  ::hyde::rt::Index<Key141> idx_141;
  ::hyde::rt::Table<Row20> table_20;
  ::hyde::rt::Table<Row23> table_23;
  ::hyde::rt::Table<Row26> table_26;
  ::hyde::rt::Table<Row29> table_29;
  ::hyde::rt::Table<Row32> table_32;
  ::hyde::rt::Index<Key527> idx_527;
  ::hyde::rt::Table<Row36> table_36;
  ::hyde::rt::Table<Row39> table_39;
  ::hyde::rt::Index<Key490> idx_490;
  ::hyde::rt::Table<Row43> table_43;
  ::hyde::rt::Index<Key440> idx_440;
  ::hyde::rt::Table<Row47> table_47;
  ::hyde::rt::Index<Key294> idx_294;
  ::hyde::rt::Table<Row51> table_51;
  ::hyde::rt::Index<Key262> idx_262;
  ::hyde::rt::Table<Row55> table_55;
  ::hyde::rt::Table<Row58> table_58;
  ::hyde::rt::Table<Row61> table_61;
  ::hyde::rt::Table<Row64> table_64;
  ::hyde::rt::Table<Row67> table_67;
  ::hyde::rt::Table<Row71> table_71;
  ::hyde::rt::Table<Row75> pos_neg_75;
  ::hyde::rt::Table<Row78> both_pos_78;
  ::hyde::rt::Table<Row81> mixed_81;
  ::hyde::rt::Table<Row84> table_84;
  ::hyde::rt::Index<Key98> idx_98;
  ::hyde::rt::Table<Row88> table_88;
  ::hyde::rt::Index<Key106> idx_106;

  uint64_t g93 = 0;
};

