import { useEffect, useState } from 'react'
import { signalReady } from '@shared/api/bridge'

// The docs app is a second React app running in its own QWebEngineView.
// It shares the same bridges as the main app — same QObjects, same data.
// This proves the multi-app architecture works.

export default function App() {
  const [appName] = useState(import.meta.env.VITE_APP_NAME || 'App')

  // signalReady() tells Qt to dismiss the loading overlay.
  // Every web app must call this after mounting.
  useEffect(() => { signalReady() }, [])

  return (
    <div className="docs">
      <h1>{appName}</h1>
      <p className="subtitle">Documentation &amp; Guide</p>

      <section>
        <h2>Welcome</h2>
        <p>
          This is the <strong>docs</strong> web app — a separate Vite build running
          in its own <code>QWebEngineView</code>. It shares all bridges with the
          main app. Changes made in one view are reflected in the other via signals.
        </p>
      </section>

      <section>
        <h2>Architecture</h2>
        <p>
          Each web app lives in <code>web/apps/&lt;name&gt;/</code> with its own
          Vite config, entry point, and styles. Shared code (bridge transport,
          TypeScript interfaces) lives in <code>web/shared/</code>.
        </p>
      </section>

      <section>
        <h2>Adding a Web App</h2>
        <p>
          Copy <code>web/apps/docs/</code>, give it a new name, register it in
          the xmake build, and point a <code>WebShellWidget</code> at it.
          The scheme handler routes <code>app://&lt;name&gt;/</code> automatically.
        </p>
      </section>

      <section>
        <h2>Dev Mode</h2>
        <p>
          Each app runs its own Vite dev server on a separate port.
          Main: <code>5173</code>, Docs: <code>5174</code>.
          Both get hot module reload independently.
        </p>
      </section>
    </div>
  )
}
