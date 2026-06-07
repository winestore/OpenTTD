# State of the Engine — OpenTTD Subsystem Review (Wave 1)

*A clean-room study of OpenTTD's architecture across nine subsystems, distilled into
decisions for our own original game. We extract **knowledge**, not GPL code. For each
system: how it works, what we take, what we drop, and the traps to avoid. Companion to
`TYCOON_DESIGN_REFERENCE.md` (economy/finance/AI deep-dive) and `GAME_DESIGN_V1.md`.*

---

## 0. The five cross-cutting decisions (read this if nothing else)

These emerged from *every* subsystem and matter more than any individual feature:

1. **Determinism from day one — adopt the command pattern.** Every game-state change
   goes through a `Command` object (test → execute → log). Simulation runs on a fixed
   **tick**, never on frame `delta`. RNG is seeded from the tick, never `randf()`. Money
   and positions use **integer math**, not floats. This one decision buys us undo/redo,
   perfect replays for bug reports, AI scripting, and multiplayer — *for free, later* —
   but only if we build it in now. Retrofitting it is a rewrite. **Highest-leverage call we make.**

2. **Split world-generation from simulation.** Generate the map once (heightmap → place
   towns/industries, no growth). All growth/production happens only in the tick loop.
   Far easier to build, test, and reason about than OpenTTD's unified approach.

3. **Save/load discipline starts now, content-modding waits.** Chunk-based, **named
   fields** (not positional), religiously versioned, with a pointer-fixup pass. OpenTTD's
   250 save versions taught one lesson: the format is easy, the *discipline* is hard.
   Bake it in at v1; it's brutal to retrofit. NewGRF-style modding is a post-launch concern.

4. **Aim for ~10x less code by dropping the "complexity multipliers."** Across the board,
   OpenTTD's depth comes from features that multiply state: shared orders, articulated
   vehicles, implicit orders, probabilistic cargo routing, presignals, NewGRF callbacks,
   toroidal map wrap. We drop these for v1. Same *feel*, a fraction of the surface area.

5. **Our signature screen has no reference to copy.** The share-price / takeover UI is the
   one major screen OpenTTD never had — so it's pure design space, not a port. Everything
   else (finances, vehicle lists, graphs) maps cleanly onto Godot's native UI nodes.

---

## 1. World, Map & Terrain

**How it works:** A flat array of 12-byte tiles indexed by a packed 32-bit `(x,y)`. Each
tile has a type, a corner height (0–255), and packed owner/feature bytes. World-gen is a
13-phase pipeline: heightmap (Perlin or template) → water → climate zones → rivers → towns
→ industries → trees. Towns place on a grid layout then grow via spiral search; industries
place by weighted-random with minimum counts.

**What we take:** The tile-array model and packed `(x,y)` index. Perlin/Simplex heightmap
gen. Weighted-random industry placement with forced minimums. Four town layouts.

**What we drop / simplify:** 8-byte tile (drop the NewGRF bytes). Drop the template-based
generator (Perlin only). **No town growth during generation** — place and stop, grow in the
tick loop. Single house type per climate. Disable toroidal map wrap (clamp coords). Drop
half-tile slopes (19 slope types, not 32).

**Traps:** RNG call-count determinism (same seed must mean same map — audit every gen RNG
call). Perlin edge cliffs at map boundary (over-generate then crop). Water-vs-slope ordering
(convert water *before* fixing slopes, or keep water flat). Grid-layout misalignment silently
breaks town growth.

---

## 2. Transport Infrastructure & Pathfinding

**How it works:** Track stored as a 6-bit "TrackBits" mask per tile plus a separate type
enum. Signals are per-track state (block + path-based/PBS). Pathfinding is **YAPF** — A\*
with segment-based cost caching, polynomial signal penalties, and **path reservation to a
"safe waiting position"** so trains never deadlock. Bridges/tunnels store only their
endpoints; the pathfinder jumps the middle.

**What we take:** A\* with segment caching — its architecture is near-optimal for a small
team. The safe-waiting reservation idea (prevents the deadlocks that plague transport sims).
Endpoint-only bridge/tunnel representation. `(tile, direction)` as the node key (handles
loops).

**What we drop / simplify:** **Start with road vehicles, not signaled rail** (our prototype
already does — validated). Simplify the cost model to base + curve + slope + red-signal; drop
platform-length matching, doubleslip, PBS reservation costs initially. Block signals only
(red/green); defer presignals forever unless players demand a routing mini-game. Cap search
at ~5k nodes. Queue pathfind requests — never pathfind every frame during congestion.

**Traps:** **This is the genre's #1 complexity sink.** Without safe-waiting reservation,
trains block each other on junctions. Diagonal+orthogonal movement invites "knight-move"
shortcut abuse (penalize 90° turns or ban diagonals). Stale path caches point to deleted
track (version-invalidate on tile change).

---

## 3. Vehicles, Stations & Orders

**How it works:** ~15k lines — the heaviest subsystem. Vehicles are pool-based, chained into
"consists," optionally sharing one order list. Movement is progress-based (`progress += speed`,
advance when it crosses a tile threshold). Orders are a typed list (goto-station/depot/waypoint,
conditional, implicit). The loading cycle moves cargo packets between station and vehicle lists,
gated by station rating; cargo routes via probabilistic **FlowStat** tables.

**What we take (~40-50%):** Consist chains (one primary vehicle executes orders).
Progress-based movement. The full order/timetable model (it's good, ~90% reusable). The
unload→refit→load loop. Station rating (time-since-pickup, last-speed). Cargo packets tagged
with origin/destination/age. Viewport spatial hashing and per-direction sprite caching.

**What we drop / simplify:** **No shared orders** (huge state-complexity win; each vehicle owns
its list). No articulated parts. **No implicit orders** (players route explicitly). No FlowStat
graph routing — cargo just follows the next order. Single cargo type per vehicle (refit in depot).
Simple catchment radius, no acceptance-tile complexity. Target <5k lines.

**Traps:** The implicit-vs-real order-index duality (we sidestep by dropping implicit orders).
Mutating a consist mid-loading-cycle crashes iterators (queue refits to tick boundary).
Full-load reservation can starve other vehicles. **New-station rating death-spiral** (rating
decays before supply builds — surface it in UI). Gradual-loading reads as "stuck vehicle" to
players (tooltip it).

---

## 4. GUI Framework & Information Presentation

**How it works:** A declarative nested-widget tree per window, with separate
size-computation and draw passes, a z-ordered window list, and contextual show/hide "planes."
All UI actions post **commands** (same system as everything else). Rich info screens:
multi-year finances grid, 5 company graphs, sortable vehicle lists, color-coded minimap.

**What we take (as UX patterns, not code — Godot has its own UI):** The information
architecture — finances cockpit, graphs, sortable vehicle list. The size-vs-draw separation
(`_on_resized` measures, `_draw` renders). Contextual button show/hide. Data-binding via signals
(`money_changed` → window invalidate). int64 money from day one.

**What we drop / modernize:** TTD's fixed 1024×768 → responsive anchors. Brown/grey → dark
theme. Add a **takeover panel** (stock price, cap table, hostile-offer slider), a cash-flow
waterfall, a loan amortization view — all things TTD lacked.

**Traps:** Don't access widgets before the tree is built. `ReInit()` (full relayout) is
expensive — toggle visibility instead. A company can be deleted while its window is open
(null-check every frame). InvalidateData is queued — expect one stale frame.

---

## 5. Modding & Data Architecture (NewGRF + Save/Load)

**How it works:** NewGRF is a 5-stage declarative content loader (label-scan → safety →
init → reserve → activate) that injects vehicles/industries/graphics without touching core
code. Save/load is metadata-driven: a `SaveLoad` descriptor per field with a version range,
one `ChunkHandler` per subsystem, a pointer-fixup pass, and a compat layer for ancient saves.

**What we take:** The chunk-based save architecture — one handler per subsystem, declarative
fields, `FixPointers()` post-load. Data-driven content definitions (JSON/TOML internally) *even
before* exposing modding, so the structure is mod-ready later. GRF-style `(modid, md5)` stamps
when modding does arrive.

**What we drop / defer:** All of NewGRF for v1 — hard-code content. Callback scripting is the
last thing to add, if ever.

**Traps:** Save-version numbering hell (increment only at release, CI-check monotonicity, keep
old saves in the test suite). Missing a pointer in fixup = silent corruption. **Use named field
headers from day one** — positional fields make reordering impossible without breaking saves.
Settings drift (version-gate and migrate, never delete old setting descriptors).

---

## 6. Engine Runtime, Game Loop & Multiplayer

**How it works:** Three independent clocks — **tick** (74/day, never pauses, drives physics),
**calendar** (cosmetic dates/aging), **economy** (finances/production). All state changes flow
through `DoCommand`: a test run validates and prices, then a real run executes; in multiplayer
the command is stamped with a future frame number and broadcast, so every client executes the
same commands in the same order and stays in lockstep. Desync is caught by logging RNG state daily.

**What we take:** The tick/calendar split (start with tick + cosmetic calendar; defer separate
economy time). **The command pattern in full** — the cross-cutting decision #1. Deterministic
RNG seeded from tick+category. The test/execute dual-path (price before committing — maps
perfectly onto "evaluate an acquisition before bidding").

**What we drop / defer:** Three clocks → one-and-a-half for v1. Network dispatch itself is
deferred — but we design commands as serializable data now so multiplayer is additive, not a rewrite.

**Traps (determinism pitfalls):** Using `delta` in simulation (use ticks; interpolate only for
rendering). Floats in money/position (use integers/fixed-point). Unsorted dict/array iteration
(sort before simulating). Side-effects inside commands (defer to tick boundaries). Test/execute
logic mismatch (guard only *mutations* with `if not test`, never logic).

---

## 7. Updated build order (revised by these findings)

The reviews reshuffle the plan around the determinism foundation:

1. **Engine spine first:** `GameState` + tick loop + `Command` base (test→execute→log) +
   deterministic RNG + chunked save/load. Unglamorous, but everything sits on it.
2. **World:** tile array + heightmap gen + place towns/industries (no growth yet).
3. **Operator loop:** cargo payment, industry production, town growth — *in the tick loop*.
   (Our prototype already proves this is fun-shaped.)
4. **Vehicles/stations/orders:** the simplified ~5k-line version. Road first, rail later.
5. **Finance + rivals:** valuations, loans, AI personalities. (Prototype v2 has a rough cut.)
6. **The differentiator:** share market, ownership ladder, LBOs — the part with no reference.
7. **UI polish, then defer:** rail signaling, multiplayer, modding — all additive by design.

---

## 8. Decisions we now need to make

- **Engine language in Godot:** GDScript (fast to write, but float-prone and slower) vs **C#**
  (better for deterministic integer-heavy simulation and large state). The determinism findings
  lean toward C# for the simulation core. *Worth deciding before step 1.*
- **Fixed-point scale:** what integer unit for money and sub-tile position? (e.g. money in whole
  dollars; position in 1/256-tile units.)
- **Tick rate:** 30/sec was suggested; ties into how fast calendar time should feel.
- **How hard to commit to multiplayer-readiness** now vs. accept some rework later. (The command
  pattern gets us most of the way regardless.)

These four feed directly into Wave 2 ("how we make it better"), which can now reason about a
concrete architecture instead of a blank page.
