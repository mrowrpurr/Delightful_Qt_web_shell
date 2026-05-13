# Tutorial — Your First Feature in 5 Minutes

> **Shortcut:** `xmake run scaffold-bridge <name>` scaffolds a new bridge end-to-end. This tutorial walks through the pattern manually so you understand what's happening under the hood.

We'll add an `addItem` method — from C++ domain logic to React UI — and see how the three-file pattern works (domain logic + bridge + TypeScript interface).

## The Three Files

```
├── <repo>/lib/todos/include/todo_store.hpp              ← C++ domain logic (pure)
├── <repo>/app/bridges/todos/include/todo_bridge.hpp     ← Bridge — extends app_shell::Bridge
└── <repo>/app/web/packages/bridge/lib/bridges/todo-bridge.ts  ← TypeScript interface
```

No separate Qt bridge and WASM bridge — one C++ class serves both targets. The framework's transport adapters handle serialization at the edges.

## Step 1: Write the C++ Logic

Add the method to `TodoStore` — pure C++, no Qt, no JSON.

**`<repo>/lib/todos/include/todo_store.hpp`:**

```cpp
TodoItem add_item(const std::string& list_id, const std::string& text) {
    TodoItem item{gen_id(), list_id, text, false, now_iso()};
    items_.push_back(item);
    return item;
}
```

## Step 2: Define the Request DTO

Bridge methods take **one request struct** as input. Add it to the DTOs header.

**`<repo>/lib/todos/include/todo_dtos.hpp`:**

```cpp
struct AddItemRequest {
    std::string list_id;
    std::string text;
};
```

That's the whole DTO. PFR (Boost.PFR) walks the struct fields and serializes them — no macros, no registration, no hand-written `to_json()`.

## Step 3: Add the Bridge Method

Add a method to `TodoBridge` and register it in the constructor. The method takes the request DTO and returns a domain struct.

**`<repo>/app/bridges/todos/include/todo_bridge.hpp`:**

```cpp
class TodoBridge : public app_shell::Bridge {
public:
    TodoBridge() {
        // ... existing registrations ...
        method("addItem", &TodoBridge::addItem);

        signal("itemAdded");
    }

    TodoItem addItem(AddItemRequest req) {
        auto item = store_.add_item(req.list_id, req.text);
        emit_signal("itemAdded", item);
        return item;
    }
};
```

That's it on the C++ side. `def_type::from_json` deserializes the request automatically. `def_type::to_json` serializes the response. One bridge class serves desktop *and* WASM.

## Step 4: Mirror in TypeScript

**`<repo>/app/web/packages/bridge/lib/bridges/todo-bridge.ts`:**

```typescript
export interface TodoBridge {
  // ... existing methods ...
  addItem(req: { list_id: string; text: string }): Promise<TodoItem>
  itemAdded(callback: (item: TodoItem) => void): () => void
}
```

Bridge calls take a **request object** — not positional arguments:

```typescript
// Correct
await todos.addItem({ list_id: id, text: 'Buy milk' })

// Wrong — won't work
await todos.addItem(id, 'Buy milk')
```

## Step 5: Use It in React

```typescript
const todos = await getBridge<TodoBridge>('todos')
await todos.addItem({ list_id: listId, text: 'Buy milk' })
```

That's it. Three files, no wiring, no glue code.

## What Just Happened?

| File | What you wrote |
|------|----------------|
| `todo_store.hpp` | The actual logic (shared by both targets) |
| `todo_dtos.hpp` | The request DTO — a plain struct |
| `todo_bridge.hpp` | `method("addItem", ...)` + the method body |
| `todo-bridge.ts` | TypeScript interface line (shared by both targets) |

The framework infrastructure didn't change. The desktop transport (`BridgeChannelAdapter`) dispatches the call through QWebChannel. The WASM transport (`WasmBridgeWrapper`) dispatches through Embind. Both call the same `TodoBridge::addItem(AddItemRequest)`.

## Return Types

The framework picks JSON shapes from your C++ return type:

| C++ returns | JSON result | Notes |
|------------|-------------|-------|
| A `def_type` struct | The struct as a JSON object | `{"id": "1", "name": "Groceries"}` |
| `std::vector<T>` | A JSON array | Each element serialized recursively |
| `bool` | `{"ok": value}` | |
| `void` / `OkResponse` | `{"ok": true}` | For side-effect-only methods |
| `nlohmann::json` | Passthrough | For manually constructed responses |

## Adding a New Bridge

When you need a new domain area (not just a method on `todos`):

```bash
xmake run scaffold-bridge notes
```

This creates the bridge class, request DTOs header, and TypeScript interface stub, and wires registration into both entry points (`application.cpp` and `test_server.cpp`). No xmake.lua edits needed — the targets use glob discovery.

Then add your methods to the bridge class and mirror them in the TS interface.

## Signals — Push Events with Typed Payloads

Signals carry typed data, not just a "something changed" notification:

```cpp
// In the constructor:
signal("itemAdded");
signal("listDeleted");

// In a method:
TodoItem addItem(AddItemRequest req) {
    auto item = store_.add_item(req.list_id, req.text);
    emit_signal("itemAdded", item);   // item is serialized via def_type::to_json
    return item;
}
```

Subscribe in TypeScript with a typed callback:

```typescript
const cleanup = todos.itemAdded((item: TodoItem) => {
  console.log('item added:', item)
  refresh()
})
// Later: cleanup() to unsubscribe
```

In a React component:

```typescript
useEffect(() => {
  const cleanup = todos.itemAdded((item) => {
    setItems(prev => [...prev, item])
  })
  return cleanup
}, [])
```

## Validate Your Work

```bash
xmake run test-all            # run all test layers
```

The desktop and WASM builds catch DTO/method mismatches at compile time — if your request struct doesn't match what the bridge method expects, the code won't compile.
