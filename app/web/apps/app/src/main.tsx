import { StrictMode } from 'react'
import { createRoot } from 'react-dom/client'
import { HashRouter, Routes, Route } from 'react-router'
import App from './App'

// Empty slate: no Tailwind, no @app/ui, no @app/theming.
// Compose those packages here if you want them — the slate stays out of your
// way. Bridge transport is wired (@app/bridge) so getBridge<T>() works.

createRoot(document.getElementById('root')!).render(
  <StrictMode>
    <HashRouter>
      <Routes>
        <Route path="*" element={<App />} />
      </Routes>
    </HashRouter>
  </StrictMode>,
)
