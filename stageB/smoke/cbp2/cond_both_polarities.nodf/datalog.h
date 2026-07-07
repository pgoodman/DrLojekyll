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

// Key of `idx_132` over `table_5`.
struct Key132 {
  bool c1;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c1);
  }
  bool operator==(const Key132 &) const noexcept = default;
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

// Key of `idx_136` over `table_12`.
struct Key136 {
  bool c1;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c1);
  }
  bool operator==(const Key136 &) const noexcept = default;
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

// Key of `idx_166` over `table_16`.
struct Key166 {
  bool c1;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c1);
  }
  bool operator==(const Key166 &) const noexcept = default;
};

// Rows of `__20` (/0).
struct Row20 {
  bool c0;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c0);
  }
  bool operator==(const Row20 &) const noexcept = default;
};

// Rows of `table_23`.
struct Row23 {
  int32_t x;
  bool c1;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(x, c1);
  }
  bool operator==(const Row23 &) const noexcept = default;
};

// Key of `idx_140` over `table_23`.
struct Key140 {
  bool c1;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c1);
  }
  bool operator==(const Key140 &) const noexcept = default;
};

// Rows of `table_27`.
struct Row27 {
  int32_t x;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(x);
  }
  bool operator==(const Row27 &) const noexcept = default;
};

// Rows of `table_30`.
struct Row30 {
  int32_t x;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(x);
  }
  bool operator==(const Row30 &) const noexcept = default;
};

// Rows of `table_33`.
struct Row33 {
  int32_t x;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(x);
  }
  bool operator==(const Row33 &) const noexcept = default;
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

// Key of `idx_579` over `table_39`.
struct Key579 {
  int32_t x;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(x);
  }
  bool operator==(const Key579 &) const noexcept = default;
};

// Rows of `table_43`.
struct Row43 {
  int32_t x;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(x);
  }
  bool operator==(const Row43 &) const noexcept = default;
};

// Rows of `table_46`.
struct Row46 {
  bool c0;
  int32_t x;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c0, x);
  }
  bool operator==(const Row46 &) const noexcept = default;
};

// Key of `idx_542` over `table_46`.
struct Key542 {
  int32_t x;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(x);
  }
  bool operator==(const Key542 &) const noexcept = default;
};

// Rows of `table_50`.
struct Row50 {
  bool c0;
  int32_t x;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c0, x);
  }
  bool operator==(const Row50 &) const noexcept = default;
};

// Key of `idx_492` over `table_50`.
struct Key492 {
  int32_t x;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(x);
  }
  bool operator==(const Key492 &) const noexcept = default;
};

// Rows of `table_54`.
struct Row54 {
  int32_t x;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(x);
  }
  bool operator==(const Row54 &) const noexcept = default;
};

// Rows of `table_57`.
struct Row57 {
  bool c0;
  int32_t x;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c0, x);
  }
  bool operator==(const Row57 &) const noexcept = default;
};

// Key of `idx_390` over `table_57`.
struct Key390 {
  int32_t x;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(x);
  }
  bool operator==(const Key390 &) const noexcept = default;
};

// Rows of `table_61`.
struct Row61 {
  bool c0;
  int32_t x;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c0, x);
  }
  bool operator==(const Row61 &) const noexcept = default;
};

// Key of `idx_291` over `table_61`.
struct Key291 {
  int32_t x;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(x);
  }
  bool operator==(const Key291 &) const noexcept = default;
};

// Rows of `table_65`.
struct Row65 {
  bool c0;
  int32_t x;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c0, x);
  }
  bool operator==(const Row65 &) const noexcept = default;
};

// Key of `idx_259` over `table_65`.
struct Key259 {
  int32_t x;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(x);
  }
  bool operator==(const Key259 &) const noexcept = default;
};

// Rows of `table_69`.
struct Row69 {
  int32_t x;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(x);
  }
  bool operator==(const Row69 &) const noexcept = default;
};

// Rows of `table_72`.
struct Row72 {
  int32_t x;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(x);
  }
  bool operator==(const Row72 &) const noexcept = default;
};

// Rows of `table_75`.
struct Row75 {
  int32_t x;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(x);
  }
  bool operator==(const Row75 &) const noexcept = default;
};

// Rows of `table_78`.
struct Row78 {
  int32_t x;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(x);
  }
  bool operator==(const Row78 &) const noexcept = default;
};

// Rows of `table_81`.
struct Row81 {
  bool c0;
  int32_t c1;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c0, c1);
  }
  bool operator==(const Row81 &) const noexcept = default;
};

// Rows of `table_85`.
struct Row85 {
  bool c0;
  int32_t c1;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c0, c1);
  }
  bool operator==(const Row85 &) const noexcept = default;
};

// Rows of `pos_neg_89` (pos_neg/1).
struct Row89 {
  int32_t x;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(x);
  }
  bool operator==(const Row89 &) const noexcept = default;
};

// Rows of `both_pos_92` (both_pos/1).
struct Row92 {
  int32_t x;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(x);
  }
  bool operator==(const Row92 &) const noexcept = default;
};

// Rows of `mixed_95` (mixed/1).
struct Row95 {
  int32_t x;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(x);
  }
  bool operator==(const Row95 &) const noexcept = default;
};

// Rows of `table_98`.
struct Row98 {
  int32_t x;
  bool c1;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(x, c1);
  }
  bool operator==(const Row98 &) const noexcept = default;
};

// Key of `idx_148` over `table_98`.
struct Key148 {
  bool c1;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c1);
  }
  bool operator==(const Key148 &) const noexcept = default;
};

// Rows of `table_102`.
struct Row102 {
  int32_t x;
  bool c1;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(x, c1);
  }
  bool operator==(const Row102 &) const noexcept = default;
};

// Key of `idx_112` over `table_102`.
struct Key112 {
  bool c1;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c1);
  }
  bool operator==(const Key112 &) const noexcept = default;
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
  bool set_c1_1(::hyde::rt::Vec<Tup_i32> vec199, ::hyde::rt::Vec<Tup_i32> vec200);

  // Message `set_c2/1`.
  bool set_c2_1(::hyde::rt::Vec<Tup_i32> vec206, ::hyde::rt::Vec<Tup_i32> vec207);

  // Message `add_r/1`.
  bool add_r_1(::hyde::rt::Vec<Tup_i32> vec213);

  // Message `add_s/1`.
  bool add_s_1(::hyde::rt::Vec<Tup_i32> vec219);

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
  bool proc_106(::hyde::rt::Vec<Tup_i32> vec108, ::hyde::rt::Vec<Tup_i32> vec109, ::hyde::rt::Vec<Tup_i32> vec144, ::hyde::rt::Vec<Tup_i32> vec145, ::hyde::rt::Vec<Tup_i32> vec170, ::hyde::rt::Vec<Tup_i32> vec173);
  bool find_116(int32_t v117, bool v118);
  bool find_129(bool v130);
  bool find_152(int32_t v153, bool v154);
  bool find_163(bool v164);
  bool find_229(int32_t v230);
  bool find_231(int32_t v232);
  bool find_233(int32_t v234);
  bool find_235(int32_t v236);
  bool find_239(int32_t v240);
  bool find_242(int32_t v243);
  bool find_245(int32_t v246);
  bool find_253(int32_t v254);
  bool find_256(int32_t v257);
  bool find_263(bool v264, int32_t v265);
  bool find_267(bool v268, int32_t v269);
  bool find_277(int32_t v278);
  bool find_285(int32_t v286);
  bool find_288(int32_t v289);
  bool find_295(bool v296, int32_t v297);
  bool find_299(bool v300, int32_t v301);
  bool find_304(int32_t v305, bool v306);
  bool find_311(bool v312);
  bool find_319(bool v320);
  bool find_325(bool v326, int32_t v327);
  bool find_329(bool v330, int32_t v331);
  bool find_334(bool v335, int32_t v336);
  bool find_338(int32_t v339);
  bool find_341(int32_t v342);
  bool find_344(int32_t v345);
  bool find_347(int32_t v348);
  bool find_351(int32_t v352);
  bool find_354(int32_t v355);
  bool find_362(int32_t v363);
  bool find_365(int32_t v366);
  bool find_369(int32_t v370);
  bool find_373(int32_t v374);
  bool find_376(int32_t v377);
  bool find_384(int32_t v385);
  bool find_387(int32_t v388);
  bool find_394(bool v395, int32_t v396);
  bool find_398(bool v399, int32_t v400);
  bool find_403(int32_t v404, bool v405);
  bool find_410(bool v411);
  bool find_418(bool v419);
  bool find_424(bool v425, int32_t v426);
  bool find_428(bool v429, int32_t v430);
  bool find_433(bool v434, int32_t v435);
  bool find_437(int32_t v438);
  bool find_440(int32_t v441);
  bool find_443(int32_t v444);
  bool find_446(int32_t v447);
  bool find_450(int32_t v451);
  bool find_468(int32_t v469, bool v470);
  bool find_478(int32_t v479);
  bool find_481(int32_t v482);
  bool find_489(int32_t v490);
  bool find_496(bool v497, int32_t v498);
  bool find_500(bool v501, int32_t v502);
  bool find_505(int32_t v506, bool v507);
  bool find_521(int32_t v522);
  bool find_525(int32_t v526);
  bool find_528(int32_t v529);
  bool find_536(int32_t v537);
  bool find_539(int32_t v540);
  bool find_546(bool v547, int32_t v548);
  bool find_550(bool v551, int32_t v552);
  bool find_560(int32_t v561, bool v562);
  bool find_565(int32_t v566);
  bool find_568(int32_t v569);
  bool find_576(int32_t v577);
  bool find_583(bool v584, int32_t v585);
  bool find_587(bool v588, int32_t v589);
  bool find_592(int32_t v593, bool v594);
  bool flow_620(::hyde::rt::Vec<Tup_b> vec120, ::hyde::rt::Vec<Tup_b> vec121, ::hyde::rt::Vec<Tup_b> vec122, ::hyde::rt::Vec<Tup_b> vec156);

  ::hyde::rt::Allocator allocator;
  [[maybe_unused]] DatabaseLog &log;
  [[maybe_unused]] DatabaseFunctors &functors;

  ::hyde::rt::Table<Row5> table_5;
  ::hyde::rt::Index<Key132> idx_132;
  ::hyde::rt::Table<Row9> __9;
  ::hyde::rt::Table<Row12> table_12;
  ::hyde::rt::Index<Key136> idx_136;
  ::hyde::rt::Table<Row16> table_16;
  ::hyde::rt::Index<Key166> idx_166;
  ::hyde::rt::Table<Row20> __20;
  ::hyde::rt::Table<Row23> table_23;
  ::hyde::rt::Index<Key140> idx_140;
  ::hyde::rt::Table<Row27> table_27;
  ::hyde::rt::Table<Row30> table_30;
  ::hyde::rt::Table<Row33> table_33;
  ::hyde::rt::Table<Row36> table_36;
  ::hyde::rt::Table<Row39> table_39;
  ::hyde::rt::Index<Key579> idx_579;
  ::hyde::rt::Table<Row43> table_43;
  ::hyde::rt::Table<Row46> table_46;
  ::hyde::rt::Index<Key542> idx_542;
  ::hyde::rt::Table<Row50> table_50;
  ::hyde::rt::Index<Key492> idx_492;
  ::hyde::rt::Table<Row54> table_54;
  ::hyde::rt::Table<Row57> table_57;
  ::hyde::rt::Index<Key390> idx_390;
  ::hyde::rt::Table<Row61> table_61;
  ::hyde::rt::Index<Key291> idx_291;
  ::hyde::rt::Table<Row65> table_65;
  ::hyde::rt::Index<Key259> idx_259;
  ::hyde::rt::Table<Row69> table_69;
  ::hyde::rt::Table<Row72> table_72;
  ::hyde::rt::Table<Row75> table_75;
  ::hyde::rt::Table<Row78> table_78;
  ::hyde::rt::Table<Row81> table_81;
  ::hyde::rt::Table<Row85> table_85;
  ::hyde::rt::Table<Row89> pos_neg_89;
  ::hyde::rt::Table<Row92> both_pos_92;
  ::hyde::rt::Table<Row95> mixed_95;
  ::hyde::rt::Table<Row98> table_98;
  ::hyde::rt::Index<Key148> idx_148;
  ::hyde::rt::Table<Row102> table_102;
  ::hyde::rt::Index<Key112> idx_112;

  uint64_t g107 = 0;
};

