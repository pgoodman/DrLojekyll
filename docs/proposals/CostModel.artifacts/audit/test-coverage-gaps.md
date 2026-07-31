# Language and Composition Test Coverage

> Evidence date: 2026-07-31, branch `keyed-instances` at `d6904ae3`.
> Counts are repository observations, not stable product guarantees. Recompute
> them when the corpus changes.

The repository has substantially more semantic testing than its six CTest
entries suggest. It does not, however, have exhaustive language coverage, a
systematic pass-management matrix, or a standing whole-system application that
combines the difficult feature families. The important distinction is between
many strong directed witnesses and broad compositional confidence.

## What OptDiff Actually Covers

At the audit baseline, `tests/OptDiff/cases` contains:

| Surface | Count | What it means |
|---|---:|---|
| `.dr` programs | 181 | Directed compiler/runtime cases |
| C++ drivers | 178 | Cases that can be compiled and executed |
| `.batches` traces | 56 | Cases checked by the independent oracle and monotone projection |
| `.drflags` | 10 | Per-case compiler flags, all concentrated around demand work |
| `.eqgate` | 5 | Flat versus keyed-instance lowering equivalence gates |
| `.irgold` | 15 | Selected structural dump pins |

The generated evidence includes 166 ordinary stdout goldens, 56 oracle
goldens, 56 monotone-projection goldens, 15 Rel structural goldens, five
DataFlow structural goldens, two textual IR goldens, and two generated-header
goldens. The full suite passed all 181 cases during this audit.

Most cases are deliberately small. Source sizes range from 6 to 96 lines, with
a median of 19, a 90th percentile of 35, and a mean of about 22.6 lines. This
is good for diagnosis and minimization. It also means that a green suite should
not be read as evidence that feature interactions in a realistic program have
been explored.

Fifty-six batch-driven cases are especially valuable: the derivation-counter
oracle reevaluates independently after each batch, and the monotone projection
checks the final surviving input set. That is strong semantic evidence for the
covered programs. It is still about 31 percent of the `.dr` corpus, and both
oracles share the parser and Query construction with the implementation under
test.

## The Four Modes Are Coarse, Not a Pass Matrix

Every ordinary OptDiff program is compiled in four modes:

```text
opt:   DataFlow optimization on,  ControlFlow optimization on
nodf:  DataFlow optimization off, ControlFlow optimization on
nocf:  DataFlow optimization on,  ControlFlow optimization off
none:  DataFlow optimization off, ControlFlow optimization off
```

This is a high-value differential gate, but it does not answer whether each
individual pass is independently correct or whether specific pass pairs are
order-dependent. The corpus does not systematically exercise:

- each named pass enabled alone;
- each named pass disabled alone;
- pairwise or three-way pass interactions;
- `-opt-only` and `-opt-disable` conflict and precedence behavior;
- `-opt-bisect-limit` prefixes and their determinism;
- the equivalence of legacy coarse aliases and the authoritative pass list.

The registered `df.sink` pass has no implementation body. Until it is deleted,
it also creates a fake dimension in any generated pass matrix.

**Disposition: fixable.** Do not run every one of `2^N` configurations across
all 181 cases. That is expensive and still gives poor diagnosis. Use four
layers:

1. A unit truth table for pass selection, aliases, precedence, and bisect
   prefixes.
2. One or more directed carrier programs per pass, proving that the pass both
   fires and preserves semantics.
3. A pairwise covering array over live passes for the whole corpus, with
   targeted three-way combinations where ownership or order overlaps.
4. The existing four coarse modes as a stable, readable regression baseline.

Record the exact selected pass sequence in failure artifacts. A configuration
label such as `nodf` is not enough once pass-level testing exists.

## Feature Composition Is Sparse

The following counts are lexical approximations over OptDiff sources, not a
semantic parser-derived inventory. They are useful for finding empty regions,
not for proving coverage:

| Feature family | Files |
|---|---:|
| Differential messages | 51 |
| Negation | 26 |
| Functors | 26 |
| Aggregate/KV syntax | 13 |
| Product | 11 |
| Demand-related cases | 10 |
| Foreign types | 2 |
| Barrier syntax | 1 |

The empty or nearly empty intersections are more informative than the totals:

| Combination | Files | Consequence |
|---|---:|---|
| demand + aggregate/KV | 0 | No evidence for their composition |
| demand + negation | 0 | No positive end-to-end composition witness |
| demand + product | 0 | No evidence for the planner interaction |
| demand + functor | 0 | No demanded external computation witness |
| demand + barrier | 0 | No scheduling interaction witness |
| aggregate + negation | 0 | Stratification boundaries are not exercised together positively |
| aggregate + product | 0 | No combined cardinality/forcing witness |
| aggregate + differential | 1 | One witness is too narrow for a broad claim |
| foreign + differential | 0 | Important for real adapters, currently absent |
| foreign + batches | 0 | No independently refereed foreign-type event trace |
| product + negation | 1 | Only one directed carrier |

The concentration around differential batches is good: 49 of the 51
differential files have batch traces. Demand has ten cases, five batch traces,
and five equivalence gates. The weakness is not that these features have never
been tested; it is that their interactions with the rest of the language have
mostly been avoided or rejected in isolated diagnostics.

Some surface areas do not appear in OptDiff at all: `#database`, `#enum`,
`#export`, and `#inline`. Across `tests` and `data/self_testing_examples`,
`#database` appears only in MiniDisassembler and PointsTo, `#enum` only in
MiniDisassembler, and `@transparent` only in PointsTo. No test source was found
for explicit `@nullable`, `@range(?)`, `@range(*)`, `@range(+)`,
`@inline(code)`, or `@impure` forms. This does not prove the semantics are
unused internally; it means the public syntax is not visibly pinned.

The OptDiff type distribution is also narrow. `i32`, `u64`, and `u32` dominate;
`bool` appears four times and `u8`, `u16`, and `i64` three times each. No OptDiff
source uses `i8`, `i16`, `f32`, or `f64`. Parser acceptance is not the same as
generated-code and runtime behavior across those types.

**Disposition: fixable, with an adjudication edge.** Add composition tests only
for combinations that are intended to work. For intentionally unsupported
combinations, add precise diagnostic witnesses and state the semantic reason.
Do not force a kitchen-sink program through an incoherent combination merely
to make the matrix nonempty.

## Useful Applications Are Under-Refereed

MiniDisassembler is the best small application test. It combines recursion,
negation, enums, messages, and incremental additions, and it asserts exact
query sizes after several updates. It is compiled only under the default pass
configuration. Despite the test name `DifferentialUpdatesWork`, its messages
are not declared `@differential` and the driver performs no retraction. The
name currently overstates the kind of differential behavior demonstrated.

PointsTo is a credible recursive analysis over a nontrivial fact set and
foreign types. Its correctness assertions only require that three output
relations be nonempty. A completely wrong but nonempty result passes. The
driver writes TSV files but compares none of them with a trusted answer, and it
also compiles under only the default configuration.

`data/self_testing_examples` contains interesting language examples, but no
CMake or shell gate was found that compiles and runs them as a suite. An
example in the repository is not test evidence by presence alone.

**Disposition: fixable.** Give PointsTo a checked semantic authority: an exact
golden for the current fixed fact set, an independently computed relation, or
both. Put MiniDisassembler and PointsTo through a bounded pass covering array.
Add a genuine retraction/correction trace to MiniDisassembler or rename the
test so its claim matches its behavior.

## Fuzzing Is Present in Name but Not Operational

The disabled `.github/workflows/fuzz.yml_` uses obsolete `ENABLE_*` CMake
variables. The current options are `DRLOJEKYLL_ENABLE_*`. Re-enabling the
workflow unchanged would therefore not configure the intended targets.

More seriously, `fuzz/CMakeLists.txt` declares `drlojekyll-backend-fuzzer` from
`BackendFuzzer.cpp`, but that source file does not exist. Enabling libFuzzer
currently encounters a missing target input before any backend fuzzing can
occur.

The parser target's documented round-trip property is inert:

```text
intended:
  input -> parse A -> format A -> parse formatted text B -> format B
  require format(A) == format(B)

implemented:
  input -> parse A -> empty string A'
  input -> parse B -> empty string B'
  require A' == B'
```

`ParsedModuleToString` constructs an output stream but never formats `module`,
and the second parse consumes `data` instead of `module_string`. The target can
still find parser crashes on mutated bytes, but it does not test the property
its comments claim.

The dictionary is visibly historical. It includes old-looking imports and
pragmas while omitting newer scheduling and demand vocabulary. It should be
generated from the current lexer/parser surface or reviewed against it, not
patched by memory.

**Disposition: fix now as a separate trust repair.** Delete the nonexistent
backend target unless a concrete replacement is implemented. Repair the parser
property, add a deterministic unit regression that would fail if either parse
uses the original buffer, regenerate the dictionary, seed with current valid
programs, and only then re-enable a bounded sanitizer/libFuzzer job. The old
corpus may be useful seed material, but its size is not coverage evidence.

## Missing Test Program

The highest-leverage addition is not another 15-line isolated witness. It is a
deterministic investigation coordinator whose outputs are useful to a human and
whose language features have natural jobs. The companion design is in
[`agent-harness-reality.md`](agent-harness-reality.md).

The test should have multiple authorities:

1. An exact reviewed event transcript and final relation contents for one
   realistic scenario.
2. A `.batches` oracle trace and monotone final-state comparison where the
   language subset permits it.
3. The pass covering array described above.
4. Metamorphic variants: reorder independent facts, split and merge batches,
   duplicate idempotent inputs, add then retract an input, and consistently
   rename identifiers.
5. A deterministic fault script: timeout, retry, cancellation, late result,
   duplicate completion, and corrected evidence.
6. A small number of reviewed structural pins for architecture-significant
   plans, not a golden for every dump.
7. A separate scale/soak profile whose performance observations are not mixed
   into correctness goldens.

This will expose sharp edges that directed witnesses cannot: interactions
between recursion, stratified negation, differential correction, demand
isolation, multiple query adornments, aggregates, custom functors, scheduling,
and publication order. Unsupported intersections should become explicit
expected-diagnostic companion cases rather than hidden omissions.

## Priority

1. Repair the dead parser round trip and missing fuzz target declaration.
2. Automate the current full OptDiff suite or a sharded equivalent.
3. Test pass selection itself, then add a pairwise pass covering array.
4. Strengthen PointsTo from nonempty smoke test to semantic test.
5. Build the deterministic investigation coordinator and its metamorphic/fault
   variants.
6. Fill isolated syntax/type gaps where the surface is intentionally supported.

CostModel work should consume this evidence, not block on completing every
item. Conversely, a passing CostModel calibration must not be presented as
whole-language validation.
