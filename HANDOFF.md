# Handoff

## What just happened

Big session of polish and features, all committed and pushed to main. Clean git. Here's what was done:

### Features built
- **`SKIP_VITE=1`** — env var in `desktop/xmake.lua` `before_build` that skips the entire Vite build (~2s instead of ~40s). For C++ iteration.
- **`xmake run storybook`** — phony target in `xmake/dev.lua` that runs `bun run storybook` in web/
- **Storybook styles fixed** — added `@tailwindcss/vite` plugin to `.storybook/main.ts` and `@source "../components"` to `shared/styles/globals.css`
- **Storybook theme + font addon** — `.storybook/manager.tsx` panel with searchable 1000+ theme picker, dark/light toggle, 1900+ Google Font picker. Communicates with preview iframe via Storybook channel events. `.storybook/preview.ts` handles the other side.
- **Moved `themes.json` and `google-fonts.json`** from `apps/main/src/data/` to `shared/data/` so Storybook can import them
- **WASM stub proxy** — `wasm-transport.ts` returns no-op proxy for unregistered bridges instead of throwing. Console warns.

### Docs updated
- Both agent + human getting started docs: SKIP_VITE, Storybook section with theme/font panel details
- Both gotchas: build time corrections, SKIP_VITE mention
- Agent tools doc: prefer headless Playwright screenshots over desktop screenshots, ask human before desktop captures, multiple monitor caveat
- Human tools doc: note about agent screenshot behavior

### Files to know
- `NOTES.md` — catalogued observations and future work items
- `.gitignore` — added `screenshot.png`, WASM artifacts in `apps/main/public/`

## What's happening NOW

**Moving the entire template into an `app/` subdirectory.** The goal: make it so this template can live as a subdirectory of a larger project instead of owning the whole repo.

### The plan
1. Create `app/` and move everything into it (except `.github/`)
2. Root `xmake.lua` just does `includes("app/xmake.lua")` as an example
3. Fix all `os.projectdir()` references — there are ~20 hits across 7 Lua files. They need to become relative paths (probably using `os.scriptdir()` or a template-root variable)
4. Get everything building and tests passing from the subdirectory
5. Rename `app/` to `testing123/` to prove directory name doesn't matter

### The `os.projectdir()` audit (from this session)
These files all use `os.projectdir()` and will break:
- `desktop/xmake.lua` — 3 hits (styles path, web dir, binary path output)
- `tests/helpers/dev-server/xmake.lua` — 1 hit (binary path output)
- `xmake/dev.lua` — 7 hits (web dir, pidfile, bat file)
- `xmake/dev-wasm.lua` — 1 hit (root)
- `xmake/scaffold-bridge.lua` — 1 hit (root)
- `xmake/setup.lua` — 1 hit (root)
- `xmake/testing.lua` — 5 hits (base dir)

**Key insight:** `os.projectdir()` returns wherever the root `xmake.lua` is. If the template is in `app/`, and the root `xmake.lua` is at the repo root doing `includes("app/xmake.lua")`, then `os.projectdir()` points to the repo root, not `app/`. Every path breaks.

**Fix approach:** Define a variable like `TEMPLATE_ROOT = os.scriptdir()` in `app/xmake.lua` and use that instead of `os.projectdir()` everywhere internal. Or use `os.scriptdir()` directly in each file with relative navigation.

### Other noted items for future
- Storybook `xmake run` doesn't get killed when terminal exits — consider `start-storybook`/`stop-storybook` pattern like desktop
- Target names might need namespacing if template is a subdirectory (e.g., `setup`, `test-all`, `storybook` are generic)
- See `NOTES.md` for the full list of observations

## Session context
- User is Mrowr Purr (M.P. / Purr)
- They prefer direct, concise communication
- They want to discuss approaches before coding
- They're moving the files themselves right now — the next agent should be ready to fix what breaks
- READ ALL THE DOCS: follow CLAUDE.md → Ethos.md → README.md → all 8 agent docs in `app/docs/DelightfulQtWebShell/for-agents/`
