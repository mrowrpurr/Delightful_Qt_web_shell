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
- [Where Does My Code Go?](#where-does-my-code-go)
- [How the Proxy Works (If You're Curious)](#how-the-proxy-works-if-youre-curious)

---

```
├── lib/
│   ├── todos/include/todo_store.hpp        ← C++ domain logic
│   └── web-bridge/include/bridge.hpp       ← Q_INVOKABLE wrapper
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

That's it on the C++ side. Return `QJsonObject` or `QJsonArray` from your methods — the infrastructure handles serialization. No routing code, no string wrangling.

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
await bridge.addItem(listId, 'Buy milk')
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

Any parameterless `Q_SIGNAL` on Bridge is automatically forwarded to connected clients.

#### `lib/web-bridge/include/bridge.hpp`

```cpp
signals:
    void dataChanged();
    void itemAdded();
```

The transport layer discovers signals automatically via `QMetaObject` introspection — no naming conventions, no registration.

### TypeScript — subscribe by signal name

```typescript
bridge.dataChanged(() => refresh())
bridge.itemAdded(() => recount())
```

### Adding a new signal

1. Add the signal to Bridge:

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
   const cleanup = bridge.itemAdded(() => {
     console.log('an item was added')
   })
   // later: cleanup() to unsubscribe
   ```

### Cleanup

Every signal subscription returns an unsubscribe function. Call it when your component unmounts:

```typescript
useEffect(() => {
  const cleanup = bridge.itemAdded(() => setStale(true))
  return cleanup
}, [])
```

## Where Does My Code Go?

| I want to... | File |
|---|---|
| Add/change business logic | `todo_store.hpp` |
| Expose a method to the UI | `bridge.hpp` |
| Define the TypeScript API | `bridge.ts` |
| Use it in React | `bridge.methodName()` |
| Push an event from C++ to JS | Signal in `bridge.hpp` + subscription in `bridge.ts` |

## How the Proxy Works (If You're Curious)

`await createBridge()` connects to the C++ backend, queries its signals via `QMetaObject`, and returns a JavaScript `Proxy`. When you access a property on it:

1. If it's a verified signal → returns a subscribe function
2. If it's a method → sends a JSON-RPC message to C++ and returns a Promise

The TypeScript interface is the implementation. There are no method stubs, no switch statements, no registration. Add a method to the interface, add the Q_INVOKABLE on the C++ side, and the Proxy connects them automatically.
