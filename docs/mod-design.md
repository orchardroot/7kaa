# 7kaa Mod Design: Cheats Menu + Turret

Two features on top of upstream 2.15.6. Single-player focus (this build has
multiplayer disabled; all new code is nonetheless gated on `!remote.is_enable()`).

## 1. Cheats menu

Goal: cheats reachable from the in-game menu instead of only via the `!!!@@@###`
keystring and letter keys.

- Add a ninth "Cheats" row to `InGameMenu` (`src/OINGMENU.cpp`). The existing
  eight rows are baked into the `GAMEMENU` bitmap, so the new row is drawn as
  text (same technique as the Map I.D. line at OINGMENU.cpp:159). Hotkey `C`.
  Hidden/disabled in multiplayer and observer mode.
- Clicking it opens `CheatMenu`, a new modal panel modelled on `OptionMenu`
  (`src/OOPTMENU.cpp`): plain widget buttons, no new bitmap resources.
- Buttons and the existing implementations they call:
  - +1000 Gold — `NationBase::add_cheat(1000)`
  - +1000 Food — `food += 1000` (as OSYS.cpp:1820)
  - All Technology & Gods — `tech_res.inc_all_tech_level()` + `god_res.enable_know_all()`
  - Reveal Map — `world.unveil()` + `world.visit()`
  - +10 Town Population (needs selected own town) — as OSYS.cpp:1838
  - Repair Building (needs selected own firm) — `hit_points = max_hit_points`
  - +20 Combat (needs selected own unit) — `set_combat_level()`
  - Immortal King toggle — `config.king_undie_flag`
  - Fast Build toggle — `config.fast_build`
- Selection-dependent buttons grey out when the selection is missing/foreign.
- Any use sets `nation->cheat_enabled_flag = 1`, so score screens still report
  "(Cheated)". The keystring path is untouched.

## 2. Turret (new defensive firm)

Goal: a buildable defensive structure that fires arrows like a Norman archer;
weak autonomous fire, boosted by garrisoned soldiers (hybrid model).

- New `FIRM_TURRET = 11` (`include/OFIRMID.h`, `MAX_FIRM_TYPE` 10 → 11; enum
  appended last because firm ids are FIRM.DBF row order).
- New class `FirmTurret : Firm` (`include/OF_TURR.h`, `src/OF_TURR.cpp`).
  - Garrison: `need_worker`, up to 4 soldiers (standard worker array).
  - Targeting: every few frames scan locations within range 8 for the nearest
    hostile mobile unit (deterministic scan order).
  - Firing: complete the engine's stubbed firm-bullet path —
    `BulletArray::add_bullet(Firm*, Unit*)` (OBULLETA.cpp:460 stub) and a
    `BULLET_BY_FIRM` branch in `Bullet::init` (relax the `err_when` at
    OBULLET.cpp:70). Projectile sprite/speed/damage copied from the Norman
    archer's `AttackInfo` row.
  - Hybrid rate: unmanned ≈ one shot / 3s at ~60% archer damage; each
    garrisoned soldier shortens the delay, ranged-skilled soldiers count
    double. Cap ≈ one shot per second.
- Data records appended to the DBFs inside `data/RESOURCE/STD.SET` (FIRM,
  FBUILD, FFRAME, FBITMAP + destroyed-animation DBFs) using the perl tools in
  `tools/`. Sprite: reuse an existing 2x2-ish building bitmap; build-menu icon
  `F-11` in `I_BUTTON.RES` cloned from the fort icon. Stats: ~300 gold,
  ~150 HP, no tech prerequisite, buildable by construction units of all races
  (`all_know`, `firm_race_id` 0).
- Touch points: `FirmArray::create_firm`/`firm_class_size` (OFIRMA.cpp),
  build order + hotkey + "where to build" tables (OUNITIF.cpp), the fourteen
  `[MAX_FIRM_TYPE]` news string tables (ONEWSENG.cpp), `FirmTurretCrc`
  (OMP_CRC.h/.cpp), Makefile.am lists.
- `GAME_VERSION` (AM.cpp) bumped 212 → 213: old saves refuse cleanly (the new
  firm type changes `FirmRes`/save layout).
- Out of scope for v1: AI building/garrisoning turrets, new artwork,
  encyclopedia entry.

## Verification

No test suite exists upstream. Each stage is verified by a clean build plus a
scripted smoke launch, then manual in-game checks: cheats panel operates each
button in a live game; a turret is built, fires unmanned, fires faster
garrisoned, kills an attacker, survives save/load.
