#!/usr/bin/env python3
r"""permcheck.py -- mechanized golden permutation check (DeltaRelationalIR.md
Sec 7.1 golden policy, Sec 9 F-8).

An R2 emission-family cutover may change a case's STDOUT ONLY as an
ORDER-PERMUTATION OF PUBLISHED DELTAS WITHIN ONE EPOCH. This tool verifies
mechanically that the diff between an old golden and a new stdout is EXACTLY
such a permutation -- never eyeballed.

    permcheck.py <old_stdout> <new_stdout> [--spec <case-spec>]

Model (surveyed from tests/OptDiff/cases/product_*.main.cpp, the only drivers
that print PUBLISHED DELTAS via deduced-log message hooks -- all other
differential cases observe via query drains):

  * The published-delta drivers (product_diff/product_conds/product_self/
    product_mixed) print ONE EPOCH PER LINE in the form

        <label>: <delta> <delta> ...

    where each delta token is `+(...)` or `-(...)`. The driver sorts within
    the epoch before printing, so today every emission mode agrees byte-for-
    byte. This check exists so that an R2 cutover that does NOT preserve that
    within-epoch order (e.g. an unsorted or one-delta-per-line driver) is
    still accepted iff the change is a pure within-epoch permutation.

  * SEGMENTS (epochs) are delimited by "boundary" lines. Within a segment,
    the multiset of "published-delta" tokens must be EQUAL (order-insensitive)
    and every OTHER line must be byte-identical IN ORDER.

  * Conservative: if the two files disagree on segment COUNT or on any
    boundary line, that is a FAIL -- never a resegmentation.

Configurable per case via an optional spec file (--spec). Spec is a tiny
KEY=VALUE file (# comments allowed). Recognized keys:

  boundary_re   Python regex; a line matching it starts a new segment AND is
                treated as structural (compared in order, not permuted). The
                match text (minus any trailing published deltas on the same
                physical line -- see inline_delta below) is the boundary key.
                Default: r'^\S.*:\s*$|^\S.*:\s'   (a non-indented label line
                ending in ':' optionally followed by inline deltas)

  delta_re      Python regex for a single published-delta TOKEN. Default:
                r'[+-]\([^)]*\)'
                Tokens are extracted wherever they occur (inline after a
                boundary label, or on their own lines). The residue of a line
                after removing all delta tokens + surrounding whitespace, if
                non-empty and NOT itself a boundary, is compared structurally.

  inline_delta  '1' (default) => delta tokens may share the boundary line
                (product_* form). '0' => deltas only appear on their own
                lines; a boundary line carries no deltas.
"""

import re
import sys

DEFAULTS = {
    # A non-indented label line ending in ':' (optionally trailed by inline
    # deltas). Two alternatives so both `label:` and `label: +(..)` match.
    "boundary_re": r'^\S.*:\s*$|^\S.*:\s',
    "delta_re": r'[+-]\([^)]*\)',
    "inline_delta": "1",
}


def load_spec(path):
    cfg = dict(DEFAULTS)
    with open(path, "r") as f:
        for raw in f:
            line = raw.rstrip("\n")
            s = line.strip()
            if not s or s.startswith("#"):
                continue
            if "=" not in line:
                die(f"bad spec line (no '='): {line!r}")
            k, v = line.split("=", 1)
            k = k.strip()
            v = v.strip()
            if k not in DEFAULTS:
                die(f"unknown spec key: {k!r} (known: {', '.join(DEFAULTS)})")
            cfg[k] = v
    return cfg


def die(msg, code=2):
    sys.stderr.write(f"permcheck: {msg}\n")
    sys.exit(code)


class Segment:
    def __init__(self, boundary_line):
        # boundary_line is the exact structural boundary text (None for a
        # leading pre-boundary preamble segment).
        self.boundary = boundary_line
        self.deltas = []      # multiset of published-delta tokens (order-free)
        self.structural = []  # non-delta, non-boundary lines, compared in order


def segment(lines, cfg):
    boundary_re = re.compile(cfg["boundary_re"])
    delta_re = re.compile(cfg["delta_re"])
    inline = cfg["inline_delta"] == "1"

    segs = []
    cur = Segment(None)  # preamble before any boundary
    cur_has_content = False

    for line in lines:
        is_boundary = bool(boundary_re.search(line))
        if is_boundary:
            # Close current segment, open a new one keyed by this line.
            if cur.boundary is not None or cur.deltas or cur.structural:
                segs.append(cur)
            elif cur_has_content:
                segs.append(cur)
            # For an inline boundary, split the physical line into the
            # structural boundary key (label + ':') and any inline deltas.
            if inline:
                deltas = delta_re.findall(line)
                # Structural boundary key = line with delta tokens removed,
                # right-trimmed so trailing spaces from removed tokens don't
                # perturb the key.
                key = delta_re.sub("", line).rstrip()
                cur = Segment(key)
                cur.deltas.extend(deltas)
            else:
                cur = Segment(line)
            cur_has_content = True
            continue

        # Non-boundary line: extract delta tokens as an order-free multiset;
        # the residue (if any real content) is structural, compared in order.
        deltas = delta_re.findall(line) if True else []
        if deltas:
            cur.deltas.extend(deltas)
            residue = delta_re.sub("", line).strip()
            if residue:
                cur.structural.append(residue)
        else:
            cur.structural.append(line)
        cur_has_content = True

    if cur.boundary is not None or cur.deltas or cur.structural or cur_has_content:
        # Avoid appending an empty synthetic preamble when file starts with a
        # boundary (cur would already have been the real seg in that case).
        if segs and cur is segs[-1]:
            pass
        else:
            segs.append(cur)
    return segs


def compare(old_segs, new_segs):
    """Return (ok, message, permuted_line_count)."""
    if len(old_segs) != len(new_segs):
        return (False,
                f"segment COUNT mismatch: old has {len(old_segs)} epoch(s), "
                f"new has {len(new_segs)}. Conservative policy: refusing to "
                f"resegment. First divergence in boundaries:\n" +
                first_boundary_divergence(old_segs, new_segs),
                0)

    permuted = 0
    for i, (o, n) in enumerate(zip(old_segs, new_segs)):
        if o.boundary != n.boundary:
            return (False,
                    f"segment {i}: boundary line differs (structural, must be "
                    f"byte-identical):\n  old: {o.boundary!r}\n  new: {n.boundary!r}",
                    permuted)
        # Structural lines must match in order, byte-for-byte.
        if o.structural != n.structural:
            hunk = first_list_divergence(o.structural, n.structural)
            return (False,
                    f"segment {i} (boundary {o.boundary!r}): non-delta output "
                    f"differs in order/content:\n{hunk}",
                    permuted)
        # Published deltas: multiset equality.
        om = sorted(o.deltas)
        nm = sorted(n.deltas)
        if om != nm:
            return (False,
                    f"segment {i} (boundary {o.boundary!r}): published-delta "
                    f"MULTISET differs (not a permutation):\n"
                    f"  only in old: {sorted(multiset_diff(o.deltas, n.deltas))}\n"
                    f"  only in new: {sorted(multiset_diff(n.deltas, o.deltas))}",
                    permuted)
        if o.deltas != n.deltas:
            permuted += len(o.deltas)
    return (True, "", permuted)


def multiset_diff(a, b):
    from collections import Counter
    ca, cb = Counter(a), Counter(b)
    out = []
    for k in ca:
        extra = ca[k] - cb.get(k, 0)
        out.extend([k] * max(0, extra))
    return out


def first_boundary_divergence(a, b):
    ab = [s.boundary for s in a]
    bb = [s.boundary for s in b]
    for i in range(min(len(ab), len(bb))):
        if ab[i] != bb[i]:
            return f"  at index {i}:\n    old: {ab[i]!r}\n    new: {bb[i]!r}"
    i = min(len(ab), len(bb))
    tail = ab[i:] if len(ab) > len(bb) else bb[i:]
    side = "old" if len(ab) > len(bb) else "new"
    return f"  extra segment(s) in {side} starting at index {i}: {tail[:1]!r}"


def first_list_divergence(a, b):
    for i in range(min(len(a), len(b))):
        if a[i] != b[i]:
            return (f"  first differing line at position {i}:\n"
                    f"    old: {a[i]!r}\n    new: {b[i]!r}")
    if len(a) != len(b):
        i = min(len(a), len(b))
        tail = a[i:] if len(a) > len(b) else b[i:]
        side = "old" if len(a) > len(b) else "new"
        return f"  {side} has extra structural line(s) at {i}: {tail[:1]!r}"
    return "  (lists equal? -- unexpected)"


def read_lines(path):
    try:
        with open(path, "r") as f:
            return f.read().split("\n")
    except OSError as e:
        die(f"cannot read {path}: {e}")


def main(argv):
    args = argv[1:]
    spec_path = None
    positional = []
    i = 0
    while i < len(args):
        a = args[i]
        if a == "--spec":
            if i + 1 >= len(args):
                die("--spec requires a path")
            spec_path = args[i + 1]
            i += 2
        elif a in ("-h", "--help"):
            sys.stdout.write(__doc__)
            return 0
        else:
            positional.append(a)
            i += 1

    if len(positional) != 2:
        die("usage: permcheck.py <old_stdout> <new_stdout> [--spec <case-spec>]")

    old_path, new_path = positional
    cfg = load_spec(spec_path) if spec_path else dict(DEFAULTS)

    # Validate regexes early with a clear error.
    for key in ("boundary_re", "delta_re"):
        try:
            re.compile(cfg[key])
        except re.error as e:
            die(f"invalid {key}: {e}")

    old_lines = read_lines(old_path)
    new_lines = read_lines(new_path)

    old_segs = segment(old_lines, cfg)
    new_segs = segment(new_lines, cfg)

    ok, msg, permuted = compare(old_segs, new_segs)

    if ok:
        nseg = len([s for s in old_segs if s.boundary is not None])
        print(f"PERMUTATION OK: {nseg} epoch/segment(s), "
              f"{permuted} published-delta line(s)/token(s) permuted "
              f"within-epoch; all other output byte-identical in order.")
        return 0
    else:
        print("NOT A PURE PERMUTATION -- FAIL")
        print(msg)
        return 1


if __name__ == "__main__":
    sys.exit(main(sys.argv))
