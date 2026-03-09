# Tutorial

Your first feature in 5 minutes. We'll add a `deleteList` method — from C++ to React — and see how the pieces connect.

```
├── lib/
│   ├── todos/include/todo_store.hpp        ← C++ domain logic
│   └── web-bridge/include/bridge.hpp       ← Q_INVOKABLE wrapper
└── web/src/api/bridge.ts                   ← TypeScript interface
```

## Adding a Feature

We're adding `deleteList` — deletes a todo list and all its items. Three files, four steps.

### 1. Write the C++ logic

Add the method to `TodoStore` — your pure C++ domain logic. No Qt, no JSON, just business logic.

#### `lib/todos/include/todo_store.hpp`

```cpp
class TodoStore {
    // ... existing methods: add_list, add_item, toggle_item, etc.

    void delete_list(const std::string& list_id) {
        std::erase_if(lists_, [&](const TodoList& l) { return l.id == list_id; });
        std::erase_if(items_, [&](const TodoItem& i) { return i.list_id == list_id; });
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
    static QJsonObject to_json(const TodoList& l) {
        return {
            {"id",         QString::fromStdString(l.id)},
            {"name",       QString::fromStdString(l.name)},
            {"item_count", l.item_count},
            {"created_at", QString::fromStdString(l.created_at)},
        };
    }

public:
    // Methods that return data — call the store, return JSON:
    Q_INVOKABLE QJsonObject addList(const QString& name) {
        auto list = store_.add_list(name.toStdString());
        emit dataChanged();
        return to_json(list);
    }

    // Our new method — nothing to return, empty object:
    Q_INVOKABLE QJsonObject deleteList(const QString& listId) {
        store_.delete_list(listId.toStdString());
        emit dataChanged();
        return {};
    }

signals:
    void dataChanged();
};
```

That's it on the C++ side. Return `QJsonObject` or `QJsonArray` from your methods — the infrastructure handles serialization. No routing code, no string wrangling.

### 3. Define the TypeScript interface

Add the new method to `TodoBridge` — the interface that defines every method your bridge supports.

#### `web/src/api/bridge.ts`

```typescript
export interface TodoBridge {
  // ... existing methods ...
  deleteList(listId: string): Promise<void>     // ← add this
}
```

### 4. Call it from React

```typescript
await bridge.deleteList(listId)
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
    void listDeleted();
```

The transport layer discovers signals automatically via `QMetaObject` introspection — no naming conventions, no registration.

### TypeScript — subscribe by signal name

```typescript
bridge.dataChanged(() => refresh())
bridge.listDeleted(() => recount())
```

### Adding a new signal

1. Add the signal to Bridge:

   #### `lib/web-bridge/include/bridge.hpp`

   ```cpp
   signals:
       void dataChanged();
       void listDeleted();  // ← new
   ```

2. Emit it where appropriate:

   ```cpp
   Q_INVOKABLE QJsonObject deleteList(const QString& listId) {
       store_.delete_list(listId.toStdString());
       emit listDeleted();
       return {};
   }
   ```

3. Add the subscription to your TypeScript interface:

   #### `web/src/api/bridge.ts`

   ```typescript
   export interface TodoBridge {
     // ...
     listDeleted(callback: () => void): () => void  // ← new
   }
   ```

4. Use it in React:

   ```typescript
   const cleanup = bridge.listDeleted(() => {
     console.log('a list was deleted')
   })
   // later: cleanup() to unsubscribe
   ```

### Cleanup

Every signal subscription returns an unsubscribe function. Call it when your component unmounts:

```typescript
useEffect(() => {
  const cleanup = bridge.dataChanged(() => setStale(true))
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
