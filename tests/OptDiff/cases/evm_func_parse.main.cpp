#include <algorithm>
#include <cstdint>
#include <iostream>
#include <vector>

#include "datalog.h"

// AST node encoding: (left_tok << 40) | (right_tok << 16) | kind.
// kind 1 = function type header, 2 = finished function type, 3 = base type.
namespace {

constexpr uint64_t Enc(uint32_t left, uint32_t right, uint64_t kind) {
  return (uint64_t(left) << 40) | (uint64_t(right) << 16) | kind;
}

constexpr uint32_t LeftOf(uint64_t node) {
  return uint32_t(node >> 40);
}

constexpr uint32_t RightOf(uint64_t node) {
  return uint32_t((node >> 16) & 0xFFFFFFu);
}

}  // namespace

uint32_t DatabaseFunctors::left_corner_bf(uint64_t node) {
  return LeftOf(node);
}

uint32_t DatabaseFunctors::right_corner_bf(uint64_t node) {
  return RightOf(node);
}

// Fixed token stream: 10 `function`, 11 `(`, 12 <base type>, 13 `)`,
// 14 <ident>; 20 `function`, 21 `(`, 22 <junk>, 23 <ident>.
uint16_t DatabaseFunctors::lexeme_of_token_bf(uint32_t tok) {
  switch (tok) {
    case 10: return 4;   // KEYWORD_FUNCTION
    case 11: return 1;   // PUNC_L_PAREN
    case 12: return 20;  // base type name
    case 13: return 2;   // PUNC_R_PAREN
    case 14: return 21;  // identifier
    case 20: return 4;   // KEYWORD_FUNCTION
    case 21: return 1;   // PUNC_L_PAREN
    case 22: return 30;  // junk
    case 23: return 21;  // identifier
    default: return 500;
  }
}

uint64_t DatabaseFunctors::start_function_type_bbf(uint32_t func_tok,
                                                   uint32_t l_paren) {
  return Enc(func_tok, l_paren, 1);
}

uint64_t DatabaseFunctors::add_function_type_param_bbf(uint64_t header,
                                                       uint64_t param_type) {
  return Enc(LeftOf(header), RightOf(param_type), 1);
}

uint64_t DatabaseFunctors::finish_function_type_params_bbf(uint64_t header,
                                                           uint32_t r_paren) {
  return Enc(LeftOf(header), r_paren, 2);
}

int main() {
  const auto allocator = hyde::rt::MallocAllocator();
  DatabaseFunctors functors;
  DatabaseLog log;
  Database db(allocator, log, functors);

  auto dump = [&db]() {
    std::vector<uint64_t> types;
    auto c = db.type_name_f();
    for (uint64_t t = 0; c.next(t);) {
      types.push_back(t);
    }
    std::sort(types.begin(), types.end());
    std::cout << "type_names:";
    for (auto t : types) {
      std::cout << ' ' << LeftOf(t) << ':' << RightOf(t) << ':'
                << (t & 0xFFFFu);
    }
    std::cout << '\n';
  };

  // Round 1: a complete `function ( T )` type. The base type spanning
  // token 12 arrives first so the fixed-up (negation) path stays blocked.
  {
    hyde::rt::Vec<other_type_input> types(allocator);
    types.Add({Enc(12, 12, 3)});
    db.other_type_1(std::move(types));

    hyde::rt::Vec<next_token_input> toks(allocator);
    toks.Add({10, 11});
    toks.Add({11, 12});
    toks.Add({12, 13});
    toks.Add({13, 14});
    db.next_token_2(std::move(toks));
  }
  std::cout << "round 1\n";
  dump();

  // Round 2: a broken head `function ( <junk>`; the negation over
  // type_name_use lets the fixed-up path inject a fake `)`.
  {
    hyde::rt::Vec<next_token_input> toks(allocator);
    toks.Add({20, 21});
    toks.Add({21, 22});
    toks.Add({22, 23});
    db.next_token_2(std::move(toks));
  }
  std::cout << "round 2\n";
  dump();

  // Round 3: token 22 turns out to start a type after all, which must
  // retract the fixed-up head derived in round 2.
  {
    hyde::rt::Vec<other_type_input> types(allocator);
    types.Add({Enc(22, 22, 3)});
    db.other_type_1(std::move(types));
  }
  std::cout << "round 3\n";
  dump();
  return 0;
}
