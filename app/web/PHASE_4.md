# 🏴‍☠️ Phase 4 Kickoff — Chart Demo + Vocabulary Completion

You're the agent picking up Phase 4. Phases 1, 2, and 3 are landed. Read this start to finish before running anything.

This is **a small phase**. Two deliverables: build a real chart demo, and wire `--chart-3` to a real consumer so all 31 theme vars from `themes.json` are alive. By exit, the original "no dead theme vocabulary" decision finally pays off.

---

## Who's who

- **You** — the agent executing Phase 4.
- **Purr** (Mrowr Purr, M.P.) — the human. Lead on all decisions. Direct. Match her energy. Use emoji. Never say "the user."
- **The coordinator agent** (Claude Opus 4.7) — tracks the plan across phases. Interface is `NOTES.md`, `TODO.md`, and git history.

## Read these before you touch code

1. **`app/Ethos.md`** — non-negotiable. Especially "Run the tests before you write a single line," "Own every failure," "Never declare success without proof."
2. **`app/web/PHASE_1.md`** preamble — the shared backstory (drift loop, dead theme vocabulary). Skip the playbook.
3. **`app/web/NOTES.md`** — the live working doc. The "Open questions" section has the chart-placement question Phase 4 has to answer.
4. **`app/web/TODO.md`** — phase status at a glance. Confirms what's done.
5. **`app/web/THEME_AUDIT.md`** — the dead-vocabulary diagnosis. Section "What it drops" and the matrix at the bottom (`--chart-3` → "nobody") are your starting point.
6. **`app/web/shared/components/ui/chart.tsx`** — the shadcn Chart wrapper. Read it. Phase 1 findings note: "consumers pass colors via `ChartConfig` (e.g. `color: 'var(--chart-1)'`), so `--chart-*` names from `themes.json` Just Work."
7. **`app/web/apps/main/src/tabs/ComponentsTab.tsx`** — the existing Chart section is a stub at line ~870. Phase 4 replaces it (or adds alongside) with a real demo.
8. **`app/docs/DelightfulQtWebShell/for-agents/08-theming.md`** — how the live theme reaches your components.

---

## What earlier phases gave you

**From Phase 1 (`cfc2487`):**
- `chart.tsx` is already installed in `shared/components/ui/`. Recharts is bundled. No `bunx shadcn add chart` needed.
- `applyTheme()` already emits `--chart-1` through `--chart-5` to `:root` on every theme switch. The vars land — there's just nothing reading `--chart-3`.

**From Phase 2 (`74a6847`):**
- The Sidebar with seven items is the app chrome. If Phase 4 adds a `📊 Stats` sidebar item, follow the same `NAV_ITEMS` + `TAB_TITLES` + hash routing pattern in `App.tsx`.

**From Phase 3:**
- `ComponentsTab.tsx` is fully populated. The Chart section currently says "full demo lands in Phase 4 alongside `--chart-*` wiring" — that's your invitation.
- Bridge access is via typed helpers (`getSystemBridge()`, `getTodoBridge()`). If Phase 4 needs todo data for a "completion-over-time" chart embedded in TodosTab, it goes through `getTodoBridge()`.
- Sonner is mounted at the app root for toasts.
- Module-scope `await getBridge(...)` no-catch crashes are fixed — follow the resilient pattern (`getXxxBridge().then(b => { …}).catch(() => {})`) for any new bridge consumer.

**From `ea7f5f7` (Phase 5 work landed early):**
- `Default` theme in `themes.json` has real values for all 31 vars including `--chart-1..5`. Don't reintroduce hardcoded fallbacks.
- `theme.css` and `App.css` use `@theme inline { --color-chart-1: var(--chart-1); … }` for all five chart vars. Tailwind utilities like `bg-chart-3` resolve correctly.

---

## Phase 4 goal

Two threads, one phase:

1. **Build a real chart demo** that reads from live data and recolors when the theme changes.
2. **Wire `--chart-3`** so it has at least one real consumer. Today Monaco uses `chart-1, 2, 4, 5` and explicitly skips `chart-3` — the chart demo is the natural home.

Exit state: every one of the 31 theme vars in `themes.json` has a live consumer. The "no dead theme vocabulary" decision from `NOTES.md` is fully cashed.

---

## The 7 locked decisions (don't relitigate)

Verbatim from `NOTES.md`. Don't drift:

1. shadcn-first for every primitive. *(done — Phase 3)*
2. Left-side `Sidebar` replaces the top `TabsList`. *(done — Phase 2)*
3. **All 31 theme vars consumed somewhere real.** *(your job to finish)*
4. `shared/components/ui/` holds the full catalog; feature code imports from there.
5. Typed bridge helpers only (`getSystemBridge()`), no `getBridge<T>('name')` magic strings. *(done — Phase 3)*
6. Full shadcn catalog installed. *(done — Phase 1)*
7. In-app `🧩 Components` page as a sidebar item. *(populated — Phase 3; you fill the Chart section)*

---

## The open question Phase 4 has to answer

**Where does the chart demo live?** `NOTES.md` left this open:

- **Option A — own `📊 Stats` sidebar item.** Cleaner for a template. The "look at me, here's a chart against your theme" surface. Easier to evolve. Adds one more sidebar entry.
- **Option B — embedded in `TodosTab`.** More realistic — todos completion-over-time. Shows charts in actual feature context. Doesn't pad the sidebar.

**Recommendation if Purr hasn't decided:** propose a spaghetti-throw — give her both with a quick recommendation. The template angle leans A (template = "look what you can do"); the realism angle leans B. **Do not decide unilaterally** — surface the choice, let Purr pick, then build the chosen one. She may want both.

The Components page Chart section gets a real demo regardless of where the "real" chart lives — that's the inventory page, every primitive shows up there.

---

## Phase 4 — Execution playbook

### Step 0 — Baseline

```bash
git pull origin template
git status

cd app
bun install

# Non-invasive tests — invisible to Purr
xmake run test-todo-store
xmake run test-bun
xmake run test-browser
```

Record what passes. **Do NOT** run `test-all`, `test-pywinauto`, or `test-desktop` without asking Purr first — those take over her desktop.

Visual baseline: snapshot the current Components page Chart section against a high-contrast theme like "Mrowr Purr - Tron." That's your before-shot.

### Step 1 — Decide chart placement

Throw spaghetti. Two options (A own `📊 Stats` tab, B embedded in `TodosTab`), one-paragraph tradeoffs each, ask Purr to pick. Don't proceed until she's chosen. If she says "do both," do both.

### Step 2 — Build the real chart demo

Wherever the chart lives, the implementation pattern is the same. shadcn's `chart.tsx` wraps Recharts and reads colors from `ChartConfig`:

```typescript
import { ChartContainer, ChartTooltip, ChartTooltipContent } from '@shared/components/ui/chart'
import { Bar, BarChart, CartesianGrid, XAxis } from 'recharts'

const chartConfig = {
  desktop: { label: 'Desktop', color: 'var(--chart-1)' },
  mobile:  { label: 'Mobile',  color: 'var(--chart-2)' },
  tablet:  { label: 'Tablet',  color: 'var(--chart-3)' },  // ← the dead one wakes up
  watch:   { label: 'Watch',   color: 'var(--chart-4)' },
  tv:      { label: 'TV',      color: 'var(--chart-5)' },
} satisfies ChartConfig
```

**Whatever data you pick, the demo MUST use all five chart vars.** Five-series bar chart, five-segment pie chart, five-line line chart — pick whichever reads cleanly. The point is `--chart-3` finally has a live consumer.

Real-data options if you go embedded in `TodosTab` (Option B):
- Completion percentage per list (one bar per list, color per list).
- Items added vs completed over time (two series — needs only chart-1/2, so add three more dimensions or pick a different shape).
- Per-list breakdown of done/pending/total (three series — still short of five, expand the design).

If the real-data chart only naturally uses 3-4 vars, **also** put a 5-series demo in `ComponentsTab` — that's the inventory page, it can demonstrate all five regardless.

### Step 3 — Verify theme reactivity

The chart MUST recolor on theme change. Test:

1. Open the app, navigate to the chart.
2. Change theme in Settings — try Tron, Synthwave, Default-light, Default-dark.
3. Each theme should give visibly different chart colors with no reload.

If colors don't update on theme switch: the chart is reading stale CSS values. Recharts captures fill/stroke on mount unless you pass them through CSS vars at every render. shadcn's `ChartContainer` injects a `<style>` block that maps `--color-{key}` to the config color — verify the React tree is re-mounting or that the CSS injection picks up theme changes via the `:root` updates `applyTheme()` writes.

### Step 4 — Verify `--chart-3` is alive

Sanity check:

1. Pick a theme where `--chart-3` is visibly distinct (Tron's `--chart-3` is a saturated purple — easy to spot).
2. Find the chart series wired to `--chart-3` and confirm it renders that color.
3. If you went with Option A and Stats is its own tab — verify the chart shows up at `#stats` and `document.title` updates correctly.

### Step 5 — Update the Components page Chart section

Replace the stub at `ComponentsTab.tsx:870-872` ("Phase 4 wires…") with a real working chart demo. This is the inventory entry — it should always render against the live theme so theme authors can spot when their theme breaks chart legibility.

Recommendation: 5-series demo. Bar or pie. Static toy data is fine here (the page is a catalog, not a feature).

### Step 6 — Update bookkeeping, commit, push

1. Flip Phase 4 boxes in `NOTES.md` (Bucket 2: build chart demo + activate `--chart-*`).
2. Flip Phase 4 in `TODO.md` to `[x]` with the commit hash.
3. Add a "Phase 4 findings" section to `NOTES.md`. Anything Phase 5 needs to know goes here — especially: did `--chart-3` use ergonomically map to a fifth real-data dimension, or did you have to pad? That data-shape lesson matters for whoever ships future chart features.
4. Single emoji-prefixed commit. Suggested: `🏴‍☠️ Phase 4: chart demo lights up --chart-* vocabulary`.
5. Push to `origin/template`.
6. Tell Purr: "Phase 4 done, ready for review."

---

## Phase 4 exit criteria

All must be true:

- [ ] **A real chart demo renders** in whichever location Purr picked (own tab, embedded, or both).
- [ ] **All five `--chart-*` vars have a live consumer.** Specifically: `--chart-3` is no longer dead.
- [ ] **Theme switches recolor the chart visibly** with no reload.
- [ ] **Components page Chart section is real** — no more "Phase 4 wires…" stub text.
- [ ] If Option A (own tab): seventh sidebar item not eighth — wait, it's now eighth (Docs, Editor, Todos, Files, System, Settings, Components, Stats). `TAB_TITLES` and `NAV_ITEMS` updated; hash routing works (`#stats`); `document.title` updates correctly.
- [ ] **`xmake run test-todo-store`** ✅
- [ ] **`xmake run test-bun`** ✅
- [ ] **`xmake run test-browser`** ✅
- [ ] **`bun run build:main`** inside `web/` ✅ (no TS errors, no surprise chunk-size jumps — Recharts is already bundled, so this should be a wash)
- [ ] `NOTES.md` has a "Phase 4 findings" section. `TODO.md` has Phase 4 checked off with the commit hash.

---

## Out of Phase 4 scope (do NOT touch)

- **No more primitive swaps.** That work is done in Phase 3. Don't reopen swaps just because you spot something while building the chart.
- **No `--radius` decision.** That's Phase 5.
- **No agent docs updates.** That's Phase 5.
- **No C++ / Qt / WASM changes.** Web only.
- **No theme JSON edits** — `themes.json` is the source of truth and was finalized in `ea7f5f7`. If a chart looks bad on a specific theme, that's the theme author's choice.
- **Don't refactor `ComponentsTab.tsx` structure.** Just replace the Chart section content.
- **Pre-existing npm leakage** (`xmake/setup.lua:23`, `app/playwright.config.ts:33`). Out of scope; flag in findings if you want.

---

## Watch-outs

- **Recharts re-render on theme change.** If colors stick after a theme switch, the issue is almost always Recharts caching paint values. shadcn's `ChartContainer` handles this if you use `var(--chart-N)` strings (not resolved colors) in `ChartConfig`. Don't pre-resolve them with `getComputedStyle`.
- **Five vars, five series.** Resist the urge to "just use chart-1/2 and skip the rest" — the whole point of the phase is `--chart-3` waking up. If the real data shape doesn't want five dimensions, either (a) reshape the demo, or (b) ensure the Components page demo is unambiguously five-series.
- **`document.title` for a new tab.** If you go Option A, follow the existing `App.tsx` pattern exactly. The Qt window title comes from `document.title` via the web engine's `titleChanged` signal — if your tab doesn't update the title, the dock label won't update either.
- **Hash collision.** `#stats` shouldn't collide with anything; the hash space is yours. Confirm `currentTab` initialization in `App.tsx` accepts `stats` as a valid hash (the `hash in TAB_TITLES` guard handles this if you add `stats` to `TAB_TITLES`).
- **Sidebar ordering.** Where in the sidebar does `📊 Stats` go? Recommendation: between Settings and Components, since it's app-content adjacent (like Settings) but inventory-adjacent (like Components). Ask Purr if she'd rather it land elsewhere.
- **`SystemTab` WASM crash** still rumored — Phase 3's no-catch standardization may have incidentally fixed it. If you happen to verify either way during testing, document in findings.

---

## Guardrails (Ethos)

- **Run the tests *before* you write a line.** Baseline matters.
- **Fix root causes.** If chart colors don't update on theme switch, find why. Don't paper with a force-re-mount key.
- **Own every failure.** If you break a green test, it's yours.
- **Never destructive git.** No `git reset --hard`, no `git checkout .`, no `git stash`. Another session may have work in the tree.
- **No `// TODO` comments in code.** Notes go in `NOTES.md` findings.
- **If the chart-placement decision feels like it has a third option** (e.g. small chart in System tab), throw spaghetti and let Purr pick. Don't unilaterally invent a fourth.

---

## If you get stuck

- **shadcn Chart API:** read `shared/components/ui/chart.tsx` directly. Recharts docs at [recharts.org](https://recharts.org) for chart types.
- **Adding a new sidebar item:** copy the existing `NAV_ITEMS` + `TAB_TITLES` + `renderTab` switch pattern in `App.tsx`. Don't reinvent.
- **Theme tokens:** `app/docs/DelightfulQtWebShell/for-agents/08-theming.md`.
- **Driving the running app:** `app/docs/DelightfulQtWebShell/for-agents/05-tools.md` for playwright-cdp.
- **Anything feels off:** stop, ping Purr in a short message. Cost of asking is low; cost of a quiet failure is high.

---

## Final note

This is the **shortest phase by a wide margin**, but it closes the longest-running thread in the migration: the dead theme vocabulary. The 12 unused vars Phase 1 diagnosed are now down to one (`--chart-3`). After this phase: zero. That's the symbolic close.

🏴‍☠️
