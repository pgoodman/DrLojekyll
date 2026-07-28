// Copyright 2026, Peter Goodman. All rights reserved.
//
// negate_never_diff_1 is an expected-diagnostic case (see the .dr header):
// '@never' over a differential negated view — the DS-R4-10 post-fixpoint
// fence must exit 1 with a rendered diagnostic in all 4 modes, so this
// driver is inert and never compiled.
int main() {
  return 0;
}
