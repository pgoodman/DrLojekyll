#!/usr/bin/env bash
# Copyright 2026, Peter Goodman. All rights reserved.
#
# Deterministic regenerator for the mono double-join witness dumps used by the
# CostModel grounding (grounding-double-join.md) and the identity-join
# recognizer calibration (measured-calibration-1.md).
#
# WHY THIS EXISTS: the checked-in `mono.demand.*` dumps were once byte-identical
# to the recognizer-OFF counterfactual while being labelled as if they were the
# default `-demand` output (cost-model-findings.md #8 / H8). This script makes
# every artifact reproducible from tracked source + explicit flags, and names
# each file after its exact configuration so a future reader cannot confuse the
# recognizer-on and recognizer-off forms. Re-run after any change that could
# move the mono witness's dataflow/rel shape, then review the git diff before
# committing.
#
# Usage:  DR=build/debug/bin/drlojekyll ./regen-mono-artifacts.sh
#         (DR defaults to build/debug/bin/drlojekyll relative to repo root)

set -euo pipefail

here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo="$(cd "$here/../../.." && pwd)"
DR="${DR:-$repo/build/debug/bin/drlojekyll}"
CASE="$repo/tests/OptDiff/cases/demand_neighborhood_mono_witness.dr"

if [ ! -x "$DR" ]; then
  echo "compiler not found/executable: $DR" >&2
  echo "build it first: cmake --build --preset debug" >&2
  exit 1
fi

cd "$here"

# normal: no demand transform runs, so the identity-join recognizer has nothing
# to fire on -- this dump is recognizer-config-independent (verified: 0 joins
# with df.ident_join both on and off).
"$DR" "$CASE" -df-out mono.normal.df -rel-out mono.normal.rel

# -demand, recognizer ON (the current default): the query-projection guard
# join.7 is dropped as a provable identity join. 1 df join, kEagerJoin=2,
# kJoinEmit=1.
"$DR" "$CASE" -demand \
  -df-out mono.demand.ident-join-on.df -rel-out mono.demand.ident-join-on.rel

# -demand, recognizer OFF (the counterfactual, df.ident_join disabled): the
# redundant projection guard join.7 survives. 2 df joins, kEagerJoin=4,
# kJoinEmit=2. This is the config the old mislabelled mono.demand.* matched.
"$DR" "$CASE" -demand -opt-disable=df.ident_join \
  -df-out mono.demand.ident-join-off.df -rel-out mono.demand.ident-join-off.rel

echo "regenerated mono artifacts in $here:"
for f in mono.normal.df mono.demand.ident-join-on.df mono.demand.ident-join-off.df; do
  printf '  %-40s df joins=%s\n' "$f" "$(grep -c '^join ' "$f" || true)"
done
