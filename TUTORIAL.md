# Tutorial

Your first feature in 5 minutes. We'll add an `addItem` method — from C++ to React — and see how the pieces connect.

## Table of Contents

- [Adding a Feature](#adding-a-feature)
  - [1. Write the C++ logic](#1-write-the-c-logic)
  - [2. Expose it to JavaScript](#2-expose-it-to-javascript)
  - [3. Define the TypeScript interface](#3-define-the-typescript-interface)
  - [4. Call it from React](#4-call-it-from-react)
  - [What just happened?](#what-just-happened)
- [Events and Signals](#events-and-signals)
  - [C++ — emit a signal](#c--emit-a-signal)
  - [TypeScript — subscribe by signal name](#typescript--subscribe-by-signal-name)
  - [Adding a new signal](#adding-a-new-signal)
  - [Cleanup](#cleanup)
- [Adding a New Bridge](#adding-a-new-bridge)
- [Where Does My Code Go?](#where-does-my-code-go)
- [How the Proxy Works (If You're Curious)](#how-the-proxy-works-if-youre-curious)

---

```
├── lib/
│   ├── todos/include/todo_store.hpp        ← C++ domain logic
│   └── web-bridge/include/bridge.hpp       ← Q_INVOKABLE wrapper (registered as "todos")
└── web/src/api/bridge.ts                   ← TypeScript interface
```

## Adding a Feature

We're adding `addItem` — adds a todo item to a list. Three files, four steps.

### 1. Write the C++ logic

Add the method to `TodoStore` — your pure C++ domain logic. No Qt, no JSON, just business logic.

#### `lib/todos/include/todo_store.hpp`

```cpp
class TodoStore {
    // ... existing methods: add_list, list_lists, etc.

    TodoItem add_item(const std::string& list_id, const std::string& text) {
        TodoItem item{gen_id(), list_id, text, false, now_iso()};
        items_.push_back(item);
        return item;
    }
};
```

### 2. Expose it to JavaScript

Add a `Q_INVOKABLE` method to `Bridge` — the QObject wrapper that makes `TodoStore` callable from the web.

#### `lib/web-bridge/include/bridge.hpp`

```cpp
class Bridge : public QObject {
    Q_OBJECT
    TodoStore store_;

    // One to_json() per type you expose. Qt has built-in JSON types.
    static QJsonObject to_json(const TodoItem& i) {
        return {
            {"id",         QString::fromStdString(i.id)},
            {"list_id",    QString::fromStdString(i.list_id)},
            {"text",       QString::fromStdString(i.text)},
            {"done",       i.done},
            {"created_at", QString::fromStdString(i.created_at)},
        };
    }

public:
    Q_INVOKABLE QJsonObject addItem(const QString& listId, const QString& text) {
        auto item = store_.add_item(listId.toStdString(), text.toStdString());
        emit itemAdded();
        return to_json(item);
    }

signals:
    void itemAdded();
};
```

That's it on the C++ side. The infrastructure handles serialization automatically.

**Supported parameter types:** `QString`, `int`, `double`, `bool`, `QJsonObject`, `QJsonArray`
**Supported return types:** `QJsonObject`, `QJsonArray`, `QString`, `int`, `double`, `bool`, `void`
**Max parameters:** 10

### 3. Define the TypeScript interface

Add the new method to `TodoBridge` — the interface that defines every method your bridge supports.

#### `web/src/api/bridge.ts`

```typescript
export interface TodoBridge {
  // ... existing methods ...
  addItem(listId: string, text: string): Promise<TodoItem>     // ← add this
}
```

### 4. Call it from React

```typescript
const todos = await useBridge<TodoBridge>('todos')
// ...
await todos.addItem(listId, 'Buy milk')
```

Done. No glue code, no method registration.

### What just happened?

You touched three files, and none of them were wiring or plumbing:

| File | What you wrote |
|------|----------------|
| `lib/todos/include/todo_store.hpp` | The actual logic |
| `lib/web-bridge/include/bridge.hpp` | Q_INVOKABLE wrapper + signal |
| `web/src/api/bridge.ts` | The TypeScript interface method |

The bridge infrastructure didn't change at all.

## Events and Signals

Push real-time updates from C++ to React. Emit a signal on the C++ side, subscribe by name on the TypeScript side.

### C++ — emit a signal

Parameterless `Q_SIGNAL`s on a bridge are automatically forwarded to connected clients. All signals (including those with parameters) are listed in `__meta__`, but only parameterless ones are auto-forwarded. Use parameterless notification signals for real-time updates:

#### `lib/web-bridge/include/bridge.hpp`

```cpp
signals:
    void dataChanged();
    void itemAdded();
```

The transport layer discovers signals automatically via `QMetaObject` introspection — no naming conventions, no registration.

### TypeScript — subscribe by signal name

```typescript
const todos = await useBridge<TodoBridge>('todos')

todos.dataChanged(() => refresh())
todos.itemAdded(() => recount())
```

### Adding a new signal

1. Add the signal to your bridge:

   #### `lib/web-bridge/include/bridge.hpp`

   ```cpp
   signals:
       void dataChanged();
       void itemAdded();  // ← new
   ```

2. Emit it where appropriate:

   ```cpp
   Q_INVOKABLE QJsonObject addItem(const QString& listId, const QString& text) {
       auto item = store_.add_item(listId.toStdString(), text.toStdString());
       emit itemAdded();
       return to_json(item);
   }
   ```

3. Add the subscription to your TypeScript interface:

   #### `web/src/api/bridge.ts`

   ```typescript
   export interface TodoBridge {
     // ...
     itemAdded(callback: () => void): () => void  // ← new
   }
   ```

4. Use it in React:

   ```typescript
   const cleanup = todos.itemAdded(() => {
     console.log('an item was added')
   })
   // later: cleanup() to unsubscribe
   ```

### Cleanup

Every signal subscription returns an unsubscribe function. Call it when your component unmounts:

```typescript
useEffect(() => {
  const cleanup = todos.itemAdded(() => setStale(true))
  return cleanup
}, [])
```

## Adding a New Bridge

Bridges let you organize your app into domains. The template ships with a `todos` bridge — here's how to add another.

### 1. Create a QObject bridge

```cpp
// lib/notes/include/notes_bridge.hpp
class NotesBridge : public QObject {
    Q_OBJECT
public:
    using QObject::QObject;

    Q_INVOKABLE QJsonArray listNotes() { /* ... */ }
    Q_INVOKABLE QJsonObject addNote(const QString& title) { /* ... */ }

signals:
    void notesChanged();
};
```

### 2. Register it in both entry points

You must register the bridge in **two** places — the desktop app and the dev server. If you only register in `main.cpp`, browser-mode dev and Playwright tests won't see your bridge.

#### `desktop/src/main.cpp`

```cpp
#include "notes_bridge.hpp"
// ...
shell->addBridge("notes", new NotesBridge);
```

#### `tests/helpers/dev-server/src/test_server.cpp`

```cpp
#include "notes_bridge.hpp"
// ...
shell.addBridge("notes", new NotesBridge);
```

Also add your new header to `add_files` in both `desktop/xmake.lua` and `tests/helpers/dev-server/xmake.lua` so Qt MOC can process it:

```lua
-- desktop/xmake.lua
add_files(
    -- ...existing files...
    path.join(os.projectdir(), "lib/notes/include/notes_bridge.hpp"),
)
```

The WebSocket and QWebChannel transports pick up the bridge automatically — no routing code needed.

### 3. Add a TypeScript interface and use it

#### `web/src/api/bridge.ts`

```typescript
export interface NotesBridge {
  listNotes(): Promise<Note[]>
  addNote(title: string): Promise<Note>
  notesChanged(callback: () => void): () => void
}
```

```typescript
const notes = await useBridge<NotesBridge>('notes')
await notes.addNote('Meeting notes')
```

## Where Does My Code Go?

| I want to... | File |
|---|---|
| Add/change business logic | `todo_store.hpp` |
| Expose a method to the UI | `bridge.hpp` (mark it `Q_INVOKABLE`) |
| Define the TypeScript API | `bridge.ts` |
| Use it in React | `todos.methodName()` |
| Push an event from C++ to JS | Signal in `bridge.hpp` + subscription in `bridge.ts` |
| Add a new domain area | Register in `main.cpp` **and** `test_server.cpp` + TypeScript interface |

**Don't forget:** new bridge classes need their `.hpp` in `add_files` for both `desktop/xmake.lua` and `tests/helpers/dev-server/xmake.lua` (Qt MOC requirement).

## How the Proxy Works (If You're Curious)

`await useBridge<T>('name')` connects to the C++ backend, queries all registered bridges and their signals via `QMetaObject`, and returns a JavaScript `Proxy` scoped to the named bridge. When you access a property on it:

1. If it's a verified signal → returns a subscribe function
2. If it's a method → sends a JSON-RPC message to C++ and returns a Promise

The TypeScript interface is the implementation. There are no method stubs, no switch statements, no registration. Add a method to the interface, add the Q_INVOKABLE on the C++ side, and the Proxy connects them automatically.
