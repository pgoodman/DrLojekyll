# IR dump formats — DRAFT (2026-07-18, pre-fleet; critique before code)

> **[SUPERSEDED IN PART — 2026-07-18, the T2 checkpoint fleet (ledger
> §4).]** The §2 DeltaRel inventory here was memory-sourced and is
> superseded by the code-verified reconciliation in
> **t2-dump-spec.md §2.3** (real DROpKind/Pred/EffKind spellings,
> index-identity ids, pinned_order, no stored band key, carriage on
> dep edges). §2.5's harness placement ("diffrun.sh additionally
> runs...") is WRONG — erratum **E-59**: the compare arm lives in
> runall.sh's `--one` worker only (t2-dump-spec.md §3.1). The §1
> BB-form design SURVIVES and is refined by t2-dump-spec.md §1.3.

Owner directives (KeyedInstances.md §0.5): the delta-relational IR
gets an id-ordered textual dump; the dataflow IR gets a non-DOT
textual form for model consumption, in a "basic block with arguments"
style where everything tail-calls. Both dumps must be deterministic
BY CONSTRUCTION (id-ordered — never container order), because they
double as byte-identity gate surfaces and (F)-hunt discriminators.

Status: DRAFT. The DeltaRel op/attribute inventory below is written
from memory of the CLAUDE.md summary; the D0 fleet's drir lane report
is the authority — reconcile before implementation. The DataFlow
sketch is a representation proposal for owner reaction.

## 1. DataFlow dump (`-df-out <PATH>` working name)

### 1.1 Design idea

The engine is push-based: a view receives rows from its producers and
pushes its output columns to every user. So the natural textual form
IS the owner's tail-call form: one block per view, block parameters =
the columns the block RECEIVES, terminator = one tail call per user
edge (fan-out = push broadcast). Fan-in becomes multiple callers of
one block — MERGE is exactly a block-arguments join point (φ). JOIN
is NOT a plain φ (it is stateful: hash-join over materialized sides),
so a JOIN block declares PORTS (lhs/rhs/pivot) rather than plain
args; each producer tail-calls a specific port.

### 1.2 Sketch (transitive closure, abridged)

    ;; module: tc(F,T) : edge(F,T).  tc(F,T) : tc(F,X), edge(X,T).

    recv ^select.1 () -> (F:u64, T:u64)        ; #message edge/2
      => ^merge.4 (F, T)
      => ^join.7 .rhs(X=F, T)

    merge ^merge.4 (F:u64, T:u64)              ; callers: ^select.1, ^tuple.9
      table %tc  class=differential
      => ^join.7 .lhs(F, X=T)
      => ^insert.12 (F, T)

    join ^join.7 [pivot X:u64] {
      .lhs <- ^merge.4  (F, X)                 ; tc keyed on To
      .rhs <- ^select.1 (X, T)                 ; edge keyed on From
    } -> (F:u64, T:u64)
      => ^tuple.9 (F, T)

    tuple ^tuple.9 (F:u64, T:u64)
      => ^merge.4 (F, T)                       ; the inductive back-edge

    insert ^insert.12 (F:u64, T:u64) into %tc  ; terminal

Conventions:
  - Blocks emitted in VIEW-ID ORDER, named `^<kind>.<id>`; columns as
    `<var-name-or-c<id>>:<type>` (variable names where the parse
    gives them; column ids on ambiguity).
  - `=>` lines are the tail calls, one per user COLUMN EDGE, in
    (user-id, port) order; the argument list is the column map at
    that edge (renames written `dst=src`).
  - Back-edges are ordinary tail calls (the graph's cycles are the
    fixpoints); a `; back-edge` comment marks user id ≤ def id.
  - Node-kind-specific headers: CMP carries the predicate; MAP the
    functor name + binding pattern; NEGATE the context kind
    (@never/context-keyed) and its negated side as a port; AGG/KVIND
    the group/config/summary column classes + algebra attribute;
    SELECT the relation/stream + receive kind; INSERT the target
    relation (terminal — no tail calls).
  - ATTRIBUTES line per block where present: backing table + class
    (differential/monotone/table-less), is_condition/unit-relation,
    group_ids elided by default (a `-df-out-verbose` can add them),
    and the producer tag for pass-minted views (e.g.
    producer=DEMAND-GUARD) — this last one is load-bearing for D1
    review: the demand pass's annotation sites become VISIBLE in the
    dump.

Open questions for the owner / the critique round:
  Q1 One block per view with `=>` fan-out (above) vs true CPS (each
     edge a distinct continuation block) — the former is flatter and
     diffable; CPS doubles block count for no review value. Draft
     picks the former.
  Q2 Where clause/rule provenance goes (a `; from clause N` comment
     per block?) — cheap and useful, but only where the parse links
     survive optimization.
  Q3 Whether the dump is pre- or post-optimization — BOTH are wanted
     ((F)-style debugging wants post; SIP/demand review wants the
     minted graph pre-CSE). Proposal: dump at the -df-out call site =
     final built Query (post-optimize); a second flag position can
     come later if needed.

## 2. DeltaRel dump (`-deltarel-out <PATH>` working name)

Emitted from the checked model AFTER validation (so a dump always
describes a graph the validators passed), in this order:

    deltarel <module-name>
    stratum <k> band=<band-key>                 ; derived order
      vec $<role>.<id> <col-types> def=[op ids] use=[op ids] carried=<role>
      ...vecs in id order...
      op.<id> <KIND> sign=<+|-|·> pos=<InNew|InI|...> claim=<ctx>
        reads: <membership predicates, the ten-predicate vocabulary>
        writes/effects: {<effect set>}
        spine: <access-plan spine — table/index ids in plan order>
        args: <table/view/vec references by id>
      ...ops in CHECKED-LINEARIZATION order (the Kahn output — which
         is already id-tie-broken by the band-key rule; the dump
         reuses the linearizer's list, never re-sorts)...
    census: <the P0 census counts as emitted>

Determinism argument: every line keys off ids (op id, vec id, view
id, table id) or the checked linearization; no pointer values, no
container iteration order. If the linearization itself is
nondeterministic (a (F) candidate), the dump EXPOSES it — that is a
feature; the dump lands before/with the (F) fix precisely to serve
as the between-layers discriminator.

Reconcile against the drir lane report before coding: exact op-kind
names, attribute fields, effect-set vocabulary, whether census
counts are cheaply re-emittable at dump time.

## 2.5 IR goldens (owner directive 5): the harness hook

The dumps are golden SURFACES, not only debug output. Harness shape
(mirrors .batches/.drflags — per-case opt-in sidecars, authoritative
scripts runall.sh/diffrun.sh):

  - `<name>.dfgold` present → diffrun.sh additionally runs the
    compiler with `-df-out` and byte-compares against
    goldens/<name>.df.stdout-style golden.
  - `<name>.deltarelgold` present → same for `-deltarel-out`.
  - MODE PINNING: IR is mode-sensitive (opt/nodf/nocf/none emit
    different graphs/programs), so the sidecar's first line names the
    mode(s) covered (default: opt only). The 4 execution modes stay
    untouched for stdout goldens.
  - Blessing: ONLY via `runall.sh --bless` after review — never
    auto-on-red (the standing policy verbatim).
  - permcheck.py stays a stdout-token referee; IR goldens are strict
    byte-compare (an IR permutation is exactly what they exist to
    catch).
  - First carriers: curated directed witnesses (fixpoint_stress_1,
    reconverge_1, demand_tc_witness + the aggregate corpus), not the
    full 168 — churn on optimization work stays reviewable.
  - demand_tc_witness's demand-ON IR+header goldens ARE the restored
    (F) gate once the fix lands.

## 3. Sequencing (per KeyedInstances.md §0.5)

rename lib/DR→lib/DeltaRel (post-fleet, byte-identity-gated) →
-deltarel-out + -df-out dumps (deterministic by construction; suite
byte-identity flag-off; new flags never touched by runall.sh) →
(F) fix using all three surfaces (.df/.ir/.deltarel + .h) as
discriminators → the epoch's D1+ work reviews all IRs end-to-end
per the standing discipline.
