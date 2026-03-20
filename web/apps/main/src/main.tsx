import { StrictMode } from 'react'
import { createRoot } from 'react-dom/client'
import App from './App'
import DialogView from './DialogView'
import './App.css'

// Hash-based routing — same React app, different content.
// The main window loads app://main/ (no hash) → full app.
// A dialog loads app://main/#/dialog → lightweight dialog UI.
// No React Router needed — the hash is set once at load time.
const route = window.location.hash

const Root = route === '#/dialog' ? DialogView : App

createRoot(document.getElementById('root')!).render(
  <StrictMode>
    <Root />
  </StrictMode>,
)
