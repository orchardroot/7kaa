# Seven Kingdoms: Modified

A mod fork of **Seven Kingdoms: Ancient Adversaries** (the 1997 Enlight strategy game, GPL'd in 2009 and lovingly kept alive by [7kfans](https://www.7kfans.com)), based on release 2.15.6 of [the3dfxdude/7kaa](https://github.com/the3dfxdude/7kaa).

I played this game far too much as a teenager and came back to it wanting two things: a defensive building that wasn't a full Fort, and to stop typing a magic cheat string like it's 1997. So this fork adds a **Turret**, a proper **Cheats menu**, and rebuilds the in-game menu with real widgets while it's at it. New work lands on the `mod-base` branch; each larger feature grows on a `feature/*` branch and merges in.

## What's new in this fork

- **Turret** — a new defensive building: 300 gold, 150 HP, hotkey **U** in the build menu, available to every civilisation from the start. It fires arrows automatically at hostile units within 8 tiles: slowly when unmanned, faster as up to 4 soldiers are garrisoned (ranged soldiers count double). Implemented by completing the engine's stubbed firm-fired-projectile path, with its own compact 2×2 keep sprite.
- **Cheats menu** — a "Cheats" entry on the in-game menu (F10, hotkey **C**, single-player only) with the classics: gold, food, all technology and gods, reveal map, town population, repairs, combat levels, immortal king and fast build. The old magic keystring still works if you're sentimental. Using any cheat still marks the score "(Cheated)" — I'm not a monster.
- **Rebuilt in-game menu** — the F10 menu is drawn with real widget buttons instead of a baked bitmap: press animations, reliable hit-testing, translatable labels, and "Quit to OS" instead of "Quit to Windows", because it's 2026 and I'm on a Mac.
- **macOS app bundle** — `packaging/mkbundle-mac.sh` builds a self-contained `Seven Kingdoms.app` (bundled dylibs, icon) and installs it to `/Applications`.

## Compatibility

Save games use version **213**: saves from the unmodified game are refused (and vice versa), because the new building type changes the save layout. Multiplayer with vanilla clients is not compatible. This is a mod; play it with people who also have the mod.

## Building

```sh
./configure CXXFLAGS="-std=gnu++11 -O2" && make -j6
SKDATA=data src/7kaa            # run from the source tree
tools/smoke.sh                  # quick boot test
packaging/mkbundle-mac.sh       # macOS: build and install Seven Kingdoms.app
```

Dependencies and the general autotools story are unchanged from upstream — see the plain-text [`README`](README) (which is what ships in the installers) for the full list: a C++11 compiler, SDL 2.24+, enet 1.3, OpenAL; optionally libcurl, gettext, the 7kaa-music pack, NSIS and TeX Live.

## Hacking on the game data

Game data lives in RES/SET archives under `data/RESOURCE`; the Perl tools in `tools/` (`deresx`/`libresx`, `dbfdump`/`libdb`, `dbf.pm`) unpack and repack them. Every data change is a scripted, replayable transform, so nothing in `data/` is a mystery binary edit:

- `tools/add_turret_data.pl` — the Turret's FIRM/FBUILD records and icon
- `tools/make_turret_icon.py` — repaints the Fort icon's label to TURRET
- `tools/patch_gamemenu_os.py` — "Quit to OS" on the old menu bitmap (kept for reference; the rebuilt menu no longer uses it)

Many source files are ISO-8859 encoded — use `grep -a`, and keep that encoding when editing, or git will show you a diff of the entire file and you'll have a bad afternoon.

## Roadmap

- Teach the AI to build and garrison turrets
- Dedicated turret artwork in every civilisation's style, and an encyclopedia entry
- More menu quality-of-life (the main menu is still bitmap-based)

## Credits and licence

Seven Kingdoms: Ancient Adversaries is © Enlight Software, released under the GPL in 2009 — see [`COPYING`](COPYING). Enormous thanks to Enlight for that, and to the 7kfans project and [the3dfxdude](https://github.com/the3dfxdude/7kaa) for fifteen-plus years of stewardship; go there for official releases, and to [7kfans.com](https://www.7kfans.com) for the community. Everything in this fork is under the same licence.

---

*orchardroot — made in Cheshire, under the supervision of two cats, who would make terrible kings.*
