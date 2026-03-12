# Testing Guide

Four test layers catch bugs at different levels — from pure C++ logic to full UI flows.

## I Added a Feature — What Tests Do I Write?

| What changed | What to do |
|---|---|
| Domain logic in `todo_store.hpp` | Add a Catch2 test |
| New bridge method in `bridge.hpp` | Nothing — the test server uses the real bridge |
| UI behavior changed | Add a Playwright e2e test |
| Nothing visible changed | You probably don't need a new test |

## Something Broke — Which Test Tells Me Where?

| Test that fails | What's wrong |
|---|---|
| **Catch2** (`test-todo-store`) | C++ domain logic. Fix `todo_store.hpp`. |
| **Bun** (`test-bun`) | Bridge protocol — message format, args, events. |
| **Playwright browser** (`test-browser`) | UI + backend integration. Could be React, bridge, or server. |
| **Playwright desktop** (`test-desktop`) | Same tests against the real Qt app. |

Work from the bottom up: if Catch2 passes but Bun fails, the logic is fine but the protocol is wrong. If Bun passes but e2e fails, the protocol is fine but the UI isn't wired up correctly.

## How Do I Run Tests?

### Setup (one time)

```bash
bun install
npx playwright install chromium
```

### Quick reference

| Layer | Command | Speed |
|-------|---------|-------|
| C++ unit (Catch2) | `xmake run test-todo-store` | ~instant |
| TS unit (Bun) | `xmake run test-bun` | < 1s |
| E2E browser (Playwright) | `xmake run test-browser` | ~5s |
| E2E desktop (Playwright) | `xmake build desktop && xmake run test-desktop` | ~15s |
| All (Catch2 + Bun + browser e2e) | `xmake run test-all` | ~10s |

### What to expect

**Catch2** prints assertion counts:
```
All tests passed (33 assertions in 11 test cases)
```

**Bun** prints pass/fail per test:
```
✓ sends correct JSON-RPC message for a no-arg method
✓ sends args for methods with parameters
...
8 pass
```

**Playwright** starts a backend, launches a browser, runs through UI flows:
```
4 passed
```

**Desktop e2e** runs the same test suite against the real Qt app. It's slower and can be less stable (GPU, window manager). Good for CI, don't gate on it locally.

### Common failures

| Symptom | Likely cause |
|---|---|
| Catch2 won't compile | Syntax errors in `todo_store.hpp` or `bridge.hpp` |
| Bun tests timeout | Something else using port 9876? |
| E2e tests fail to start | Run `xmake build dev-server` |
| E2e "locator not found" | A `data-testid` changed in your React components |
| Desktop tests fail | Run `xmake build desktop` first |
| Desktop tests flaky | GPU/window manager issues — inherently less stable |

## How Do I Add a New Test?

### Catch2 — domain logic

Test your C++ directly. No mocking, no setup.

#### `lib/todos/tests/unit/todo_store_test.cpp`

```cpp
TEST_CASE("delete_list removes the list and its items") {
    TodoStore store;
    auto list = store.add_list("Groceries");
    store.add_item(list.id, "Milk");

    store.delete_list(list.id);

    REQUIRE(store.list_lists().empty());
    REQUIRE(store.search("Milk").empty());
}
```

Run: `xmake run test-todo-store`

### Playwright — UI flows

Drive a real browser against the full stack.

#### `tests/e2e/todo-lists.spec.ts`

```typescript
test('delete a list', async ({ page }) => {
  await page.goto('/')

  // Create a list first
  await page.getByTestId('new-list-input').fill('Temporary')
  await page.getByTestId('create-list-button').click()
  await expect(page.getByTestId('todo-list').filter({ hasText: 'Temporary' })).toBeVisible()

  // Delete it
  await page.getByTestId('delete-list-button').click()
  await expect(page.getByTestId('todo-list').filter({ hasText: 'Temporary' })).not.toBeVisible()
})
```

Run: `xmake run test-browser`

## Test Architecture at a Glance

```
Catch2           Bun              Playwright browser      Playwright desktop
  │                │                   │                     │
  ▼                ▼                   ▼                     ▼
TodoStore     Bridge Proxy       React + C++ server     Same tests → real Qt app
(pure C++)    (protocol only)    (full integration)     (same assertions)
```

Each layer catches a different class of bug. See [ARCHITECTURE.md](ARCHITECTURE.md) for how the pieces fit together.
