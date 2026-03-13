# Adding Features

## Adding a Method to an Existing Bridge

Three files. No wiring.

### 1. C++ domain logic

`lib/todos/include/todo_store.hpp` — pure C++, no Qt:

```cpp
TodoItem add_item(const std::string& list_id, const std::string& text) {
    TodoItem item{gen_id(), list_id, text, false, now_iso()};
    items_.push_back(item);
    return item;
}
```

### 2. Bridge wrapper

`lib/web-bridge/include/bridge.hpp` — mark it `Q_INVOKABLE`:

```cpp
Q_INVOKABLE QJsonObject addItem(const QString& listId, const QString& text) {
    auto item = store_.add_item(listId.toStdString(), text.toStdString());
    emit dataChanged();
    return to_json(item);
}
```

### 3. TypeScript interface

`web/src/api/bridge.ts`:

```typescript
export interface TodoBridge {
  // ... existing methods ...
  addItem(listId: string, text: string): Promise<TodoItem>
}
```

### Use it

```typescript
const todos = await getBridge<TodoBridge>('todos')
await todos.addItem(listId, 'Buy milk')
```

Done. The proxy connects them automatically.

---

## Adding a New Bridge

When you need a new domain area (not just a new method on `todos`).

### Step 1: Create the C++ bridge

```cpp
// lib/notes/include/notes_bridge.hpp
#pragma once
#include <QJsonArray>
#include <QJsonObject>
#include <QObject>
#include <QString>

class NotesBridge : public QObject {
    Q_OBJECT
public:
    using QObject::QObject;

    Q_INVOKABLE QJsonArray listNotes() const { /* ... */ }
    Q_INVOKABLE QJsonObject addNote(const QString& title) { /* ... */ }

signals:
    void notesChanged();
};
```

### Step 2: Add the .hpp to xmake build files

**This is the step agents forget.** Qt's MOC (Meta-Object Compiler) needs to process your header. If you skip this, you'll get a cryptic vtable linker error.

Add your header to `add_files()` in **both**:

**`desktop/xmake.lua`:**
```lua
add_files(
    -- ...existing files...
    path.join(os.projectdir(), "lib/notes/include/notes_bridge.hpp"),
)
```

**`tests/helpers/dev-server/xmake.lua`:**
```lua
add_files(
    -- ...existing files...
    path.join(os.projectdir(), "lib/notes/include/notes_bridge.hpp"),
)
```

### Step 3: Register in both entry points

**`desktop/src/main.cpp`:**
```cpp
#include "notes_bridge.hpp"
// ...
auto* notes = new NotesBridge;
shell->addBridge("notes", notes);
```

**`tests/helpers/dev-server/src/test_server.cpp`:**
```cpp
#include "notes_bridge.hpp"
// ...
auto* notes = new NotesBridge;
shell.addBridge("notes", notes);
```

If you only register in `main.cpp`, browser-mode dev and Playwright tests won't see your bridge. No error — it just silently won't exist.

### Step 4: TypeScript interface

`web/src/api/bridge.ts`:

```typescript
export interface NotesBridge {
  listNotes(): Promise<Note[]>
  addNote(title: string): Promise<Note>
  notesChanged(callback: () => void): () => void
}
```

### Step 5: Use it

```typescript
const notes = await getBridge<NotesBridge>('notes')
await notes.addNote('Meeting notes')
```

### Checklist

- [ ] C++ header with `Q_OBJECT` + `Q_INVOKABLE` methods
- [ ] Header in `add_files()` in `desktop/xmake.lua`
- [ ] Header in `add_files()` in `tests/helpers/dev-server/xmake.lua`
- [ ] `#include` + `addBridge()` in `desktop/src/main.cpp`
- [ ] `#include` + `addBridge()` in `tests/helpers/dev-server/src/test_server.cpp`
- [ ] TypeScript interface in `web/src/api/bridge.ts`
- [ ] Run `xmake run validate-bridges` to verify C++ and TS match

---

## Signals (C++ → JavaScript Events)

Push real-time updates from C++ to React.

### Emit from C++

Add a parameterless signal and emit it:

```cpp
// bridge.hpp
signals:
    void dataChanged();

// In a method:
Q_INVOKABLE QJsonObject addItem(...) {
    // ...
    emit dataChanged();
    return result;
}
```

Parameterless signals are auto-forwarded to connected clients. No registration needed.

### Subscribe in TypeScript

Add to your interface:
```typescript
export interface TodoBridge {
  dataChanged(callback: () => void): () => void
}
```

Use it:
```typescript
const todos = await getBridge<TodoBridge>('todos')
const cleanup = todos.dataChanged(() => {
  console.log('data changed, refreshing...')
  refresh()
})

// Later: cleanup() to unsubscribe
```

### In React

```typescript
useEffect(() => {
  const cleanup = todos.dataChanged(() => setStale(true))
  return cleanup
}, [])
```

---

## Validate Your Work

```bash
xmake run validate-bridges   # checks TS interfaces match C++ methods
xmake run test-all            # run all tests
```

The bridge validator catches drift between C++ and TypeScript at dev time — before you find out at runtime.
