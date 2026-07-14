# The Dr. Lojekyll Datalog Dialect

This is the language reference for the Datalog dialect accepted by the
`drlojekyll` compiler. The parser (`lib/Parse/*.cpp`) is the ground truth;
every example in this document compiles with `drlojekyll <file> -ir-out
/dev/null`. Where `docs/Grammar.md` disagrees with the parser, this document
says so (see the final section).

## Evaluation model

Dr. Lojekyll compiles a module into an incremental, message-driven,
differential database:

- **Bottom-up.** Rules are materialized eagerly: when a fact is added, every
  clause that can consume it runs to a fixpoint, updating derived relations.
- **Message-driven.** All ground data enters through `#message` receipt and
  all push-style output leaves through `#message` publication. A received
  message vector triggers one round of evaluation; `#query` calls read the
  materialized state between rounds.
- **Differential.** Messages marked `@differential` carry removals as well as
  additions. Retracting an input fact triggers re-checking of everything
  derived from it: each stored tuple carries a state (absent / present /
  unknown); retraction flips dependents to *unknown*, and the engine attempts
  to re-prove them through alternative derivations before flipping them to
  *absent*. A tuple with several independent proofs survives the loss of one
  of them. Runtime detail is in [RuntimeAndCodegen.md](RuntimeAndCodegen.md).

Facts are sets, not multisets: proving the same tuple twice stores it once,
though a published message may be observed more than once by the outside
world.

## Lexical basics

- **Comments** run from `;` to the end of the line.
- **Atoms** (predicate, functor, and message names) match `[a-z][A-Za-z0-9_]*`.
- **Variables** match `[A-Z][A-Za-z0-9_]*`; `_`-prefixed identifiers are
  anonymous variables, each use distinct.
- **Number literals**: decimal `42`, octal `010`, hexadecimal `0x10`, binary
  `0b101`, floating point `1.5`. **Booleans**: `true`, `false`. **Strings**:
  double-quoted, single-line.
- **Code literals** are triple-backquoted and optionally language-tagged:
  ` ```c++ ...``` `, ` ```python ...``` `, or untagged ` ```...``` ` (applies
  to all target languages). Only the C++ backend generates code today;
  `python`-tagged literals parse but are unused.
- Declarations and clauses end with a period. Maximum arity everywhere is 63.

## Types

The built-in type vocabulary is exactly:

| Type | Meaning |
|---|---|
| `bool` | Boolean |
| `i8` `i16` `i32` `i64` | signed integers |
| `u8` `u16` `u32` `u64` | unsigned integers |
| `f32` `f64` | floating point |

Everything else is a **foreign type** (`#foreign`, `#enum`) referenced by
name. There is no built-in string or bytes type; declare a foreign type
(e.g. `#foreign str ```c++ std::string```.`) to store strings.

## Module directives

The complete set of directives the parser accepts: `#database`, `#import`,
`#inline`, `#foreign`, `#constant`, `#enum`, `#message`, `#local`, `#export`,
`#query`, `#functor`. There are no others.

### `#database`

Names the database; the C++ backend uses the name for the generated files and
wraps everything in `namespace <name>`. The name is an identifier, or a
string/code literal that is split into identifier-shaped parts.

```datalog
#database mydb.
```

### `#import`

Imports another module by path (searched relative to the importing file, then
each `-M` directory). All imports must precede every other declaration and
clause in the module.

```datalog
#import "shared_defs.dr".
```

### `#inline`

Splices code into the generated output at a named stage. The stage must be
one of (whitespace inside the parentheses is ignored):
`c++:database:{prologue,epilogue}`,
`c++:database:{prologue,epilogue}:namespace`,
`c++:database:enums:{prologue,epilogue}`,
`c++:database:functors:{prologue,epilogue}`,
`c++:database:functors:definition:{prologue,epilogue}`, and the same four
`log` variants.

```datalog
#inline(c++:database:prologue) ```c++
#include <cstdint>
```.
```

### `#foreign`

Declares an opaque type implemented in the target language. A bare
declaration forward-declares the name; further declarations attach a
per-language (or untagged, language-generic) representation, an optional
untagged constructor expression containing exactly one `$` (where the value
is substituted), and the `@transparent` / `@nullable` pragmas.

```datalog
#foreign StdString.
#foreign StdString ```c++ std::string```.
#foreign Addr ```c++ uint64_t``` ```(uint64_t)($)``` @transparent @nullable.
```

### `#constant`

Declares a named constant of a foreign, enum, or built-in type. The value is
a language-tagged code literal, an untagged code literal, a number (built-in
integral/float and enum types), `true`/`false` (`bool` only), or a string
(foreign types only). Strings cannot initialize built-in-typed constants, and
a value is always required. `@unique` marks the constant as unequal to every
other constant of the type.

```datalog
#foreign Addr ```c++ uint64_t``` @transparent.
#constant Addr NULL_ADDR ```c++ 0```.
#constant u32 kMagic 42 @unique.
#constant Addr kEntry ```0x1000``` @unique.
```

### `#enum`

Declares a named integral type. The underlying type (signed/unsigned integer,
default when omitted) may be given once; enumerators are added with
`#constant`, and each enumerator must be given an explicit value.

```datalog
#enum Color u8.
#constant Color RED 1.
#constant Color BLUE 2.
```

### `#message`

A message is the input/output boundary. Parameters are `type Var` pairs — no
binding specifiers. A message used in a clause body is *received*; used as a
clause head it is *published*; a given message must be one or the other.
Every received message must contribute to some query or published message,
or the compiler rejects the module ("Last receive of message ... is
unused").

`@differential` marks a message as carrying removals as well as additions.
It is *required* on a published message when the compiler determines
retractions can reach it (e.g. its value depends on a negation or on a
differential input).

```datalog
#message add_edge(u32 From, u32 To) @differential.
#local edge(u32 From, u32 To).
edge(From, To) : add_edge(From, To).
#message publish(u32 From, u32 To) @differential : edge(From, To).
```

The last line shows the embedded-clause form: `#message decl : body.`
defines the publication rule inline. `@product` may also appear after a
message declaration that has an embedded clause, blessing a cross-product in
that body.

### `#local` and `#export`

Relational predicates defined by clauses in Datalog. A `#local` is visible
only in its module; an `#export` is visible to importing modules (same-named,
same-arity exports in different modules are the same relation). Parameters
are `type Var`, bare `Var` (type inferred from use), or
`mutable(<merge functor>) Var` (see the KV-index gap below). After the
parameter list: `@inline` (inlining hint), and either `.` or an embedded
clause `: body.` (a trailing `@highlight` is permitted only together with an
embedded clause).

```datalog
#message m(u32 A, u32 B).
#local helper(A, B) @inline.
helper(A, B) : m(A, B).
#local hi(u32 X) @highlight : helper(X, _).
#query q(free u32 X).
q(X) : hi(X).
```

### `#query`

A pre-packaged read of materialized state, callable from the host program.
Every parameter carries a binding specifier: `bound` (caller supplies the
value) or `free` (result column). The name-plus-binding-pattern is the unit
of identity — the same name may be re-declared with different patterns, and
each pattern generates its own entry point. In generated C++, an all-`bound`
query is a `bool` existence check; a query with `free` columns is a cursor
(`<name>_<pattern>(db, ...)` — a hidden-friend call taking the database as its
first argument, where the pattern is one `b`/`f` letter per column). `@first` (requires at least one `free` parameter) limits the result
to the first match. Queries also accept an embedded clause.

```datalog
#message data(u32 X, u32 Y).
#local d(u32 X, u32 Y).
d(X, Y) : data(X, Y).
#query lookup(bound u32 X, free u32 Y) @first.
lookup(X, Y) : d(X, Y).
#query all_pairs(free u32 X, free u32 Y) : d(X, Y).
```

### `#functor`

A function implemented by native code. Parameters carry `bound`, `free`,
`aggregate`, or `summary` specifiers plus a type. In generated C++, each
functor is a member the user defines on the `DatabaseFunctors` struct, named
`<name>_<pattern>` (one `b`/`f` letter per parameter): with only `bound`
parameters it is a filter returning `bool` (`keep_b(uint32_t)`); with `free`
parameters it returns the value directly (`@range(.)`), `std::optional`
(`@range(?)`), or `std::vector` (`@range(*)`, `@range(+)`).

- `@range(.)` one-to-one, `@range(?)` zero-or-one, `@range(*)` zero-or-more
  (the default with `free` parameters), `@range(+)` one-or-more. A functor
  with no `free` parameters is implicitly — and must be — zero-or-one.
- `@impure` parses but is then rejected: "Impure functors are not yet
  supported." All functors must therefore be pure (same bound inputs, same
  outputs).
- `@inline` requests direct inlining; `@inline(<code literal>)` supplies the
  callable to inline, e.g. a C++ lambda, which is spliced directly into the
  call site instead of generating a `DatabaseFunctors` member.
- Functors with only `free` parameters are rejected.
- `aggregate`/`summary` declare an aggregating functor (see aggregation
  below); such functors may not take a `@range`.

```datalog
#functor add_u32(bound u32 LHS, bound u32 RHS, free u32 Sum) @range(.).
#functor filter_lt(bound u32 A, bound u32 B).
#functor add1(bound u32 X, free u32 Y) @range(.)
    @inline(```c++ [] (auto x) { return x + 1; }```).
```

## Pragmas

The full set of `@` pragmas the lexer accepts, and where each is legal:

| Pragma | Where | Meaning |
|---|---|---|
| `@differential` | `#message` | message carries removals as well as additions |
| `@range(.` \| `?` \| `*` \| `+)` | `#functor` | output multiplicity per bound input |
| `@impure` | `#functor` | parses, then rejected ("not yet supported") |
| `@inline` / `@inline(code)` | `#functor`, `#local`, `#export` | inlining hint; on functors, optionally with the code to inline |
| `@transparent` | `#foreign` | equality implies identity; skip interning |
| `@nullable` | `#foreign` | type has a natural null; no `std::optional` wrapper |
| `@unique` | `#constant` | constant is unequal to every other constant of its type |
| `@product` | clause head, `#message` w/ embedded clause | opt in to a cross-product in this body |
| `@highlight` | clause head, decl w/ embedded clause | highlight this clause's nodes in `-dot-out` dataflow output |
| `@first` | `#query` decl; query clause body | first-match-only query; in a body, `@first msg(...)` sends a forcing message before reading |
| `@never` | clause body | monotone negation: once satisfied, always satisfied |
| `@barrier` | clause body | join everything before the barrier before joining what follows |

## Clauses

A clause is `head : conjunct, conjunct, ... .` or a fact `head.`. The head
is an atom with a parenthesized argument list of named variables, literals,
or named constants (a literal argument `one(1).` becomes an equality
constraint on a fresh variable). Anonymous variables are not allowed in
heads. Head pragmas (`@highlight`, `@product`) sit between the argument list
and the `:` or `.`.

Body conjuncts, separated by commas:

- **Predicate application** `p(A, 5, _)` — arguments are variables, anonymous
  variables, literals, or named constants. Every predicate used must already
  be declared (except zero-arity conditions, below).
- **Comparison** `A = B`, `A != B`, `A < B`, `A > B` between variables and/or
  literals. `<=` and `>=` do not exist; the same variable may not appear on
  both sides.
- **Negation** `!p(A)` — stratified negation over a relation. `@never p(A)`
  is the monotone variant. Functor applications may only be negated with
  `@never`, and zero-arity predicates cannot be negated with `@never`.
- **Boolean variables as conjuncts**: for `B` of type `bool`, bare `B` means
  `B = true` and `!B` means `B = false`. Bare `true` is a no-op conjunct;
  `false` (or `!true`) statically disables the clause.
- **Zero-arity conditions**: an atom with no argument list is a global
  Boolean condition, provable by a zero-arity clause head and usable without
  any declaration:

  ```datalog
  #message enable(u32 X).
  is_enabled : enable(_).
  #message e(u32 A).
  #query q(free u32 A).
  q(A) : e(A), is_enabled.
  ```

  Conditions are pure sugar for *unit relations*: the compiler desugars each
  condition into a 1-column `bool` relation whose only possible row is
  `(true)`. Proving the condition inserts the row; a positive test joins
  against it; a negative test (`!is_enabled`) is an ordinary negation. There
  is no other condition machinery, so conditions compose with recursion and
  `@differential` retraction exactly like any relation.

  Relatedly, every conjunct group of a clause body must share at least one
  variable with the clause head (transitively). A group that shares none is
  a style error -- "should be factored out into a zero-argument predicate"
  -- prompting exactly this sugar: name the group as a condition and test
  the condition instead.

- **`@barrier`** splits the body into join groups (must be followed by a
  comma).
- **`@first msg(A, ...)`** (query clause bodies only) sends the message
  before reading the materialized view — an "unlocking" send; at most one per
  body, and it must name a message.
- **Aggregation** `agg_functor(...) over pred(...)` applies an
  `aggregate`/`summary` functor over a predicate, grouping by the predicate
  variables not bound by the functor application; `over (params) { body }`
  aggregates over an inline anonymous predicate. This *parses and builds
  dataflow* but does not compile to a runnable program (see gaps).

**Multiple bodies** for one head can be chained with `:`; the clause
`f(A) : e(A, 1) : e(2, A).` is exactly two clauses.

**Cross-products.** A body whose variables split into groups not linked by
any shared variable, join, or comparison requires a cross-product. These are
errors unless the head is annotated `@product`:

```datalog
#message a(u32 X).
#message b(u32 Y).
#local prod(u32 X, u32 Y).
prod(X, Y) @product : a(X), b(Y).
#query q(free u32 X, free u32 Y).
q(X, Y) : prod(X, Y).
```

## Known feature gaps

These forms parse and pass semantic analysis but are rejected by
`Program::Build`'s pre-pass with a clean diagnostic, so no program using
them compiles end-to-end:

- **Aggregates** (`aggregate`/`summary` functors, `over`).
- **KV indices** — `mutable(merge_functor)` parameters on locals/exports.
- Cross-products over deletable data **inside recursive cycles** (a
  `@product` body atom recursively derived from the product's own result).
  Acyclic differential cross-products — including `@differential @product`
  messages — compile and are incrementally maintained.
- `@impure` functors (as noted above, rejected at parse time).

## Worked example

```datalog
#database reach.

#message edge(u32 From, u32 To) @differential.

#local path(u32 From, u32 To).
path(From, To) : edge(From, To).
path(From, To) : path(From, X), edge(X, To).

#message new_path(u32 From, u32 To) @differential : path(From, To).

#query reachable_from(bound u32 From, free u32 To).
reachable_from(From, To) : path(From, To).

#query connected(bound u32 From, bound u32 To).
connected(From, To) : path(From, To).
```

`drlojekyll reach.dr -cpp-out gen/` emits `gen/reach.h` and `gen/reach.cpp`
containing, inside `namespace reach`:

```cpp
struct DatabaseFunctors {};                 // one method per #functor (none here)

struct DatabaseLog {                        // one method per published message
  void new_path_2(uint32_t From, uint32_t To, bool added) {}
};

struct Database {                           // sealed state struct; members private
 public:
  explicit Database(::hyde::rt::Allocator); // allocates empty tables; no epoch 0
};

// Driver-facing functions are HIDDEN FRIENDS of Database, reachable only by
// unqualified (ADL) call with the database argument. Types may be qualified
// (reach::Database), but a qualified CALL such as reach::init(db, ...) does
// not compile:
//
//   init(db, log, functors);                    // epoch 0: empty model + t=0 deltas
//   edge_2(db, log, functors, added, removed);  // @differential => (adds, removals)
//   auto c = reachable_from_bf(db, From);        // #query (bf): cursor over free col
//   bool ok = connected_bb(db, From, To);        // #query (bb): existence check
```

A non-differential message takes a single `Vec`. The driver constructs `db`
(construction runs no epoch 0), then calls `init(db, log, functors)` exactly
once before any message; the entry points and queries assert it has run. The
log and functors types flow by DEDUCTION — nothing is virtual, so any type
providing the same member signatures observes the published deltas. The driver
pushes edge tuples through `edge_2`; the fixpoint over `path` runs inside that
call, invoking `log.new_path_2(..., true)` for each new pair (and with `false`
for each retracted pair when removals are supplied); afterwards the driver
iterates `reachable_from_bf(db, from)` with `cursor.next(to)`.

## Divergences of docs/Grammar.md from the parser

`docs/Grammar.md` is stale in these respects (parser wins):

- It describes `#prologue`/`#epilogue` directives and
  `server:`/`client:`/`interface:` inline stages; neither exists. The only
  code-splicing directive is `#inline(<stage>)` with the stage names listed
  above.
- It lists a built-in `bytes` type; there is no such type.
- Its pragma inventory omits `@nullable` and `@first`, and its
  `message_decl` rule omits both `@product` and the embedded-clause form
  (`#message decl [@differential] [@product] : body.`).
- Its `enum_decl` requires an underlying type; the parser also accepts
  `#enum Foo.` with a default underlying type.
- Its clause head rule allows only named variables; the parser also accepts
  literals and named constants in heads (rewritten to equality constraints).
- Its `literal` rule and prose omit binary (`0b...`) literals, which lex and
  parse.
- The comment in `lib/Lex/Token.h` claiming enum `#constant`s may omit their
  value is also wrong: the parser requires a value.
