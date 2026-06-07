extends Node2D
## HOSTILE — Prototype v2: Rivalry + the first taste of takeovers
##
## v1 proved the transport loop works. v2 adds the parts that make this OUR
## game and not just OpenTTD-lite:
##   - a RIVAL AI company that builds routes and competes for the same cargo
##   - RUNNING COSTS, so money can go down and choices carry risk
##   - a SHARE PRICE per company, derived from net worth
##   - BUYING SHARES in a rival and TAKING IT OVER at >50%
##
## Controls:
##   Left-click an INDUSTRY (square) then a TOWN (circle) to build YOUR route.
##   Left-click a "BUY 5%" button (right panel) to buy into a rival.
##   B — add a truck to your most recently built route.
##   Space — pause/resume.   Esc — cancel a selection.

# --- Tunable constants ------------------------------------------------------
const TILE: float = 32.0
const START_MONEY: float = 3000.0
const BUILD_COST_PER_TILE: float = 8.0
const TRUCK_COST: float = 600.0
const TRUCK_VALUE: float = 360.0           # book value of a truck (for net worth)
const TRUCK_CAPACITY: int = 25
const TRUCK_SPEED: float = 130.0
const TRUCK_RUNNING_COST: float = 3.5      # $/second per truck — this is the "stakes"
const PRODUCTION_RATE: float = 6.0
const STOCK_CAP: float = 220.0
const CARGO_RATE: float = 1.0
const TIME_DECAY: float = 0.22
const TIME_FACTOR_FLOOR: float = 0.2
const GROWTH_THRESHOLD: float = 60.0
const POP_PER_GROWTH: int = 50

const SHARES_OUTSTANDING: float = 1000.0
const STAKE_STEP: float = 0.05             # buy 5% at a time
const STAKE_PREMIUM_SLOPE: float = 0.6     # creeping stakes get pricier
const CONTROL_THRESHOLD: float = 0.5       # >50% = takeover
const AI_BUILD_INTERVAL: float = 5.0

# --- State ------------------------------------------------------------------
var companies: Array = []    # { name, color, cash, is_player, alive, stake }
var industries: Array = []   # { pos, stock }
var towns: Array = []        # { pos, pop, received, radius }
var routes: Array = []       # { owner, from, to, length, trucks }
var floaters: Array = []
var buy_buttons: Array = []  # { rect, company } — rebuilt each frame

var paused: bool = false
var selected_industry: int = -1
var last_route: int = -1
var ai_timer: float = 0.0
var _font: Font

const PLAYER: int = 0


func _ready() -> void:
	_font = ThemeDB.fallback_font
	randomize()
	companies.append({"name": "You", "color": Color(0.3, 0.55, 0.9), "cash": START_MONEY, "is_player": true, "alive": true, "stake": 0.0})
	companies.append({"name": "Rival Co.", "color": Color(0.9, 0.5, 0.2), "cash": START_MONEY, "is_player": false, "alive": true, "stake": 0.0})
	_build_world()


func _build_world() -> void:
	for p in [Vector2(170, 220), Vector2(170, 560), Vector2(760, 720)]:
		industries.append({"pos": p, "stock": 40.0})
	for p in [Vector2(470, 250), Vector2(560, 470), Vector2(820, 320), Vector2(380, 640)]:
		towns.append({"pos": p, "pop": 100, "received": 0.0, "radius": 26.0})


# --- Main loop --------------------------------------------------------------
func _process(delta: float) -> void:
	if not paused:
		_simulate(delta)
		_ai(delta)
	_update_floaters(delta)
	queue_redraw()


func _simulate(delta: float) -> void:
	for ind in industries:
		ind["stock"] = min(STOCK_CAP, ind["stock"] + PRODUCTION_RATE * delta)

	for route in routes:
		var owner: int = route["owner"]
		var ind = industries[route["from"]]
		var leg: float = route["length"]
		for truck in route["trucks"]:
			# Running cost: every truck drains its owner's cash continuously.
			companies[owner]["cash"] -= TRUCK_RUNNING_COST * delta
			truck["progress"] += TRUCK_SPEED * delta / max(leg, 1.0)
			if truck["progress"] < 1.0:
				continue
			truck["progress"] = 0.0
			if truck["phase"] == "to_town":
				if truck["cargo"] > 0:
					_deliver(route, truck["cargo"])
					truck["cargo"] = 0
				truck["phase"] = "to_industry"
			else:
				# Both companies draw from the SAME industry stock — competition!
				var load_amt: int = int(min(float(TRUCK_CAPACITY), ind["stock"]))
				truck["cargo"] = load_amt
				ind["stock"] -= load_amt
				truck["phase"] = "to_town"


func _deliver(route: Dictionary, pieces: int) -> void:
	var town = towns[route["to"]]
	var leg_tiles: float = route["length"] / TILE
	var transit_time: float = route["length"] / TRUCK_SPEED
	var time_factor: float = clamp(1.0 - transit_time * TIME_DECAY, TIME_FACTOR_FLOOR, 1.0)
	var pay: float = leg_tiles * float(pieces) * CARGO_RATE * time_factor
	companies[route["owner"]]["cash"] += pay
	if route["owner"] == PLAYER:
		_add_floater(town["pos"], "+$%d" % int(pay))

	town["received"] += float(pieces)
	while town["received"] >= GROWTH_THRESHOLD:
		town["received"] -= GROWTH_THRESHOLD
		town["pop"] += POP_PER_GROWTH
		town["radius"] = min(60.0, town["radius"] + 1.5)


# --- Rival AI ---------------------------------------------------------------
func _ai(delta: float) -> void:
	ai_timer += delta
	if ai_timer < AI_BUILD_INTERVAL:
		return
	ai_timer = 0.0
	for ci in companies.size():
		var c = companies[ci]
		if c["is_player"] or not c["alive"]:
			continue
		# Simple operator AI: if flush, build a new route; otherwise reinforce one.
		if c["cash"] > 1300.0:
			var ind := randi() % industries.size()
			var town := randi() % towns.size()
			_build_route(ci, ind, town)
		elif c["cash"] > 800.0:
			_add_truck_to_a_route(ci)


# --- Building ---------------------------------------------------------------
func _build_route(owner: int, ind_idx: int, town_idx: int) -> bool:
	for r in routes:
		if r["owner"] == owner and r["from"] == ind_idx and r["to"] == town_idx:
			return false
	var length: float = industries[ind_idx]["pos"].distance_to(towns[town_idx]["pos"])
	var cost: float = (length / TILE) * BUILD_COST_PER_TILE + TRUCK_COST
	if companies[owner]["cash"] < cost:
		if owner == PLAYER:
			_add_floater(towns[town_idx]["pos"], "Can't afford ($%d)" % int(cost))
		return false
	companies[owner]["cash"] -= cost
	routes.append({"owner": owner, "from": ind_idx, "to": town_idx, "length": length, "trucks": [_new_truck()]})
	if owner == PLAYER:
		last_route = routes.size() - 1
	return true


func _add_truck_to_a_route(owner: int) -> void:
	for route in routes:
		if route["owner"] == owner:
			companies[owner]["cash"] -= TRUCK_COST
			var t := _new_truck()
			t["progress"] = randf() * 0.5
			route["trucks"].append(t)
			return


func _buy_player_truck() -> void:
	if last_route == -1 or routes[last_route]["owner"] != PLAYER:
		return
	if companies[PLAYER]["cash"] < TRUCK_COST:
		return
	companies[PLAYER]["cash"] -= TRUCK_COST
	var t := _new_truck()
	t["progress"] = randf() * 0.5
	routes[last_route]["trucks"].append(t)


func _new_truck() -> Dictionary:
	return {"phase": "to_industry", "progress": 0.0, "cargo": 0}


# --- Finance: net worth, share price, takeovers -----------------------------
func _net_worth(ci: int) -> float:
	var v: float = companies[ci]["cash"]
	for route in routes:
		if route["owner"] == ci:
			v += route["trucks"].size() * TRUCK_VALUE
	return v


func _share_price(ci: int) -> float:
	return max(0.1, _net_worth(ci) / SHARES_OUTSTANDING)


func _buy_shares(ci: int) -> void:
	var c = companies[ci]
	if not c["alive"]:
		return
	# Creeping acquisition: each slice costs more as your stake grows.
	var nw: float = _net_worth(ci)
	var cost: float = STAKE_STEP * nw * (1.0 + STAKE_PREMIUM_SLOPE * c["stake"])
	if companies[PLAYER]["cash"] < cost:
		_add_floater(_company_anchor(ci), "Need $%d for 5%%" % int(cost))
		return
	companies[PLAYER]["cash"] -= cost
	c["stake"] += STAKE_STEP
	_add_floater(_company_anchor(ci), "Bought 5%% of %s" % c["name"])
	if c["stake"] > CONTROL_THRESHOLD:
		_take_over(ci)


func _take_over(ci: int) -> void:
	# Control achieved: absorb the rival's network and treasury into yours.
	for route in routes:
		if route["owner"] == ci:
			route["owner"] = PLAYER
	companies[PLAYER]["cash"] += companies[ci]["cash"]
	companies[ci]["cash"] = 0.0
	companies[ci]["alive"] = false
	_add_floater(_company_anchor(ci), "TAKEOVER — %s is yours" % companies[ci]["name"])


func _company_anchor(ci: int) -> Vector2:
	return Vector2(get_viewport_rect().size.x - 200, 70 + ci * 96)


# --- Input ------------------------------------------------------------------
func _unhandled_input(event: InputEvent) -> void:
	if event is InputEventKey and event.pressed:
		if event.keycode == KEY_ESCAPE:
			selected_industry = -1
		elif event.keycode == KEY_SPACE:
			paused = not paused
		elif event.keycode == KEY_B:
			_buy_player_truck()
		return
	if event is InputEventMouseButton and event.pressed and event.button_index == MOUSE_BUTTON_LEFT:
		_handle_click(event.position)


func _handle_click(pos: Vector2) -> void:
	# Buy-share buttons take priority over map clicks.
	for b in buy_buttons:
		if (b["rect"] as Rect2).has_point(pos):
			_buy_shares(b["company"])
			return

	if selected_industry == -1:
		var idx := _nearest(industries, pos, 34.0)
		if idx != -1:
			selected_industry = idx
		return
	var t := _nearest(towns, pos, 40.0)
	if t != -1:
		_build_route(PLAYER, selected_industry, t)
	selected_industry = -1


func _nearest(arr: Array, pos: Vector2, max_dist: float) -> int:
	var best := -1
	var best_d := max_dist
	for i in arr.size():
		var d: float = arr[i]["pos"].distance_to(pos)
		if d < best_d:
			best_d = d
			best = i
	return best


# --- Floaters ---------------------------------------------------------------
func _add_floater(pos: Vector2, text: String) -> void:
	floaters.append({"pos": pos + Vector2(0, -30), "text": text, "life": 1.4})


func _update_floaters(delta: float) -> void:
	for f in floaters:
		f["pos"].y -= 24.0 * delta
		f["life"] -= delta
	floaters = floaters.filter(func(f): return f["life"] > 0.0)


# --- Rendering --------------------------------------------------------------
func _draw() -> void:
	_draw_grid()

	for route in routes:
		var col: Color = companies[route["owner"]]["color"]
		var a: Vector2 = industries[route["from"]]["pos"]
		var b: Vector2 = towns[route["to"]]["pos"]
		draw_line(a, b, Color(col.r, col.g, col.b, 0.4), 4.0)
		for truck in route["trucks"]:
			var t: float = truck["progress"]
			if truck["phase"] == "to_industry":
				t = 1.0 - t
			var tp: Vector2 = a.lerp(b, t)
			var c := col if truck["cargo"] > 0 else Color(col.r, col.g, col.b, 0.45)
			draw_rect(Rect2(tp - Vector2(5, 5), Vector2(10, 10)), c)

	for i in industries.size():
		var ind = industries[i]
		draw_rect(Rect2(ind["pos"] - Vector2(16, 16), Vector2(32, 32)), Color(0.55, 0.4, 0.35))
		if i == selected_industry:
			draw_rect(Rect2(ind["pos"] - Vector2(20, 20), Vector2(40, 40)), Color(1, 1, 1), false, 2.0)
		_label(ind["pos"] + Vector2(-16, 30), "stock %d" % int(ind["stock"]), Color(0.2, 0.2, 0.2))

	for town in towns:
		draw_circle(town["pos"], town["radius"], Color(0.45, 0.5, 0.55))
		_label(town["pos"] + Vector2(-20, town["radius"] + 16), "pop %d" % town["pop"], Color(0.2, 0.2, 0.2))

	for f in floaters:
		_label(f["pos"], f["text"], Color(0.1, 0.45, 0.1, clamp(f["life"], 0.0, 1.0)), 16)

	_draw_hud()
	_draw_company_panel()


func _draw_grid() -> void:
	var view := get_viewport_rect().size
	draw_rect(Rect2(Vector2.ZERO, view), Color(0.93, 0.93, 0.9))
	var line := Color(0.88, 0.88, 0.84)
	var x := 0.0
	while x < view.x:
		draw_line(Vector2(x, 0), Vector2(x, view.y), line, 1.0)
		x += TILE
	var y := 0.0
	while y < view.y:
		draw_line(Vector2(0, y), Vector2(view.x, y), line, 1.0)
		y += TILE


func _draw_hud() -> void:
	var view := get_viewport_rect().size
	draw_rect(Rect2(Vector2.ZERO, Vector2(view.x, 36)), Color(0.15, 0.16, 0.2))
	var cash_col := Color(0.6, 1.0, 0.6) if companies[PLAYER]["cash"] >= 0 else Color(1.0, 0.5, 0.5)
	_label(Vector2(14, 24), "$%d" % int(companies[PLAYER]["cash"]), cash_col, 20)
	var hint := "Click industry then town to build  |  B: add truck  |  Space: %s" % ("resume" if paused else "pause")
	if selected_industry != -1:
		hint = "Industry selected — click a town to connect it   (Esc to cancel)"
	_label(Vector2(150, 24), hint, Color(0.85, 0.85, 0.9), 13)


func _draw_company_panel() -> void:
	buy_buttons.clear()
	var view := get_viewport_rect().size
	var px := view.x - 230.0
	draw_rect(Rect2(Vector2(px, 44), Vector2(222, 44 + companies.size() * 96)), Color(0.15, 0.16, 0.2, 0.92))
	for ci in companies.size():
		var c = companies[ci]
		var y := 56.0 + ci * 96.0
		draw_rect(Rect2(Vector2(px + 10, y + 2), Vector2(14, 14)), c["color"])
		_label(Vector2(px + 32, y + 14), c["name"], Color(0.95, 0.95, 1.0), 15)
		if not c["alive"]:
			_label(Vector2(px + 10, y + 38), "ACQUIRED BY YOU", Color(0.6, 1.0, 0.6), 13)
			continue
		var worth_col := Color(0.85, 0.85, 0.9) if c["cash"] >= 0 else Color(1.0, 0.55, 0.55)
		_label(Vector2(px + 10, y + 34), "cash $%d   nw $%d" % [int(c["cash"]), int(_net_worth(ci))], worth_col, 12)
		_label(Vector2(px + 10, y + 50), "share $%.1f" % _share_price(ci), Color(0.85, 0.85, 0.9), 12)
		if not c["is_player"]:
			_label(Vector2(px + 110, y + 50), "your stake %d%%" % int(c["stake"] * 100), Color(0.95, 0.8, 0.4), 12)
			var brect := Rect2(Vector2(px + 10, y + 60), Vector2(200, 22))
			draw_rect(brect, Color(0.25, 0.45, 0.7))
			var buy_cost: float = STAKE_STEP * _net_worth(ci) * (1.0 + STAKE_PREMIUM_SLOPE * c["stake"])
			_label(Vector2(px + 22, y + 76), "BUY 5%%  ($%d)" % int(buy_cost), Color(1, 1, 1), 13)
			buy_buttons.append({"rect": brect, "company": ci})


func _label(pos: Vector2, text: String, color: Color, size: int = 13) -> void:
	if _font == null:
		return
	draw_string(_font, pos, text, HORIZONTAL_ALIGNMENT_LEFT, -1, size, color)
