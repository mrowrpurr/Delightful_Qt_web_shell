# 🏴‍☠️ Phase 5 Kickoff — `--radius` Decision + Agent Docs

You're the agent picking up Phase 5, the final phase of the shadcn-template migration. Phases 1–4 are landed. Read this start to finish before running anything.

This is **a small phase**, but it's the one that makes the migration *stick*. Most of the leak-cleanup landed early in `ea7f5f7`. What's left: one open decision (`--radius` per-theme), and the doc rewrite that locks in the new patterns so the next agent doesn't recreate the drift loop the migration just killed.

---

## Who's who

- **You** — the agent executing Phase 5.
- **Purr** (Mrowr Purr, M.P.) — the human. Lead on all decisions. Direct. Match her energy. Use emoji. Never say "the user."
- **The coordinator agent** (Claude Opus 4.7) — tracks the plan across phases. Interface is `NOTES.md`, `TODO.md`, and git history.

## Read these before you touch code

1. **`app/Ethos.md`** — non-negotiable. Especially "Stay curious," "Fix root causes not symptoms," "Choose self-explanatory names."
2. **`app/web/PHASE_1.md`** preamble — the "drift loop" diagnosis. The whole reason the new agent docs exist is to prevent agents from reading old docs and reproducing the loop. Read it.
3. **`app/web/NOTES.md`** — the live working doc. The 7 locked decisions, all bucket items, all running observations. Phase 5's doc rewrite needs to encode the *settled* state of these.
4. **`app/web/TODO.md`** — phase status at a glance. Confirms Phases 1–4 are done.
5. **`app/web/COMPONENT_AUDIT.md`** and **`app/web/THEME_AUDIT.md`** — the original diagnoses. Useful as raw material for the doc rewrite — but **the docs you write describe the post-migration state, not the audit findings**.
6. **`app/docs/DelightfulQtWebShell/for-agents/`** — the docs you'll be updating. Read all eight before deciding what changes.

---

## What earlier phases gave you

**Phase 1–4, condensed:**
- Full shadcn catalog installed; every primitive in `shared/components/ui/`.
- Sidebar shell, hash routing, `document.title` discipline.
- Zero hand-rolled primitives, zero magic-string bridge calls — typed helpers everywhere (`getSystemBridge()`, `getTodoBridge()`).
- Module-scope no-catch crashes fixed.
- Sonner toasts at app root.
- `ComponentsTab` populated with a section per primitive.
- Chart demo live; all 31 theme vars have real consumers.
- `themes.json` is the single source of truth — no fallback maps in code, `Default` theme populated.

**Already landed in `ea7f5f7` (Phase 5 work, done early):**
- `App.css` `#1a1a1a` hex leak → `var(--color-muted)` ✅
- `theme.css` + `App.css` `@theme` blocks merged into `@theme inline` referencing live CSS vars ✅
- Duplicate `DEFAULT_DARK` / `DEFAULT_LIGHT` palettes deleted from `themes.ts` ✅
- `Default` theme populated in `themes.json` ✅
- `globals.css` shadcn-default `:root { --sidebar: …}` and `.dark { … }` blocks deleted (Phase 1's note "do not delete" is now obsolete — `@theme inline` made them redundant) ✅

**Result:** the entire "Bucket 3 — Leaks, duplication, consistency" list from `NOTES.md` is mostly complete. What's still on the table is small.

---

## Phase 5 goal

Two threads, one phase:

1. **Decide `--radius` per-theme.** Spot-check the imported themes — does any of them actually want a different radius? If yes, move it into `themes.json`. If no, leave it where it is and document why.
2. **Rewrite the agent docs** in `app/docs/DelightfulQtWebShell/for-agents/` so they describe the post-migration reality. Add a "Component Patterns" doc that codifies the new rules. The next agent reading the docs should land *into* the new patterns, not into stale guidance that recreates the drift loop.

Exit state: the migration is fully reflected in the docs. Agents picking this up cold see the right patterns first.

---

## The 7 locked decisions (don't relitigate)

Verbatim from `NOTES.md`. Don't drift:

1. shadcn-first for every primitive.
2. Left-side `Sidebar` replaces the top `TabsList`.
3. All 31 theme vars consumed somewhere real.
4. `shared/components/ui/` holds the full catalog; feature code imports from there.
5. Typed bridge helpers only (`getSystemBridge()`), no `getBridge<T>('name')` magic strings.
6. Full shadcn catalog installed.
7. In-app `🧩 Components` page as a sidebar item.

These are the decisions your "Component Patterns" doc has to teach.

---

## Phase 5 — Execution playbook

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

Record what passes. **Do NOT** run `test-all`, `test-pywinauto`, or `test-desktop` without asking Purr first.

### Step 1 — `--radius` spot-check + decision

Current state:
- `--radius: 0.5rem` lives in `app/web/shared/styles/theme.css:39` (and `apps/main/src/App.css:35` for the dialog window — separate `@theme` block).
- It's **not** in `themes.json`.
- Many shadcn primitives reference it via `var(--radius)` and `calc(var(--radius) - 5px)`.

The question: does any imported theme actually *want* a different radius?

**Spot-check:** the themes were imported from [ui.jln.dev](https://github.com/jln13x/ui.jln.dev). Look at the source repo's theme schema — does it author per-theme radius? Sample a handful of themes (Tron, Synthwave, a couple "neutral"-named ones, a couple "bold"-named ones) and check if their original ui.jln.dev definitions have radius variation.

Three possible outcomes, all valid — pick the one the data supports:

- **A — Radius doesn't vary.** Leave `--radius` where it is. **Document the decision** in the new "Theming" agent doc (or wherever theming is covered). One line: "Radius is intentionally global; `themes.json` is colors only." Close the open question in `NOTES.md`.
- **B — A few themes want different radius.** Add `--radius` to `themes.json` as an optional field; have `applyTheme()` use a default when the field is missing. Update the Default theme's entry. Update the `@theme inline` blocks if needed.
- **C — Many themes want different radius.** Add `--radius` to `themes.json` as a required field; backfill all 1030 entries (script it). Move it out of the `@theme` blocks. Make Tailwind utility classes resolve from the live var.

**The "no-op" outcome (A) is the most likely** based on `THEME_AUDIT.md` line 187 ("`--radius` is a theme-like concept that *every* theme inherits identically because it lives in CSS not in `themes.json`. Custom themes can't override it.") The audit notes it as a *gap*, but the gap may not be one anyone cares about. Let the data tell you.

If outcome is A: total work is ~5 lines of doc. If B or C: real engineering, get Purr's sign-off on scope before doing it.

### Step 2 — Agent doc rewrite

This is the longer part of Phase 5. The eight `for-agents` docs were written before the migration. Most are still mostly accurate (they describe the C++/bridge architecture, which didn't change). What needs to change:

**2a. Audit the existing docs first.** Read all eight. For each, note:
- Is anything in here now wrong? (e.g. "the Tabs primitive is hand-rolled" — no longer true)
- Is anything in here now misleading? (e.g. "use `getBridge<T>('name')` to access a bridge" — directly contradicts the typed-helper rule)
- Is anything missing that the post-migration agent needs?

Most likely deltas (verify each by reading the actual doc):
- **`01-getting-started.md`** — the project layout section may still describe the pre-migration `web/` structure. Update to reflect the populated `shared/components/ui/`.
- **`02-architecture.md`** — bridge access example almost certainly uses `getBridge<TodoBridge>('todos')`. Update to `getTodoBridge()` and explain why magic strings are banned.
- **`03-adding-features.md`** — bridge feature recipe. Needs to teach: when adding a bridge, also add a typed helper (`getMyBridge()`) in the same PR. The bridge interface lives in `web/shared/api/<name>-bridge.ts` alongside the helper. **The scaffolder (`xmake run scaffold-bridge`) should also be updated** if it doesn't already emit a typed helper — verify and patch if needed.
- **`05-tools.md`** — playwright-cdp examples may still target old top-tab selectors. Update to sidebar `data-testid="sidebar-<id>"`.
- **`06-gotchas.md`** — quick-reference table. Add new entries for the post-migration patterns; remove ones that are now resolved.
- **`07-desktop-capabilities.md`** — likely still accurate (capabilities didn't change).
- **`08-theming.md`** — major update needed. Drop any references to `DEFAULT_DARK` / `DEFAULT_LIGHT` fallback maps, the `globals.css` default `:root` block, the empty `Default` theme. Make sure it describes `@theme inline` referencing live vars and that `themes.json` is the single source of truth.

**Don't rewrite for the sake of rewriting.** If a doc is correct, leave it alone. Surgical edits are better than wholesale rewrites — they're easier to review and less likely to introduce new mistakes.

**2b. Add the new "Component Patterns" doc.** New file: `app/docs/DelightfulQtWebShell/for-agents/09-component-patterns.md`.

This is the doc that kills the drift loop. It teaches the agent reading it:

- **The shadcn-first rule.** All primitives come from `@shared/components/ui/`. If a primitive isn't there, install it via `bunx shadcn@latest add <name>` — don't hand-roll.
- **The typed-bridge-helper rule.** Every bridge has a typed helper (`getXxxBridge()`) in `@shared/api/<name>-bridge.ts`. Feature code never imports `getBridge` directly. Magic strings leak C++ registration names and break silently on rename.
- **The no-catch crash rule.** Module-scope `await get*Bridge(...)` without `.catch()` crashes the tab on bridge failure. Use the resilient pattern.
- **The Components page contract.** Every primitive in `shared/components/ui/` has a section in `ComponentsTab`. New primitives → new section.
- **The 31-var theme contract.** Every theme var has at least one consumer. New design tokens → new var → new consumer.
- **The hash-routing contract.** `App.tsx` initializes `currentTab` from `window.location.hash` and writes it back on change. Don't break this.
- **The Sonner-at-root contract.** Toasts use `toast(...)` from `sonner`. Don't reintroduce `setState + setTimeout` patterns.

**Style match the existing for-agents docs** — terse, scannable, tables where they fit, code blocks for patterns. Match the voice of `06-gotchas.md` for the rule list, `02-architecture.md` for the conceptual sections.

**2c. Update the index.** `app/README.md` (or wherever the for-agents doc index lives) needs `09-component-patterns.md` added.

### Step 3 — Update `NOTES.md` and `TODO.md` to closure

After Phase 5, the migration is *done*. Both docs should reflect that.

**`NOTES.md`:**
- Mark Phase 5 complete.
- Resolve the `--radius` open question with the Step 1 outcome.
- Move "Running observations" entries that are no longer relevant into a "Resolved" subsection or delete (e.g. "another agent is working in this codebase" — that's a session-specific note, can go).
- Add a "Migration complete" header or similar so future readers know `NOTES.md` is now historical, not active.

**`TODO.md`:**
- Flip Phase 5 to `[x]` with the commit hash.
- Either retire `TODO.md` or repurpose it for the next round of work (Purr's call).

### Step 4 — Test sweep

```bash
xmake run test-todo-store    # ✅
xmake run test-bun           # ✅
xmake run test-browser       # ✅
```

If you went with outcome B or C on `--radius`, also run a manual visual check across a few themes — radius is widely consumed in shadcn primitives, a wrong wiring will show up everywhere at once.

### Step 5 — Commit, push, ship

1. Single emoji-prefixed commit. Suggested: `🏴‍☠️ Phase 5: --radius decision + agent docs reflect post-migration patterns`. Or split into two commits if the radius decision is non-trivial.
2. Push to `origin/template`.
3. Tell Purr: "Phase 5 done — the migration is complete. Ready for review."

---

## Phase 5 exit criteria

All must be true:

- [ ] **`--radius` decision made** with one-line documented rationale (in the appropriate agent doc and `NOTES.md` open-question section).
- [ ] **All eight existing for-agents docs** are accurate against the post-migration code. No stale `getBridge<T>('name')`, no references to deleted fallback maps, no top-tab selectors in playwright-cdp examples.
- [ ] **`09-component-patterns.md`** exists and teaches the seven post-migration rules (shadcn-first, typed helpers, resilient bridge access, Components page contract, 31-var theme contract, hash routing, Sonner at root).
- [ ] **`app/README.md` index** lists the new doc.
- [ ] **`xmake run test-todo-store`** ✅
- [ ] **`xmake run test-bun`** ✅
- [ ] **`xmake run test-browser`** ✅
- [ ] **`NOTES.md` open questions** all resolved or explicitly marked "won't do" with reason.
- [ ] **`TODO.md`** has Phase 5 checked off; migration marked complete.
- [ ] **`bun run build:main`** inside `web/` ✅ (sanity check — radius edits could break compile).

---

## Out of Phase 5 scope (do NOT touch)

- **No primitive swaps, no new components, no chart additions.** That work is done.
- **No C++ / Qt / WASM changes** beyond what `--radius` outcome B/C might force (and only if Purr signs off).
- **No `themes.json` edits** unless `--radius` outcome is B or C.
- **Don't touch CI/CD or the release workflow.** Those are working.
- **Pre-existing npm leakage** (`xmake/setup.lua:23`, `app/playwright.config.ts:33`). Could be `bunx` / `bun run` but it's a separate housekeeping pass, not a Phase 5 deliverable. Flag in findings if you have a strong opinion.
- **`SystemTab` WASM crash rumor.** Not a docs problem. If you're confident it's been fixed by Phase 3's no-catch standardization, document and close. Don't go investigating proactively.
- **Phase 2 commit `74a6847` `(WIP)` label / missing Phase 2 findings.** Cosmetic. If you want to add a `NOTES.md` line acknowledging Phase 2 landed in `74a6847`, fine. Don't rewrite history.

---

## Watch-outs

- **Doc rewrites have a unique failure mode: lying.** It's easy to write "the bridge access pattern is X" when the code has drifted to Y. **Verify every claim against the current code.** When in doubt, grep first.
- **Don't overwrite `06-gotchas.md` wholesale.** It's been carefully curated — agents reading it use it as a quick-reference. Surgical updates only.
- **The "Component Patterns" doc is high-leverage but easy to over-engineer.** Aim for ~150 lines, scannable. Each rule: name, one-line rule, one-line why, one code example. If it grows past 300 lines, you've drifted.
- **Hash routing claim in docs vs code.** The `App.tsx` comment block at lines 91-94 explains *why* hash routing is required (custom URL schemes can't `replaceState` paths). Cite that in the new doc — it's load-bearing context that should not be rediscovered.
- **`--radius` outcome B is the trap.** "Some themes might want it" easily becomes "let's add it everywhere just in case." Don't add a feature unless data shows themes actually want it. Defaulting to outcome A is fine and probably correct.
- **Backwards-compat shims.** When you delete or move things in the docs, don't leave "as of phase 4 this used to be…" notes. The git history is the history. Docs describe current state.

---

## Guardrails (Ethos)

- **Run the tests *before* you write a line.** Even for a docs-heavy phase, baseline matters — radius edits can break compile.
- **Verify before you write.** Doc claims that don't match the code rot fast.
- **Fix root causes.** If a doc is wrong, fix the doc — don't add a "but actually" footnote.
- **Own every failure.** A broken doc is your problem now.
- **Never destructive git.** No `git reset --hard`, no `git checkout .`, no `git stash`.
- **No `// TODO` comments in code.** Notes go in `NOTES.md` findings.
- **Choose self-explanatory names.** The new doc's filename, section names, and rule names matter — the next agent's first impression is "is this thing worth reading?"

---

## If you get stuck

- **Existing doc voice/style:** read `06-gotchas.md` and `02-architecture.md` to internalize tone before writing the new doc.
- **Theming context:** `app/docs/DelightfulQtWebShell/for-agents/08-theming.md` is your starting point — and a likely target for surgical updates.
- **The settled state of the codebase:** grep is your friend. `getBridge<` should have zero matches in `apps/main/src/`. `setTimeout.*setMessage` should have zero matches. If anything still shows up, that's a Phase 3 leak — escalate to Purr; don't silently fix it during a Phase 5 doc rewrite.
- **Anything feels off:** stop, ping Purr in a short message. Cost of asking is low; cost of a quiet failure is high.

---

## Final note

This is the **last phase**. The end state isn't "the migration is done" — it's "the migration is done *and the docs prove it*." The next agent who lands cold in this codebase reads `09-component-patterns.md` and gets the right pattern in their head before they touch any code. That's the only durable way to keep the drift loop dead.

Ship clean. 🏴‍☠️
