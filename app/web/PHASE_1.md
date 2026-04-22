# 🏴‍☠️ Phase 1 Kickoff — Install shadcn Catalog + Activate 31-Var Theme Vocabulary

You're the agent picking up Phase 1 of a multi-phase cleanup. This doc gives you everything you need to land in cold. Read it start to finish before running a single command.

---

## Who's who

- **You** — the agent executing Phase 1.
- **Purr** (a.k.a. Mrowr Purr, M.P.) — the human you report to. Lead on all decisions. Direct, experienced, prefers terse responses with proof. Never call her "the user." Read `~/.claude/skills/working-with-purr/SKILL.md` (or the equivalent working-with-purr skill) if you have it. Match her energy. Use emoji.
- **The coordinator agent** (that's me, an Opus 4.7 instance on the same team) — I'm tracking the plan across phases. I won't be executing Phase 1 code with you. Your interface to me is the git history and `NOTES.md` — update it, push, and I'll see it.

---

## Why this work exists

This project is `delightful-qt-web-shell` — a Qt + React template Purr built to combine her favorite patterns for the Qt web-shell apps she regularly ships. Desktop (Qt) + browser (WASM) from one codebase, C++ bridges with def_type DTOs, five test layers. The C++/bridge side is elegant and working. **The web side drifted.**

Specifically:

1. **Hand-rolled drift.** The template was supposed to use shadcn/ui for everything. Previous agents rolled a 130-line custom Combobox here, a custom Tabs there, a custom Toggle duplicated across two files, bare `<input>` elements everywhere. Each new agent looked at the existing code, saw custom primitives, and concluded "custom is the standard here." Loop reinforced itself.
2. **Dead theme vocabulary.** `web/shared/data/themes.json` ships 1030 themes, each with 31 CSS variables (surface, action, support, chart, sidebar). Only 19 of those 31 vars are consumed anywhere. The chart vars are partially used by Monaco (4 of 5); the 8 sidebar vars are 100% dead. Not because the themes were authored wrong — because the components that consume `--sidebar-*` and `--chart-*` were never installed.

Both problems are the same problem: **missing shadcn components = dead theme vars.** Phase 1 is the foundation that enables every other phase.

---

## Read these before you touch code

1. **`app/Ethos.md`** — the team principles. Non-negotiable. Especially: "Run the tests before you write a single line," "Own every failure," "Fix root causes not symptoms," "Never use destructive git commands."
2. **`app/CLAUDE.md`** — points at the for-agents docs.
3. **`app/docs/DelightfulQtWebShell/for-agents/01-getting-started.md` through `08-theming.md`** — the project-wide guide. Architecture, bridges, testing, tools, gotchas, theming.
4. **`app/web/COMPONENT_AUDIT.md`** — every .tsx, what's hand-rolled, what's shadcn, what's broken. I wrote this during the planning.
5. **`app/web/THEME_AUDIT.md`** — how themes are authored, what each consumer uses, where the 12-var gap comes from.
6. **`app/web/NOTES.md`** — locked decisions, open questions, TODO buckets, phase overview. This is the live working doc for the whole effort.

Don't skip the Ethos. It's short. It's how we work.

---

## The 6 locked decisions (don't relitigate)

Verbatim from `NOTES.md`:

1. **shadcn-first is the rule.** Every primitive comes from `@shared/components/ui/` — the files there are shadcn-CLI-generated, rendering Radix (or cmdk) primitives with Tailwind classes baked in. They *are* the components, not wrappers around them.
2. **Left-side `Sidebar` replaces the top `TabsList`** as the main app chrome.
3. **Every one of the 31 theme vars in `themes.json` should be consumed somewhere real.**
4. **The root `shared/components/ui/` IS the reusability signal.** Holds the full shadcn catalog; feature code imports from there.
5. **Typed bridge helpers (`getSystemBridge()`), never `getBridge<T>('name')` in feature code.** Magic strings are banned.
6. **Install the full shadcn catalog (~50 components), not just what the demo uses right now.** Tree-shaking = zero runtime cost. Template ships batteries included.
7. **One in-app "Components" page showing every installed primitive on a single scroll** (sidebar item).

---

## The 5 phases at a glance, so you know where you are

| # | Phase | Visible change? |
|---|-------|-----------------|
| **1** | **Install & theme activation — YOU ARE HERE** | No |
| 2 | Shell restructure (Sidebar replaces top tabs) | Yes, big |
| 3 | Primitive swaps + bridge helpers | Yes |
| 4 | Chart demo + vocabulary completion | Small |
| 5 | Leak cleanup + agent docs | No |

---

## Phase 1 — What you're doing

**Two deliverables. No visible UI change.**

### Deliverable 1 — Install the full shadcn catalog

Current state: `shared/components/ui/` has 4 files (`button.tsx`, `card.tsx`, `select.tsx`, `tabs.tsx`). After Phase 1, ~50 files. Every shadcn primitive as of the current catalog, installed.

### Deliverable 2 — Extend `applyTheme()` to emit all 31 vars

Current state: `web/shared/lib/themes.ts` has an `ALL_VARS` constant with 19 names. It writes `--foo` and `--color-foo` to a `<style>` tag for each. `applyTheme()` never emits `--chart-*` or `--sidebar-*`. After Phase 1, all 31 vars reach `:root` on every theme switch.

---

## Phase 1 — Execution playbook

### Step 0 — Establish a baseline (Ethos: "Run the tests before you write a single line")

```bash
# Non-invasive tests first — these don't take over the desktop
xmake run test-todo-store
xmake run test-bun
xmake run test-browser
```

Record what passes. If anything is already red, **tell Purr before proceeding** — don't adopt someone else's broken state.

**Do NOT** run `xmake run test-all` or `test-pywinauto` or `test-desktop` without asking Purr first — those take over her desktop for ~30 seconds.

Also:
```bash
# Visual baseline — take a headless screenshot of the running app
# See 05-tools.md for the pattern
```

### Step 1 — Pre-flight checks

1. `cat web/components.json` (if it exists) — shadcn's config. Verify aliases point at `@shared/*` not the default `@/*`. If the file doesn't exist, shadcn hasn't been init'd — but the 4 existing files imply it has. If anything is confusing, **stop and ask Purr**.
2. Check `package.json` for `@radix-ui/*` peer deps already present. Fine if they are; shadcn components will reuse them.
3. Check that `web/shared/styles/theme.css` still has its `@theme { ... }` block with the 19 `--color-*` vars. That block is load-bearing for the current app.

### Step 2 — Install the full catalog

Full shadcn catalog as of now (~50 components). Install each via the CLI. Either batch them (check if `npx shadcn@latest add --all` works) or script a loop. Names:

> `accordion`, `alert-dialog`, `alert`, `aspect-ratio`, `avatar`, `badge`, `breadcrumb`, `button`, `calendar`, `card`, `carousel`, `chart`, `checkbox`, `collapsible`, `combobox` (may be a pattern, not a file — verify), `command`, `context-menu`, `data-table`, `date-picker`, `dialog`, `drawer`, `dropdown-menu`, `form`, `hover-card`, `input-otp`, `input`, `label`, `menubar`, `navigation-menu`, `pagination`, `popover`, `progress`, `radio-group`, `resizable`, `scroll-area`, `select`, `separator`, `sheet`, `sidebar`, `skeleton`, `slider`, `sonner`, `switch`, `table`, `tabs`, `textarea`, `toast`, `toggle-group`, `toggle`, `tooltip`

**Existing files (`button.tsx`, `card.tsx`, `select.tsx`, `tabs.tsx`):** shadcn will prompt to overwrite.
- `button`, `card`, `select` — let it overwrite; they were shadcn-generated previously and may have drifted. Diff before/after, verify no regressions.
- `tabs` — **overwrite.** The existing file is hand-rolled (context-based), not shadcn. We want the Radix version.

**Watch out:**
- If shadcn's CLI tries to run `init` and wants to blow away `components.json`, `globals.css`, or the `@theme` blocks — **STOP.** Don't let it. Those are the theme system. Ask Purr.
- If an install rewrites `tailwind.config.*` — check whether we have a v4 `@theme` block-based setup or a v3 config file. Project is on Tailwind v4 (`@import "tailwindcss"` + `@theme { ... }` in `theme.css`/`App.css`). shadcn should respect that.
- Generated files may import from `@/lib/utils`. We use `@shared/lib/utils`. `components.json` aliases should make shadcn emit the right path; if you see raw `@/` imports, fix the alias or do a find/replace immediately.

### Step 3 — Extend `applyTheme()`

Edit `web/shared/lib/themes.ts`:

1. Extend `ALL_VARS` from 19 → 31. Add:
   - `chart-1`, `chart-2`, `chart-3`, `chart-4`, `chart-5`
   - `sidebar`, `sidebar-foreground`, `sidebar-primary`, `sidebar-primary-foreground`, `sidebar-accent`, `sidebar-accent-foreground`, `sidebar-border`, `sidebar-ring`

2. Extend `DEFAULT_DARK` and `DEFAULT_LIGHT` fallback maps with the same 12 new keys. Pick reasonable defaults (e.g. chart-1 = primary, chart-2 = accent, sidebar = card, sidebar-foreground = card-foreground, etc.). These matter for the `Default` theme's empty-palette case.

3. Verify in DevTools: open the app, change themes, inspect `:root` — should see all 31 vars change.

### Step 4 — Gotcha: shadcn sidebar var-name convention

**Heads-up for Phase 2, not for you to fix now** — but you need to document it.

shadcn's Sidebar component (which just landed in `shared/components/ui/sidebar.tsx`) likely expects variables named `--sidebar-background`, `--sidebar-foreground`, `--sidebar-primary`, etc. Our `themes.json` uses `--sidebar` (no `-background` suffix).

- Open `shared/components/ui/sidebar.tsx`, grep for `--sidebar`. See what names it references.
- Compare to the keys in any theme in `web/shared/data/themes.json` (the `Mrowr Purr - Tron` theme is a good test sample).
- Document the mismatch in `NOTES.md` under "Running observations" so Phase 2 can decide: rename our vars, or edit the sidebar.tsx file to match. **Don't fix this in Phase 1** — wiring Sidebar is Phase 2's job.

Same check for `chart.tsx` — document what var names it expects.

### Step 5 — Regression check

1. Re-run the same tests from Step 0. Everything that was green stays green.
2. Launch the app (`xmake run dev-desktop` or `xmake run desktop`) and click through every tab: Docs, Editor, Todos, Files, System, Settings. Nothing should look different. No runtime errors in DevTools console.
3. Change themes a few times (Settings tab → Theme picker). Should still work. Inspect `:root` in DevTools — see all 31 vars.
4. Screenshot: take a headless playwright-cdp screenshot of each tab against a distinctive theme like "Mrowr Purr - Tron" and save them for Phase 2's Sidebar work to compare against.

If any test regressed, **fix the root cause before calling Phase 1 done.** Per Ethos: unverified "done" is worse than "not started."

### Step 6 — Update `NOTES.md` + commit + push

1. Check off the relevant boxes in NOTES.md Bucket 2 that are now done (applyTheme extended to 31 vars).
2. Add findings to "Running observations" — especially the shadcn var-name mismatch for Phase 2.
3. Commit (one commit is fine — "Phase 1: install shadcn catalog + emit all 31 theme vars"). Emoji-prefixed title per the repo style.
4. Push to `origin/template` (this branch).
5. Ping Purr: "Phase 1 done, ready for your review. PHASE_2 can kick off when you approve."

---

## Phase 1 exit criteria

All of these must be true:

- [ ] `shared/components/ui/` contains ~50 shadcn-generated files.
- [ ] `applyTheme()` emits 31 CSS variables (grep `ALL_VARS` in `themes.ts` for 31 entries).
- [ ] `:root` in DevTools shows `--sidebar-*` and `--chart-*` after a theme switch.
- [ ] Baseline tests (`test-todo-store`, `test-bun`, `test-browser`) still pass.
- [ ] App visually unchanged — every existing tab renders identically to before.
- [ ] NOTES.md has a "Running observations" entry about shadcn var-name conventions (for Phase 2).
- [ ] Clean commit + push on `template` branch.

---

## What's OUT of Phase 1 scope (do NOT touch)

- **Don't wire the Sidebar.** That's Phase 2. Install and document the var-name gotcha, don't render it.
- **Don't swap any hand-rolled primitive.** That's Phase 3.
- **Don't touch any C++ / Qt / WASM code.** This is web-only.
- **Don't touch the bridge system** (no `getBridge` helpers yet — Phase 3).
- **Don't touch `docs/DelightfulQtWebShell/for-agents/` or other agent docs.** That's Phase 5.
- **Don't change `theme.css` or `App.css` @theme blocks** unless a shadcn install absolutely requires it (if it does, pause and ask Purr).
- **Don't remove `screenshot.png` or touch anything on `main` branch.** You're on `template`.

---

## Guardrails (from the Ethos)

- **Own every failure.** "Pre-existing" only applies to things you verified were already red in the baseline. Everything else is yours the moment you see it.
- **Fix root causes, not symptoms.** If a shadcn install breaks an import, fix the import. Don't wrap it in try/catch.
- **Never use destructive git commands.** No `git checkout .`, no `git reset --hard`, no `git stash` (another session may have work). If a file looks unfamiliar, `git show HEAD:path/to/file` to see the original.
- **Match energy with Purr.** She's direct and values directness back. Push back if something in this doc is wrong. Say so loudly if you find something broken — don't bury it in a report.
- **No code comments like `// TODO`.** Purr hates them. Notes go in `NOTES.md`.

---

## If you get stuck

- **Ethos question:** re-read `app/Ethos.md`.
- **Project architecture:** `app/docs/DelightfulQtWebShell/for-agents/02-architecture.md`.
- **Testing mechanics:** `app/docs/DelightfulQtWebShell/for-agents/04-testing.md`.
- **Driving the running app from CLI:** `app/docs/DelightfulQtWebShell/for-agents/05-tools.md` (playwright-cdp pattern).
- **Something smells wrong:** stop, flag to Purr loudly. Don't paper over it. The stakes are low for asking; the stakes are high for a quiet failure.

---

## Final note

This phase has no UI surface area. It's pure setup. Boring is the goal. If you find yourself making cool decisions, you've drifted. Cool decisions live in Phase 2+.

🏴‍☠️
