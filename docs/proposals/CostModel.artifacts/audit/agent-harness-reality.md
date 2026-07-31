# Agent Harness and Reverse-Engineering Reality Boundary

The proposed long-term application is a good fit for Datalog's strengths:
maintaining derived state over changing evidence, explaining why work is
runnable or blocked, and coordinating several analyses over a shared fact
graph. It is not yet safe to equate a derived fact with a real-world action.
The boundary between logical intent and an external effect is the central
architecture problem.

## A Concrete Flagship: Investigation Coordinator

Build one application that coordinates a reverse-engineering investigation and
can later generalize to personal agents. Its useful question is:

> Can attacker-controlled input reach an indirect call, and what work or
> evidence would change that conclusion?

The deterministic test scenario ingests artifacts such as binary identities,
functions, instructions, call and data-flow edges, decoded operands, and prior
findings. It also ingests goals, task dependencies, agent capabilities, leases,
tool results, evidence support/refutation, and invalidations.

The program derives:

- call-graph and task-dependency reachability;
- evidence that supports or refutes a hypothesis;
- runnable tasks whose prerequisites are satisfied;
- blocked tasks and their concrete blockers;
- the best eligible worker for a task under a declared deterministic policy;
- investigation goals affected by a corrected artifact or finding;
- command intents to dispatch, cancel, or re-check work;
- user-facing answers such as `why_blocked`, `evidence_for`, and
  `affected_goals`.

A scripted C++ world adapter supplies deterministic tool and agent results. A
scenario can ask a decoder for a function, a CFG recovery worker for edges, a
specialist analysis worker for an indirect-call classification, and an
independent verifier for the final hypothesis. These are fake collaborators in
correctness tests, but they exercise the same typed command/result protocol a
real adapter would implement.

The scenario should include a correction: a newly decoded edge invalidates an
earlier no-path conclusion, cancels or supersedes dependent work, and causes
the affected hypothesis to be recomputed. A second independent goal should be
active at the same time so keyed demand isolation and cross-goal contamination
are observable.

## Natural Language Feature Roles

This is not a demand to use every feature decoratively. Each feature must earn
its place:

| Feature | Domain role |
|---|---|
| Recursion | Call-graph reachability and transitive task prerequisites |
| Stratified negation | Runnable only if no blocker, active lease, or invalid evidence exists |
| Differential messages | Corrected artifacts, revoked evidence, task/result invalidation |
| Demand/keyed instances | Materialize one investigation goal and its relevant subgraph |
| Multiple adornments | Query from goal to artifacts and from artifact to affected goals |
| Aggregate/KV | Count or summarize nonrecursive/base evidence per hypothesis |
| Custom functors | Normalize addresses and deterministically classify capabilities |
| Product/forcing | Select one dispatch candidate only where the semantics justify it |
| Publish/subscribe | Emit typed effect intents and state-change notifications |

Current aggregate restrictions matter. If aggregate inputs owned by induction
remain unsupported, the flagship should aggregate base evidence only and carry
a separate expected-diagnostic witness for the tempting recursive form. A
single application is not useful if it succeeds only by hiding unsupported
semantics.

## Derived Facts Must Not Directly Perform Effects

Datalog evaluation may revisit, retract, or rediscover conclusions. External
systems do not rewind when a relation changes. Sending an email, editing a
file, invoking a paid model, or asking another agent to mutate a repository is
not a pure consequence that can be silently replayed.

The required boundary is an effect protocol:

```text
logical engine:
  derives EffectIntent with stable identity and current justification

durable dispatcher:
  claims intent, invokes adapter at least once, records acknowledgement/result

adapter:
  uses idempotency key, reports typed outcome, never pretends uncertainty is
  success

logical engine:
  consumes acknowledgement/result as new facts and derives subsequent state
```

At minimum, name these domain values rather than passing strings and integers:

```text
InvestigationId
TaskId
AttemptId
EffectIntentId
IdempotencyKey
ArtifactId
EvidenceId
AgentId
LeaseEpoch
ResultRevision
```

An effect lifecycle needs explicit states, for example:

```text
Requested -> Claimed -> Dispatched -> Acknowledged -> Completed
                                      \-> Failed
Requested/Claimed/Dispatched -> CancellationRequested
Completed + newer invalidation -> Superseded
```

The exact state machine requires adjudication. `None`, absence from a map, or a
warning-and-continue path must not encode timeout, rejection, cancellation,
conflict, and unknown as if they were one state.

## Exactly Once Is Not a Credible Default Claim

Across a process boundary, the dispatcher can crash after an external system
accepts a request but before the local acknowledgement is durable. Retrying may
duplicate the effect; not retrying may lose it. The engine cannot infer which
happened from in-memory state.

The realistic default is at-least-once dispatch with idempotent adapters and a
stable idempotency key. Effects that cannot be made idempotent need an explicit
human-approval or reconciliation policy. “Exactly once” should be removed from
any future proposal unless the claim is restricted to a transactional boundary
that genuinely owns both the durable intent and the effect.

**Disposition: fundamental design decision.** This cannot be fixed by adding a
callback to generated code. Choose the durability owner, delivery contract,
and reconciliation policy before enabling irreversible adapters.

## Cancellation and Late Results Are Normal State

An investigation can invalidate a task while an agent is running. The external
worker may not observe cancellation, or may return after a newer attempt has
completed. The result protocol therefore needs both task identity and attempt
or lease epoch. A result is applicable only if its revision/attempt is current
under a declared policy.

Required deterministic tests include:

- cancellation before dispatch;
- cancellation after dispatch but before acknowledgement;
- a late completion from a superseded attempt;
- duplicate completion of the same attempt;
- retry after lease expiry;
- two workers racing to claim the same intent;
- corrected evidence arriving while dependent work is active;
- replay of the complete durable event log after a crash.

Dropping a stale result may be correct for scheduling while still retaining it
as auditable evidence. The program must distinguish “not applied” from “never
received.”

**Disposition: design decision plus fixable implementation.** Decide the state
machine first; then encode each transition as an exact model-based test.

## Persistence Is Currently a Product Gap

The generated runtime observed in this audit is an in-process incremental
database. That is sufficient for deterministic correctness scenarios, but not
for a durable personal-agent coordinator. Process loss cannot also mean loss of
which external actions were requested, acknowledged, or awaiting
reconciliation.

Plausible ownership choices are:

1. The Datalog runtime owns a durable event log and recoverable materialized
   state.
2. An external harness owns an append-only log and reconstructs the Datalog
   state by replay.
3. A transactional store owns intents/results, and the engine is a derived
   projection over that store.

All can work. Maintaining two as fallback modes cannot. The choice affects
message identity, ordering, compaction, query consistency, and what a CostModel
scenario must represent.

**Disposition: owner adjudication.** For a first flagship test, use an explicit
in-memory event log with deterministic replay. Do not describe it as production
durability. Before real effects, select one durable owner and test crash points.

## Agent-Authored Programs Add a Safety Boundary

An LLM-authored Datalog program is untrusted policy. Even a type-correct,
terminating program can derive an unsafe or unexpectedly expensive intent.
The effect adapter must enforce capabilities independently of the program:

- which tools and paths an agent may access;
- spend, rate, concurrency, and recursion budgets;
- whether an effect needs human approval;
- which result schemas and provenance are accepted;
- maximum task/delegation depth;
- whether another agent may delegate further;
- deterministic denial behavior that becomes a typed fact, not a muted error.

These are justified boundary policies, not a request for arbitrary environment
flags. They should be immutable inputs to a run and visible in the audit log.
The generated program should derive requested capabilities; a trusted harness
grants or denies them.

Termination deserves separate attention. Datalog fixpoint termination does not
bound a workflow that can create fresh task IDs or prompt another agent to
create more goals. Budgets must operate on named quantities such as
`MaxDelegationDepth`, `MaxExternalAttempts`, or `SpendLimit`, and exhaustion
must be a modeled outcome.

**Disposition: fundamental product boundary.** Do not let generated code call
arbitrary adapters directly. A capability-checking dispatcher is mandatory for
the eventual harness.

## Relation to Rel and CostModel

The flagship application is valuable evidence for the Rel migration because
it combines scheduling, demand, invalidation, and publications in a way small
cases do not. It should run through the same pass covering array and pin only
the Rel facts whose ownership is under adjudication.

It also clarifies CostModel scope. Static plan facts may estimate logical work,
but real agent cost depends on a concrete event trace and external outcomes:
timeouts, retries, fanout, late results, and capability denials. A CostModel
that claims end-to-end agent cost from a relation census alone is dead on
arrival. The concrete-trace interpreter can model declared deterministic
adapter costs; observed network/model latency and spend belong to the runtime
measurement product.

Use distinct quantities:

```text
LogicalOperationCount
ExternalAttemptCount
TokenCount
MonetaryCost
WallDuration
RetryCount
```

They cannot share an untyped `double` score. Optimization of one may worsen
another, so any policy that combines them needs named weights and an explicit
objective, not an accidental scalar.

## Incremental Delivery Path

1. Implement the deterministic investigation facts and exact read-only query
   answers without external dispatch.
2. Add typed `EffectIntent` output and a recording no-op dispatcher. Verify
   exact intents, but perform no real effect.
3. Add the scripted adapter, attempt identities, cancellation, duplicate, and
   late-result scenarios with full replay.
4. Run the pass covering array, oracle comparisons, metamorphic variants, and
   selected Rel structural pins.
5. Adjudicate and implement durable ownership.
6. Add one reversible local adapter under an explicit capability policy.
7. Add irreversible or paid adapters only after idempotency, approval, crash,
   and reconciliation tests exist.

This path produces something visibly useful at step 1 while keeping the claims
honest. The deterministic application is fixable now. Durable real-world
effects and safe agent-authored policy need deeper adjudication; treating them
as ordinary callback plumbing would make the eventual system fail at its most
important boundary.
