extends Node2D
## HOSTILE — Operator Loop Prototype
##
## The single question this prototype answers: is it *fun* to connect an
## industry to a town, watch cargo flow, and grow? Everything here is the
## "physics" layer from the systems reference (cargo payment + town growth).
## No share market, no AI, no LBOs yet — just the core loop.
##
## Controls:
##   Left-click an INDUSTRY (square), then a TOWN (circle) to build a route.
##   B  — add another truck to the most recently built route.
##   Esc — cancel a selection.
##   Space — pause / resume.

# --- Tunable constants (tweak these to change the feel) ---------------------
const TILE: float = 32.0
const START_MONEY: int = 3000
const BUILD_COST_PER_TILE: float = 8.0     # cost to lay a route, per tile of distance
const TRUCK_COST: int = 600
const TRUCK_CAPACITY: int = 25
const TRUCK_SPEED: float = 130.0           # pixels / second
const PRODUCTION_RATE: float = 6.0         # cargo units produced / second
const STOCK_CAP: float = 220.0
const CARGO_RATE: float = 1.0              # base pay per (tile * unit)
const TIME_DECAY: float = 0.22             # how fast pay decays with transit time
const TIME_FACTOR_FLOOR: float = 0.2       # minimum fraction of pay for slow routes
const GROWTH_THRESHOLD: float = 60.0       # cargo a town must receive to grow once
const POP_PER_GROWTH: int = 50

# --- World state ------------------------------------------------------------
var money: float = START_MONEY
var paused: bool = false
var industries: Array = []   # { pos:Vector2, stock:float }
var towns: Array = []        # { pos:Vector2, pop:int, received:float, radius:float }
var routes: Array = []       # { from:int, to:int, length:float, trucks:Array }
var selected_industry: int = -1
var last_route: int = -1
var floaters: Array = []      # { pos:Vector2, text:String, life:float }

var _font: Font


func _ready() -> void:
	_font = ThemeDB.fallback_font
	randomize()
	_build_world()


func _build_world() -> void:
	# A few industries and towns scattered across the map. Fixed-ish layout so
	# distances (and therefore route choices) actually matter.
	var ind_spots := [Vector2(180, 180), Vector2(220, 540), Vector2(1050, 250)]
	for p in ind_spots:
		industries.append({"pos": p, "stock": 40.0})

	var town_spots := [
		Vector2(560, 200), Vector2(680, 480), Vector2(950, 560), Vector2(420, 360),
	]
	for p in town_spots:
		towns.append({"pos": p, "pop": 100, "received": 0.0, "radius": 26.0})


# --- Simulation -------------------------------------------------------------
func _process(delta: float) -> void:
	if not paused:
		_simulate(delta)
	_update_floaters(delta)
	queue_redraw()


func _simulate(delta: float) -> void:
	# Industries produce cargo over time, up to a cap.
	for ind in industries:
		ind["stock"] = min(STOCK_CAP, ind["stock"] + PRODUCTION_RATE * delta)

	# Trucks move along their routes, load at the industry, pay out at the town.
	for route in routes:
		var ind = industries[route["from"]]
		var leg: float = route["length"]
		for truck in route["trucks"]:
			var travel: float = TRUCK_SPEED * delta / max(leg, 1.0)
			truck["progress"] += travel
			if truck["progress"] < 1.0:
				continue
			truck["progress"] = 0.0
			if truck["phase"] == "to_town":
				# Arrived at town: deliver cargo and get paid.
				if truck["cargo"] > 0:
					_deliver(route, truck["cargo"])
					truck["cargo"] = 0
				truck["phase"] = "to_industry"
			else:
				# Arrived at industry: load whatever is in stock.
				var load_amt: int = int(min(float(TRUCK_CAPACITY), ind["stock"]))
				truck["cargo"] = load_amt
				ind["stock"] -= load_amt
				truck["phase"] = "to_town"


func _deliver(route: Dictionary, pieces: int) -> void:
	var town = towns[route["to"]]
	var leg_tiles: float = route["length"] / TILE
	var transit_time: float = route["length"] / TRUCK_SPEED
	# Payment shape from the reference: distance x time_decay x pieces x rate.
	# Short routes pay full; long routes decay toward a floor. Sweet spot in the middle.
	var time_factor: float = clamp(1.0 - transit_time * TIME_DECAY, TIME_FACTOR_FLOOR, 1.0)
	var pay: float = leg_tiles * float(pieces) * CARGO_RATE * time_factor
	money += pay
	_add_floater(town["pos"], "+$%d" % int(pay))

	# Town growth: receiving cargo grows the population (and the town's size).
	town["received"] += float(pieces)
	while town["received"] >= GROWTH_THRESHOLD:
		town["received"] -= GROWTH_THRESHOLD
		town["pop"] += POP_PER_GROWTH
		town["radius"] = min(60.0, town["radius"] + 1.5)


# --- Input ------------------------------------------------------------------
func _unhandled_input(event: InputEvent) -> void:
	if event is InputEventKey and event.pressed:
		if event.keycode == KEY_ESCAPE:
			selected_industry = -1
		elif event.keycode == KEY_SPACE:
			paused = not paused
		elif event.keycode == KEY_B:
			_buy_truck_for_last_route()
		return

	if event is InputEventMouseButton and event.pressed and event.button_index == MOUSE_BUTTON_LEFT:
		_handle_click(event.position)


func _handle_click(pos: Vector2) -> void:
	if selected_industry == -1:
		# First click: pick the nearest industry under the cursor.
		var idx := _nearest(industries, pos, 34.0)
		if idx != -1:
			selected_industry = idx
		return

	# Second click: pick a town and build the route.
	var t := _nearest(towns, pos, 40.0)
	if t != -1:
		_build_route(selected_industry, t)
	selected_industry = -1


func _build_route(ind_idx: int, town_idx: int) -> void:
	# Don't allow duplicate routes between the same pair.
	for r in routes:
		if r["from"] == ind_idx and r["to"] == town_idx:
			return
	var length: float = industries[ind_idx]["pos"].distance_to(towns[town_idx]["pos"])
	var cost: float = (length / TILE) * BUILD_COST_PER_TILE + TRUCK_COST
	if money < cost:
		_add_floater(towns[town_idx]["pos"], "Can't afford ($%d)" % int(cost))
		return
	money -= cost
	routes.append({
		"from": ind_idx,
		"to": town_idx,
		"length": length,
		"trucks": [_new_truck()],
	})
	last_route = routes.size() - 1


func _buy_truck_for_last_route() -> void:
	if last_route == -1:
		return
	if money < TRUCK_COST:
		_add_floater(towns[routes[last_route]["to"]]["pos"], "Can't afford truck")
		return
	money -= TRUCK_COST
	# Stagger the new truck slightly so they spread out along the road.
	var truck := _new_truck()
	truck["progress"] = randf() * 0.5
	routes[last_route]["trucks"].append(truck)


func _new_truck() -> Dictionary:
	return {"phase": "to_industry", "progress": 0.0, "cargo": 0}


func _nearest(arr: Array, pos: Vector2, max_dist: float) -> int:
	var best := -1
	var best_d := max_dist
	for i in arr.size():
		var d: float = arr[i]["pos"].distance_to(pos)
		if d < best_d:
			best_d = d
			best = i
	return best


# --- Floating text feedback -------------------------------------------------
func _add_floater(pos: Vector2, text: String) -> void:
	floaters.append({"pos": pos + Vector2(0, -30), "text": text, "life": 1.2})


func _update_floaters(delta: float) -> void:
	for f in floaters:
		f["pos"].y -= 24.0 * delta
		f["life"] -= delta
	floaters = floaters.filter(func(f): return f["life"] > 0.0)


# --- Rendering (simple shapes, no art assets) -------------------------------
func _draw() -> void:
	_draw_grid()

	# Routes (roads) and the trucks on them.
	for route in routes:
		var a: Vector2 = industries[route["from"]]["pos"]
		var b: Vector2 = towns[route["to"]]["pos"]
		draw_line(a, b, Color(0.55, 0.55, 0.6), 5.0)
		for truck in route["trucks"]:
			var t: float = truck["progress"]
			if truck["phase"] == "to_industry":
				t = 1.0 - t
			var tp: Vector2 = a.lerp(b, t)
			var col := Color(0.95, 0.75, 0.2) if truck["cargo"] > 0 else Color(0.7, 0.7, 0.75)
			draw_rect(Rect2(tp - Vector2(5, 5), Vector2(10, 10)), col)

	# Industries (squares).
	for i in industries.size():
		var ind = industries[i]
		var hl: bool = (i == selected_industry)
		draw_rect(Rect2(ind["pos"] - Vector2(16, 16), Vector2(32, 32)), Color(0.85, 0.35, 0.25))
		if hl:
			draw_rect(Rect2(ind["pos"] - Vector2(20, 20), Vector2(40, 40)), Color(1, 1, 1), false, 2.0)
		_label(ind["pos"] + Vector2(-16, 30), "stock %d" % int(ind["stock"]), Color(0.2, 0.2, 0.2))

	# Towns (circles, sized by population).
	for town in towns:
		draw_circle(town["pos"], town["radius"], Color(0.3, 0.55, 0.85))
		_label(town["pos"] + Vector2(-20, town["radius"] + 16), "pop %d" % town["pop"], Color(0.2, 0.2, 0.2))

	# Floating payment text.
	for f in floaters:
		_label(f["pos"], f["text"], Color(0.1, 0.5, 0.1, clamp(f["life"], 0.0, 1.0)), 16)

	_draw_hud()


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
	draw_rect(Rect2(Vector2.ZERO, Vector2(get_viewport_rect().size.x, 36)), Color(0.15, 0.16, 0.2))
	_label(Vector2(14, 24), "$%d" % int(money), Color(0.6, 1.0, 0.6), 20)
	var hint := "Click an industry then a town to build a route   |   B: add truck   |   Space: %s" % ("resume" if paused else "pause")
	if selected_industry != -1:
		hint = "Industry selected — now click a town to connect it   (Esc to cancel)"
	_label(Vector2(150, 24), hint, Color(0.85, 0.85, 0.9), 14)


func _label(pos: Vector2, text: String, color: Color, size: int = 13) -> void:
	if _font == null:
		return
	draw_string(_font, pos, text, HORIZONTAL_ALIGNMENT_LEFT, -1, size, color)
