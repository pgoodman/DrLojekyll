# D3.a — RITUAL-HEAD RULING BRIEF (RATIFIED)

> **House banner.** Tip **7fc23279** at presentation. Substrate:
> d3a-substrate.md (stage (a), §20(AD)); the twelve §6 open questions ruled
> here. RATIFICATION RECORD at the tail. These rulings are BINDING for every
> D3.a stage-(b) design lane; they fold into the d3a sub-slice design docs as
> the slices land. Do not re-litigate (the OD-1..14 / RAT-1..10 / ADJ-*
> discipline applies).

One section per ruling; recommendation first, grounds compressed (full
grounds in the session presentation; decision inputs = d3a-substrate.md §5-§6
anchors).

## OQ-MODEL — RULED: the FULL-RESCAN model.
The store stays a predicate-free set island; an instance's content is rebuilt
from scratch per touched epoch; differentialness is computed by DIFFING frozen
vs current at publish — (F,T) born (already emitted) + (T,F) dropped (the new
scan). Moots G-DIFF-TABLE / G-ROW-RETRACT / G-OCCUPANCY (current is only ever
built by a fresh monotone rescan; NumRows()>0 stays exact). CONSEQUENCE
(binding frame): D3.a is a pub-BOUNDARY + LIFECYCLE problem, not a
store-representation problem.

## OQ-AXES — RULED: two axes, staged.
D3.a.1 = differential DEMAND (retract/death; inputs monotone) on the existing
`TableIsDifferential(demand_table)` mint gate. D3.a.2 = differential
INPUT/content (lift FENCE (iii) + the V-INST-SOLE forbiddance). Independent
fences, independent lighting.

## OQ-RETRACT-POLICY — RULED: SET-demand + batch SET netting.
Any retract kills the key's demand; add∩remove within one batch annihilates
(the OQ3 user-message semantics verbatim). No ref-counting (a consumer
needing it carries a token column). Channel: fabricated message goes
`@differential` → the injector's existing IsDifferential del_vec arm
(Build.cpp:416-421) → differential demand relation → death on net-removal.

## OQ-N1 — RULED: no signed count; N-1 CLOSES as moot-under-rescan.
Recorded contingency: reopens only if OQ-MODEL is ever overturned.

## OQ-BELT — RULED: RAT-7 resolves YES.
A runtime band-(b) PARTITION belt lands WITH the (T,F) scan (always-on
counters: born + carried == cur.NumRows AND dropped + carried ==
frz.NumRows), the differential-mode invariant. HP-7's frozen⊆current belt
stays armed for monotone-constructed stores; the RAT-4 bool gets its selector
plumbing (with the XC-2 region diff bit).

## OQ-PUBLISH-ORDER — RULED: no bespoke ordering.
Signed deltas into pub's existing machinery ((T,F) → delete side, (F,T) → add
side); cross-instance same-row conflicts net in pub's per-row counters.
Within a store: drop scan before born scan per touched iid (OVERDELETE-first
discipline).

## OQ-DEATH-VS-REBUILD — RULED: death = full (T,F) retract of frozen rows +
RecycleCurrent; rebirth = ordinary band-(a1) rebuild in a later epoch.
PINNED THREE-WAY COUPLING (state it in every affected design): batch SET
netting makes same-batch death+re-demand impossible; death's Touch suppresses
a same-epoch a2 rescan of the dead key via TouchedFlag; V-INST-FRESH keeps
its abort unchanged because Recycle leaves current empty. No iid tombstone —
namespace append-only; a dead key rebids via FindOrAdd.

## OQ-INPUT — RULED: YES, as sub-slice D3.a.2.
The input table gains a net-REMOVALS frontier as a second a2 trigger — any
input change (either sign) for a live-demanded key fires RecycleCurrent +
full rescan; band-(b) diff-at-publish emits the net retractions. No per-row
deletes in the store, ever. G-STALE subsumed.

## OQ-ADORN-KEY — RULED: N disjoint stores, one per (query, BindingPattern)
forcing. The pass loops STEP 1b→10 per adornment; EVERY registry/lookup keys
on (query, BindingPattern) (today only the injector belt does). No union-key
store.

## OQ-OWN3 — RULED: promote View.cpp:588 to an always-on record-comparing
fprintf+abort (print both annotation records + forcing indices) AND make the
post-Optimize census equation (n_stamped + folded == total) always-on with
it. Lands as D3.a.0 hygiene, before any multi-adornment admission.

## OQ-NEVER — RULED: REJECT (DS-R4-10 discharged as a fence).
Clean diagnostic: `@never` over a CanReceiveDeletions negated view, + a
directed witness case. Auto-promotion to `!` stays a possible later opt-in.

## OQ-INDUCTION-UNION — RULED: DEFER.
FENCE (i) (recursive demand) stands for ALL of D3.a. The §20(AB) precondition
binds any future slice that touches it: model the owning-merge union region
BEFORE relaxing NeedsInductionCycleVector.

## SUB-SLICE ORDER — RULED:
D3.a.0 (OWN-3 promotion + diff-selector plumbing: XC-2 region diff bit +
store monotone bool, all INERT) → D3.a.1 (differential demand:
-demand-retract, kInstanceDeath ON + its lowering, the (T,F) scan, the
partition belt, the demand net-removals frontier + V-INST-DRAIN extension) →
D3.a.2 (differential input: lift FENCE (iii) + V-INST-SOLE, the a2 removal
trigger) → D3.a.3 (multi-adornment). Each under the full per-slice ritual.
STANDING REFEREE NOTE: the flat -demand lowering is already
differential-capable (the fences are instance-only), so the eqgate's
flat==nested answer identity is the live cross-lowering oracle for every
sub-slice.

=====================================================================
RATIFICATION RECORD (owner, 2026-07-28, in-session): "ratify all" —
all TWELVE OQ rulings + the sub-slice order ratified AS RECOMMENDED.
These rulings are BINDING; stage (b) design lanes open under them.
=====================================================================
