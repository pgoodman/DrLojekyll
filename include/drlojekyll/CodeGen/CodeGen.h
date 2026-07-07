// Copyright 2020, Trail of Bits. All rights reserved.

#pragma once

#include <drlojekyll/Parse/Parse.h>

#include <string_view>

namespace hyde {

class Program;
class OutputStream;
enum class Language : unsigned;

std::vector<ParsedFunctor> Functors(ParsedModule module);
std::vector<ParsedQuery> Queries(ParsedModule module);
std::vector<ParsedMessage> Messages(ParsedModule module);
std::vector<ParsedInline> Inlines(ParsedModule module, Language lang);

namespace cxx {

// Emits a concrete C++ implementation of `program`: a header (row structs,
// enums, the `Database` class) and a source file (procedure and cursor
// definitions). `header_name` is the file name the source file `#include`s.
void GenerateDatabaseCode(const Program &program, OutputStream &os_h,
                          OutputStream &os_cc, std::string_view header_name);

}  // namespace cxx
}  // namespace hyde
