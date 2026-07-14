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

These became the standing OptDiff fixtures (product_diff, product_mixed,
product_self, product_conds) at the Stage-5 landing (2026-07-13) — see
tests/OptDiff/cases/ and stage5-notes.md's landing record. The drivers
observe the published delta stream by providing their own log type whose
member signatures match the published-message hooks (the entry points deduce
the log's static type -- nothing is virtual, no inheritance or overriding);
cross-message oracle batches split into one runtime
epoch per message (the runtime has one entry point per message), and the
same-epoch double flip lives in product_conds' second database instance
and product_self's mixed batch. Run: build/debug/bin/drlojekyll-oracle
<dr> <batches> [--project-monotone] or --stress <seed> <rounds>.
