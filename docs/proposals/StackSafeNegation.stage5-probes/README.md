# Stage-5 oracle-verified probes (pre-fixture material)

Probe programs + batch scripts for differential @product, each verified
against drlojekyll-oracle at the Stage-5 design pass (2026-07-12) BEFORE
any compiler support exists (the oracle interprets unoptimized dataflow IR
and never runs the CF-build F14 pre-pass):

- prod_diff.{dr,batches}  — both sides @differential; 7 batches: retract-
  one-side, same-batch +a3/-a2 phantom, +a5/-a5 annihilation, full drain.
  ORACLE: OK (898 assertions); --stress 42 200 clean.
- prod_mixed.{dr,batches} — monotone side x differential side; 5 batches.
  ORACLE: OK (494 assertions); monotone projection = 3.
- prod_self.{dr,batches}  — differential SELF-product (both sides one
  table); 4 batches. ORACLE: OK (435 assertions); --stress 7 150 clean.
- ctb.batches             — 3-batch script for
  data/examples/conditions_to_bools.dr (+enable 1 / +enable 2 /
  idempotent re-add). ORACLE: OK (727 assertions); monotone = 2.

These become the standing OptDiff fixtures (product_diff, product_mixed,
product_self, product_conds) per stage5-notes.md section 6: add drivers
with a custom DatabaseLog printing per-batch published deltas; author the
oracle/monotone golden pair from oracle truth any time; bless the stdout
golden only AFTER the compiler change lands (the oracle does not model
publishing). Run: build/debug/bin/drlojekyll-oracle <dr> <batches>
[--project-monotone] or --stress <seed> <rounds>.
