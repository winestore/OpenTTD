# HOSTILE — Operator Loop Prototype

A tiny, deliberately ugly playable test of the **core transport loop** — the
"is it fun to run a route?" question — before we build the share market, AI
rivals, or LBO mechanics.

## Run it

1. Open **Godot 4.6.3**.
2. *Import* → select this folder's `project.godot`.
3. Press **F5** (Play).

## What you're testing

- Left-click an **industry** (red square), then a **town** (blue circle) to
  build a route. It costs money (distance + one truck).
- The truck shuttles cargo; you get **paid on delivery** (green `+$` popups).
- Delivering cargo **grows the town** (population + size).
- Press **B** to add another truck to your most recent route.
- **Space** pauses; **Esc** cancels a selection.

## The questions to answer while playing

- Does connecting a route and watching the money tick feel satisfying?
- Is there a meaningful choice between short routes (less pay, faster) and long
  ones (more pay per trip, slower)? *(That trade-off lives in the payment
  formula in `Main.gd`.)*
- Do you find yourself *wanting* to expand? That pull is what the whole game
  hangs on.

If yes → we layer the financial game on top. If no → we fix the core first.

## Tuning

All the knobs (truck cost, pay rate, production speed, growth threshold) are
constants at the top of `Main.gd`. Tell me how it feels and I'll adjust them —
or change them yourself and re-run.
