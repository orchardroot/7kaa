# Cheats Menu + Turret Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add an in-game Cheats menu and a new defensive "Turret" building that fires arrow projectiles, per `docs/mod-design.md`.

**Architecture:** Feature 1 adds a ninth text-drawn row to `InGameMenu` opening a new modal `CheatMenu` panel that calls the game's existing cheat functions. Feature 2 appends firm type 11 (`FIRM_TURRET`) end-to-end: enum, class, factory, data records inside `STD.SET`, build menu, and completion of the engine's stubbed firm-fired-bullet path.

**Tech Stack:** C++11 (clang on macOS 12 x86_64), autotools. Build: `./configure CXXFLAGS="-std=gnu++11 -O2" && make -j6`. Run: `SKDATA=data src/7kaa`. Data tools: perl scripts in `tools/` (`dbfdump`, `libdb`/`delibdb`, `deresx`/`libresx`, `icnpack`).

## Global Constraints

- Repo: `~/Projects/7kaa`, branch base `mod-base`. Feature branches `feature/cheats-menu` and `feature/turret`, merged back into `mod-base` with `--no-ff`; push to `origin` (orchardroot fork) after each task.
- Many `.cpp` files are ISO-8859 encoded — always `grep -a`; do not re-encode files.
- Every new gameplay-affecting code path must be guarded by `!remote.is_enable()` (matches all existing cheat handlers, e.g. `src/OSYS.cpp:1801`).
- Any cheat applied must set `~nation_array`'s player nation `cheat_enabled_flag = 1` (same flag the keystring sets at `src/OSYS.cpp:1540-1547`).
- `GAME_VERSION` in `src/AM.cpp:119` is bumped 212 → 213 exactly once, in Task 6 (first save-format-breaking change).
- No test suite exists upstream. Each task's test cycle = clean `make` + scripted smoke launch (below) + the task's listed manual in-game check.
- Smoke launch script (create once in Task 1): `tools/smoke.sh` — launches the game for 8 s and fails on early exit or crash.
- Match surrounding code style: tabs, 90s-style comments sparse, class naming `FirmTurret`/file naming `OF_TURR.*`, `OCHTMNU.*`.

---

### Task 1: Smoke-test script + CheatMenu skeleton (opens/closes, no effects yet)

**Files:**
- Create: `tools/smoke.sh`
- Create: `include/OCHTMNU.h`
- Create: `src/OCHTMNU.cpp`
- Modify: `src/Makefile.am` (add `OCHTMNU.cpp` to `7kaa_SOURCES` list, alphabetical near `OCONFIG.cpp`), `include/Makefile.am` (add `OCHTMNU.h`)

**Interfaces:**
- Produces: global `CheatMenu cheat_menu;` with `void enter()` (modal; returns when closed), following the modal-loop pattern of `InGameMenu::enter` at `src/OINGMENU.cpp:71-120`. Task 2 calls `cheat_menu.enter()`.

- [ ] **Step 1: Write `tools/smoke.sh`**

```sh
#!/bin/sh
# Launch the game briefly to prove it boots. Usage: tools/smoke.sh
cd "$(dirname "$0")/.." || exit 1
SKDATA=data src/7kaa & pid=$!
sleep 8
if kill -0 "$pid" 2>/dev/null; then kill "$pid"; echo "SMOKE OK"; exit 0
else echo "SMOKE FAIL: game exited early"; wait "$pid"; exit 1; fi
```
`chmod +x tools/smoke.sh`.

- [ ] **Step 2: Write the CheatMenu class**

`include/OCHTMNU.h`: class `CheatMenu` with `char active_flag;`, `void enter();`, `void disp();`, `int detect();`, `void abort();` — mirror the shape of `include/OINGMENU.h:31-47`. Panel geometry: centered box ~400×340 in the zoom area (copy the centering arithmetic from `src/OINGMENU.cpp:46-61`). Declare `extern CheatMenu cheat_menu;`.

`src/OCHTMNU.cpp`: implement `enter()` as a modal loop cloned from `InGameMenu::enter` (`src/OINGMENU.cpp:71-120`): save the screen area, loop `sys.yield(); mouse.get_event();` dispatch `detect()`, restore screen on exit. `disp()`: `vga_util.d3_panel_up(...)` for the panel, title "CHEATS" via `font_san.center_put(...)` (copy exact font/panel calls from `OptionMenu::disp` in `src/OOPTMENU.cpp:207` area — use whatever font object that file uses for section headers). Draw 10 rows of left-aligned text (9 cheat rows from the design doc + "Done"), each row an entry in a static `struct { const char* label; }` table; record per-row hit rects. `detect()`: hit-test rows, return row index or 0; row "Done" (and `KEY_ESC`) exits the loop. For this task every cheat row is a no-op.

- [ ] **Step 3: Build**

Run: `make -j6 2>&1 | tail -5`
Expected: links clean, no warnings introduced.

- [ ] **Step 4: Smoke test**

Run: `tools/smoke.sh`
Expected: `SMOKE OK`.

- [ ] **Step 5: Commit on new branch**

```bash
git checkout -b feature/cheats-menu mod-base
git add tools/smoke.sh include/OCHTMNU.h src/OCHTMNU.cpp src/Makefile.am include/Makefile.am
git commit -m "feat: CheatMenu modal panel skeleton + smoke script"
```

---

### Task 2: Ninth "Cheats" row in InGameMenu

**Files:**
- Modify: `include/OINGMENU.h:31-37` (`GAME_OPTION_COUNT` 8 → 9, `menu_hot_key[9]`)
- Modify: `src/OINGMENU.cpp` (hotkeys `:63`, enable/disable in `enter()` `:81-94`, draw in `disp()` `:128-169`, dispatch in `detect()` `:216-290`)

**Interfaces:**
- Consumes: `cheat_menu.enter()` from Task 1.

- [ ] **Step 1: Implement the row**

- `GAME_OPTION_COUNT` → 9. Append hotkey `'c'` before `KEY_ESC` stays on the *last* slot: new table `{'o','s','l',0,0,0,0,'c',KEY_ESC}` — the "continue/ESC" behavior must remain on the final index; renumber the `switch(i)` in `detect()` accordingly (old case 8 "continue" becomes case 9; new case 8 = cheats).
- In `enter()`: disable the cheats row (`game_menu_option_flag[7]=0` for 0-based index of the new row) when `remote.is_enable()` — same pattern as the observer/multiplayer disables at `:81-94`.
- In `disp()`: the `GAMEMENU` bitmap only has 8 painted rows. Draw the ninth as text at `GAME_OPTION_X1, GAME_OPTION_Y1 + 8*GAME_OPTION_HEIGHT` using the same font call pattern as the Map I.D. line at `:159-165` (shift the Map I.D. line down one row height so they don't collide; verify it still fits the 400-px panel — if not, reduce the gap between the Map I.D. line and the last row rather than moving buttons).
- In `detect()`: new case calls `cheat_menu.enter();` then redraws the in-game menu (mirror how case 1 re-enters after `option_menu.enter`, `:216-230`).

- [ ] **Step 2: Build + smoke**

Run: `make -j6 && tools/smoke.sh`
Expected: `SMOKE OK`.

- [ ] **Step 3: Manual check**

Launch `SKDATA=data src/7kaa`, start any single-player game, press F10: a "Cheats" text row appears under the eight bitmap rows; clicking it opens the panel; ESC/Done closes it; game resumes.

- [ ] **Step 4: Commit**

```bash
git add include/OINGMENU.h src/OINGMENU.cpp
git commit -m "feat: add Cheats row to in-game menu"
```

---

### Task 3: Wire the nine cheat effects

**Files:**
- Modify: `src/OCHTMNU.cpp`

**Interfaces:**
- Consumes: existing implementations, exact call sites to copy: money/food `src/OSYS.cpp:1816-1823`; tech+gods `:1825-1830`; unveil `:1832-1836`; town pop `:1838-1857`; firm repair `:1880-1887`; combat `:1900-1907`; toggles `:1859-1866` (`config.king_undie_flag`) and `:1889-1896` (`config.fast_build`). Cheat-mode flag: `(~nation_array)->cheat_enabled_flag = 1` as at `:1540-1547`.

- [ ] **Step 1: Implement effects**

In `detect()`, replace the no-ops. Each handler: (a) `if(remote.is_enable()) return;` (b) apply the effect by copying the referenced OSYS.cpp block verbatim (including its selection guards, e.g. town cheat requires `town_array.selected_recno` and own nation); (c) set `cheat_enabled_flag=1`. For the two toggles, append the current state to the row label (`"Immortal King: ON"`) — rebuild the label with `snprintf` into a `static char` buffer each `disp()`. For selection-dependent rows (town pop, repair, combat), when the selection guard fails, draw the row in the disabled color: use the same `adjust_brightness`/dim-text technique `InGameMenu::disp` uses at `src/OINGMENU.cpp:147-157`, and make `detect()` ignore clicks on them.

- [ ] **Step 2: Build + smoke**

Run: `make -j6 && tools/smoke.sh` → `SMOKE OK`.

- [ ] **Step 3: Manual check (full pass)**

In a live game verify each button: gold +1000 (nation report), food +1000, all-tech (war factory list full), reveal map, +10 pop with a town selected and greyed without, repair on a damaged own firm, +20 combat on own unit, both toggles flip and persist, score screen after retiring says "(Cheated)".

- [ ] **Step 4: Commit, merge, push**

```bash
git add src/OCHTMNU.cpp
git commit -m "feat: wire cheat effects into CheatMenu"
git checkout mod-base && git merge --no-ff feature/cheats-menu -m "Merge cheats menu"
git push origin mod-base feature/cheats-menu
```

---

### Task 4: Firm-fired bullets (engine path only, no turret yet)

**Files:**
- Modify: `src/OBULLETA.cpp:460-476` (implement the two stubbed overloads — `add_bullet(Firm*, Unit*)` is required; leave `add_bullet(Firm*, Firm*)` stubbed but `err_here()` so misuse is loud)
- Modify: `src/OBULLET.cpp:62-121` (`Bullet::init` firm branch; remove/loosen `err_when(parent_type!=BULLET_BY_UNIT)` at `:70`)
- Modify: `src/OB_HOMIN.cpp:66` (keep its `err_when` but make it tolerate `BULLET_BY_FIRM` only if reached; homing bullets stay unit-only — assert message unchanged)
- Modify: `include/OBULLET.h` if `init` needs a new parameter (see step 1)

**Interfaces:**
- Produces: `int BulletArray::add_bullet(Firm* parentFirm, Unit* targetUnit, AttackInfo* attackInfo)` — fires one projectile from the firm's center loc to the target's current loc using the given `AttackInfo` (range/damage/speed/sprite fields per `include/OUNITRES.h:310-332`). Returns bullet recno or 0. Task 7 calls this.
- Consumes: existing `Bullet` motion/damage code (`process_move` `src/OBULLET.cpp:127`, `check_hit`, `attenuated_damage`) which the explorer confirmed is parent-agnostic.

- [ ] **Step 1: Implement**

Study the unit-parent flow first: `BulletArray::add_bullet(Unit*, Unit*)` and `Bullet::init` (`src/OBULLET.cpp:62-121`) — note every field `init` reads from `parentUnit` (`attack_info_array[cur_attack]`, `attack_dir`, `nation_recno`, sprite pos). Then:
- Add the third parameter `AttackInfo* attackInfo` to a new overload signature in `include/OBULLET.h:104-107` (replacing the stubbed 2-arg `(Firm*, Unit*)` — no other caller exists, verified by the stub returning 0).
- In `Bullet::init`, branch on `parent_type`: for `BULLET_BY_FIRM`, source position = firm center (`firm->center_x, firm->center_y` converted to the same coordinate space the unit branch uses — copy the conversion the unit branch applies to `parentUnit->cur_x/cur_y`), `nation_recno = firm->nation_recno`, direction computed from source→target delta with the same `misc.get_dir(...)` helper the unit path uses (grep -a for `get_dir` in `src/OBULLET.cpp`/`src/OUNITAT2.cpp`), and all ballistic fields (`attack_damage`, `bullet_speed`, `sprite_id`, `fire_radius`) from the passed `attackInfo`.
- Determinism: no `misc.random()` in this path.

- [ ] **Step 2: Build + smoke**

Run: `make -j6 && tools/smoke.sh` → `SMOKE OK` (path is dead code until Task 7 — this task only proves it compiles and nothing regresses).

- [ ] **Step 3: Commit**

```bash
git checkout -b feature/turret mod-base
git add include/OBULLET.h src/OBULLETA.cpp src/OBULLET.cpp src/OB_HOMIN.cpp
git commit -m "feat: implement engine's stubbed firm-fired bullet path"
```

---

### Task 5: FIRM_TURRET type + FirmTurret class (registered, inert)

**Files:**
- Modify: `include/OFIRMID.h:29,43` (`MAX_FIRM_TYPE` 11, append `FIRM_TURRET=11`)
- Create: `include/OF_TURR.h`, `src/OF_TURR.cpp`
- Modify: `include/OFIRMALL.h` (include OF_TURR.h), `src/OFIRMA.cpp:187-293` (`create_firm` + `firm_class_size` cases), `src/Makefile.am`, `include/Makefile.am`
- Modify: `src/ONEWSENG.cpp` — every `[MAX_FIRM_TYPE]` string table gains an 11th `"Turret"`/appropriate entry (explorer lists lines `478, 492, 506, 520, 574, 588, 880, 894, 976, 1273, 1287, 1301, 1315, 1612`)
- Modify: `include/OMP_CRC.h` + `src/OMP_CRC.cpp` (`FirmTurretCrc` struct + `crc8/init_crc/clear_ptr`, cloned from the simplest existing pair — `FirmResearchCrc` at `include/OMP_CRC.h:237`)
- Modify: `src/OUNITIF.cpp:96-124,1006` (append `FIRM_TURRET` to `build_firm_button_order`, hotkey table, `select_where_to_build` string "Select a location for the Turret.")

**Interfaces:**
- Produces: `class FirmTurret : public Firm` in `OF_TURR.h` with members `short fire_delay_count; short target_unit_recno;` and overrides `next_day()`, `process_animation()`, `being_attacked()`, `write_derived_file/read_derived_file` inherited defaults (raw blob covers the two shorts), `crc8/init_crc/clear_ptr`. Constructor sets `firm_skill_id = SKILL_LEADING` (so soldiers garrison like a camp). This task: methods are empty/default — building exists but does nothing.
- Consumes: nothing new.

- [ ] **Step 1: Implement all touch points listed above**

`create_firm` case: `case FIRM_TURRET: firmPtr = new FirmTurret; break;` — clone the syntax of the `FIRM_MONSTER` case at `src/OFIRMA.cpp:187-244`. `firm_class_size`: `sizeof(FirmTurret)`. Note `#pragma pack(1)` in `include/OFIRM.h:148` — keep it for `OF_TURR.h` (match every other `OF_*.h` header's pragma usage exactly).

- [ ] **Step 2: Build**

Run: `make -j6` — expect clean. The game must not yet be launched into a real map with a turret (no data records exist), but boot is unaffected.

- [ ] **Step 3: Smoke + commit**

Run `tools/smoke.sh` → `SMOKE OK`.
```bash
git add include/OFIRMID.h include/OF_TURR.h src/OF_TURR.cpp include/OFIRMALL.h src/OFIRMA.cpp src/ONEWSENG.cpp include/OMP_CRC.h src/OMP_CRC.cpp src/OUNITIF.cpp src/Makefile.am include/Makefile.am
git commit -m "feat: register FIRM_TURRET firm type (inert)"
```

---

### Task 6: Turret data records in STD.SET + icon + save version bump

**Files:**
- Modify: `data/RESOURCE/STD.SET` (append DBF records: FIRM, FBUILD, FFRAME, FBITMAP; and FDBUILD/FDFRAME/FDBITMAP in `I_FIRMDI.RES`'s DB set only if required by loader — check `src/OFIRMDIE.cpp:34-36` handling of missing records first)
- Modify: `data/RESOURCE/I_BUTTON.RES` (add `F-11` icon, cloned from `F-5` fort icon)
- Create: `tools/add_turret_data.pl` (idempotent script that performs the above from a pristine STD.SET, so the transform is reviewable and re-runnable)
- Modify: `src/AM.cpp:119` (`GAME_VERSION` 212 → 213)

**Interfaces:**
- Consumes: record layouts `FirmRec`/`FirmBuildRec`/`FirmFrameRec`/`FirmBitmapRec` at `include/OFIRMRES.h:55-146` (DBF field widths must match byte-for-byte); loader behavior `src/OFIRMRES.cpp:292-359` (`buildable = setup_cost > 0`, `all_know=='1'` grants to all nations, `loc_width/height` from first build record).
- Produces: firm id 11 loadable: name "Turret", `setup_cost` 300, `hit_points` 150 (field name per FirmRec — confirm with `tools/dbfdump` on the FIRM db), `all_know='1'`, `firm_race_id=0`, footprint and bitmaps cloned from an existing 2×2 building (pick the best-looking candidate after dumping FBUILD — the mine or market; decision recorded in the script comments).

- [ ] **Step 1: Dump and study**

```bash
cd ~/Projects/7kaa
perl tools/deresx ...   # per tools' own usage text: unpack STD.SET dbs
perl tools/dbfdump ...  # dump FIRM, FBUILD, FBITMAP; note exact field names/widths
```
(The tools are self-documenting perl; read their headers first. Work in `scratch/` — add `scratch/` to `.gitignore`.)

- [ ] **Step 2: Write `tools/add_turret_data.pl`**

Script clones the chosen donor building's FBUILD/FFRAME/FBITMAP rows under firm id 11 (bitmap records can point at the *same* sprite offsets in `I_FIRM.RES` — no new sprite data needed), appends the FIRM record with the stats above, repacks STD.SET, and appends an `F-11` entry to `I_BUTTON.RES` duplicating `F-5`'s bitmap bytes. Verify repack byte-sizes with the tools' own list mode before overwriting; keep a `data/RESOURCE/STD.SET.orig` backup **outside** git.

- [ ] **Step 3: Build + smoke + manual check**

`make -j6 && tools/smoke.sh`, then launch: start a game, select a construction worker → Build menu shows the Turret with the fort-style icon; place it; it constructs and stands there inert; F-11 no crash; save then load the game (new saves work at version 213; a pre-change save is refused with the version message, not a crash).

- [ ] **Step 4: Commit**

```bash
git add tools/add_turret_data.pl data/RESOURCE/STD.SET data/RESOURCE/I_BUTTON.RES src/AM.cpp .gitignore
git commit -m "feat: turret data records, build icon, save version 213"
```

---

### Task 7: Turret firing logic (hybrid garrison model)

**Files:**
- Modify: `src/OF_TURR.cpp`, `include/OF_TURR.h`

**Interfaces:**
- Consumes: `bullet_array.add_bullet(Firm*, Unit*, AttackInfo*)` from Task 4; `Worker::max_attack_range()` (`src/OFIRM.cpp:3721-3737`); `AttackInfo` source: at `init()`, scan `unit_res` for the Norman soldier's ranged `AttackInfo` (race Norman, `attack_range > 1`); fallback = first any-unit `AttackInfo` with `attack_range >= 6`; store the pointer in a **non-serialized** static/looked-up-each-use variable (pointers must not live in the save blob — look it up in `init()` and after `read_derived_file` via an overridden `read_derived_file` that re-resolves it, or simplest: re-resolve lazily each shot).
- Produces: observable behavior — see manual check.

- [ ] **Step 1: Implement targeting + firing**

In `process_animation()` (called every frame per `src/OFIRMA.cpp:447`):
```cpp
// pseudocode shape; follow real member/API names from OFIRM.h and OWORLD.h
if( --fire_delay_count > 0 ) return;
Unit* target = find_target();          // nearest hostile mobile unit within TURRET_RANGE=8
if( !target ) { fire_delay_count = TURRET_SCAN_DELAY; return; }   // rescan soon (10 frames)
bullet_array.add_bullet(this, target, turret_attack_info());
fire_delay_count = current_fire_delay();
```
- `find_target()`: iterate `unit_array` linearly (recno order — deterministic), skip dead/own/friendly (`nation_recno` diplomacy check via `nation_array`—copy the hostility test used by `FirmCamp::defense` around `src/OF_CAMP.cpp:1040-1160`), require air/land unit on same region reachable by straight shot (`bullet_array.add_bullet_possible`, `src/OBULLETA.cpp:188-338`), require `misc.points_distance(center_x, center_y, unitX, unitY) <= TURRET_RANGE`; keep nearest.
- `current_fire_delay()`: base `90` frames (~3 s at 30 fps); each garrisoned worker −15, ranged workers (`worker_array[i].max_attack_range() > 1`) −30; floor `30` frames (~1 s).
- Damage: unmanned scale `attack_damage * 3 / 5` — pass a stack-copied `AttackInfo` with scaled damage when `worker_count == 0`.
- All constants `enum`s at top of `OF_TURR.cpp`.

- [ ] **Step 2: Build + smoke**

`make -j6 && tools/smoke.sh` → `SMOKE OK`.

- [ ] **Step 3: Manual check**

Start a game next to a hostile town/lair (or use the new Cheats menu: reveal map, fast build). Build a turret in enemy path: it fires arrows at approaching enemies unmanned (slow), visibly faster with 4 soldiers garrisoned; arrows draw, fly, and kill; turret stops firing when target dies/leaves range; save/load mid-fight and behavior resumes sanely.

- [ ] **Step 4: Commit**

```bash
git add src/OF_TURR.cpp include/OF_TURR.h
git commit -m "feat: turret targeting and hybrid-garrison firing"
```

---

### Task 8: Finish line — merge, push, docs

**Files:**
- Modify: `README` (short "Fork modifications" section at top listing the two features and save-version bump)

- [ ] **Step 1: Full rebuild from clean + smoke**

Run: `make clean && make -j6 && tools/smoke.sh` → `SMOKE OK`.

- [ ] **Step 2: Final manual regression pass**

One 15-minute play session touching: normal build menu (all 10 vanilla firms still buildable and drawn correctly — off-by-one check for the new array slots), news messages still correct, cheats menu, turret combat, save+load, retire → score screen.

- [ ] **Step 3: Merge and push**

```bash
git add README && git commit -m "docs: describe fork modifications"
git checkout mod-base && git merge --no-ff feature/turret -m "Merge turret"
git push origin mod-base feature/cheats-menu feature/turret
```
