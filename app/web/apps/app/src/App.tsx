import { useEffect, useState } from 'react'
import { signalReady } from '@app/bridge/lib/transport/bridge'
import { getSystemBridge } from '@app/bridge/lib/bridges/system-bridge'

// getBridge MUST be called at module scope — calling it inside a component
// creates a new instance on every render and breaks signal subscriptions.
// Top-level await is supported (target: esnext).
const systemReady = getSystemBridge().then(b => b).catch(() => null)

export default function App() {
  const [bridgeMethods, setBridgeMethods] = useState<string | null>(null)

  // signalReady MUST fire after mount — Qt's loading overlay times out after
  // 15s without it and shows an error.
  useEffect(() => { signalReady() }, [])

  useEffect(() => {
    systemReady.then(b => {
      if (b) setBridgeMethods('connected')
      else setBridgeMethods('unavailable (WASM/browser mode or bridge missing)')
    })
  }, [])

  return (
    <div style={{
      fontFamily: 'system-ui, -apple-system, sans-serif',
      padding: '2rem',
      maxWidth: 720,
      margin: '0 auto',
      color: '#e5e5e5',
      background: '#1a1a1a',
      minHeight: '100vh',
    }}>
      <h1 style={{ fontSize: '1.75rem', marginBottom: '0.5rem' }}>Your app goes here.</h1>
      <p style={{ opacity: 0.7, marginBottom: '1.5rem' }}>
        Empty slate — no UI components, no theming, no editor. Compose them in
        as needed. <code>@app/bridge</code> is wired so <code>getBridge&lt;T&gt;()</code> works.
      </p>
      <p style={{ fontSize: '0.875rem', opacity: 0.6 }}>
        SystemBridge: <strong>{bridgeMethods ?? 'connecting…'}</strong>
      </p>
    </div>
  )
}
