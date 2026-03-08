import { useEffect, useState, useCallback } from 'react'

declare global {
  interface Window {
    qt?: { webChannelTransport: unknown }
    QWebChannel?: new (
      transport: unknown,
      callback: (channel: { objects: Record<string, unknown> }) => void
    ) => void
  }
}

interface Bridge {
  greet(name: string, callback: (result: string) => void): void
}

function useBridge() {
  const [bridge, setBridge] = useState<Bridge | null>(null)

  useEffect(() => {
    if (!window.qt?.webChannelTransport || !window.QWebChannel) return
    new window.QWebChannel(window.qt.webChannelTransport, (channel) => {
      setBridge(channel.objects.bridge as Bridge)
    })
  }, [])

  return bridge
}

export default function App() {
  const bridge = useBridge()
  const [response, setResponse] = useState('')
  const [name, setName] = useState('World')

  const callCpp = useCallback(() => {
    if (!bridge) return
    bridge.greet(name, (result) => setResponse(result))
  }, [bridge, name])

  return (
    <div className="app">
      <h1>Qt Web Shell Example</h1>
      <p>A minimal Qt + QtWebEngine + React + QWebChannel shell.</p>

      <div className="card">
        <input
          value={name}
          onChange={(e) => setName(e.target.value)}
          placeholder="Enter your name"
          onKeyDown={(e) => e.key === 'Enter' && callCpp()}
        />
        <button onClick={callCpp} disabled={!bridge}>
          Call C++
        </button>
      </div>

      {response && <div className="response">{response}</div>}
      {!bridge && <p className="hint">QWebChannel not available (running outside Qt)</p>}
      <p className="hint">Press F12 for Developer Tools</p>
    </div>
  )
}
