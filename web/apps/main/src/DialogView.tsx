import { useEffect, useState, useCallback } from 'react'
import { getBridge, signalReady, type TodoBridge, type TodoList } from '@shared/api/bridge'

// Top-level await — same pattern as App.tsx.
const todos = await getBridge<TodoBridge>('todos')

// DialogView — a lightweight UI rendered when the hash is #/dialog.
//
// This proves the "same app, different route" pattern: the main window
// renders App, dialogs render this. Same bridges, same build, different UI.
// Add a todo in the dialog → the main window updates via dataChanged signal.

export default function DialogView() {
  const [lists, setLists] = useState<TodoList[]>([])
  const [selectedListId, setSelectedListId] = useState<string | null>(null)
  const [itemText, setItemText] = useState('')
  const [feedback, setFeedback] = useState('')

  const loadLists = useCallback(async () => {
    const result = await todos.listLists()
    setLists(result)
    // Auto-select first list if none selected
    if (result.length > 0 && !selectedListId) {
      setSelectedListId(result[0].id)
    }
  }, [selectedListId])

  // signalReady() — every view must call this to dismiss the loading overlay.
  useEffect(() => { signalReady() }, [])

  // Load lists and subscribe to changes
  useEffect(() => {
    loadLists()
    return todos.dataChanged(() => loadLists())
  }, [loadLists])

  const handleAdd = useCallback(async () => {
    const text = itemText.trim()
    if (!text || !selectedListId) return
    await todos.addItem(selectedListId, text).catch(console.error)
    setItemText('')
    setFeedback('Added!')
    setTimeout(() => setFeedback(''), 1500)
  }, [itemText, selectedListId])

  return (
    <div className="dialog-view">
      <h2>Quick Add Todo</h2>
      <p className="hint">Add an item — it appears in the main window instantly.</p>

      {lists.length === 0 ? (
        <p className="hint">No lists yet. Create one in the main window first.</p>
      ) : (
        <>
          <select
            data-testid="dialog-list-select"
            value={selectedListId ?? ''}
            onChange={e => setSelectedListId(e.target.value)}
          >
            {lists.map(list => (
              <option key={list.id} value={list.id}>
                {list.name} ({list.item_count} items)
              </option>
            ))}
          </select>

          <div className="dialog-input">
            <input
              data-testid="dialog-item-input"
              placeholder="New todo item"
              value={itemText}
              onChange={e => setItemText(e.target.value)}
              onKeyDown={e => e.key === 'Enter' && handleAdd()}
              autoFocus
            />
            <button data-testid="dialog-add-button" onClick={handleAdd}>
              Add
            </button>
          </div>

          {feedback && <span className="feedback">{feedback}</span>}
        </>
      )}
    </div>
  )
}
