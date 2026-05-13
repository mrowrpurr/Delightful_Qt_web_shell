// Hash router that bypasses react-router's createBrowserURLImpl.
//
// react-router's bundled HashRouter computes a URL base by reading
// window.location.origin and falling back to .href only when origin is the
// literal string "null". Chromium serves registered-scheme origins as
// "<scheme>://" (e.g. "app://") — not "null" — so the fallback never fires,
// the base becomes "app://", and `new URL("/", "app://")` throws Invalid URL.
// That error fires inside encodeLocation on every route match, so the app
// never renders.
//
// Fix: write a hash history that conforms to the navigator contract <Router>
// expects (createHref, encodeLocation, push, replace, go, listen, location,
// action) and pass it directly to react-router's lower-level <Router>.
// Bypasses createBrowserURLImpl entirely — never calls new URL. Works under
// app:// and http://localhost (dev mode) without any environment check.

import * as React from 'react'
import { Router } from 'react-router'

type Path = { pathname: string; search: string; hash: string }
type Location = Path & { state: unknown; key: string }
type To = string | Partial<Path>

function parsePath(s: string): Path {
  const hashIdx = s.indexOf('#')
  const hash = hashIdx >= 0 ? s.slice(hashIdx) : ''
  const before = hashIdx >= 0 ? s.slice(0, hashIdx) : s
  const qIdx = before.indexOf('?')
  const search = qIdx >= 0 ? before.slice(qIdx) : ''
  const pathname = qIdx >= 0 ? before.slice(0, qIdx) : before
  return { pathname: pathname || '/', search, hash }
}

function createPath(p: Partial<Path>): string {
  return (p.pathname ?? '/') + (p.search ?? '') + (p.hash ?? '')
}

function toPath(to: To): Path {
  if (typeof to === 'string') return parsePath(to)
  return { pathname: to.pathname ?? '/', search: to.search ?? '', hash: to.hash ?? '' }
}

function readLocation(): Location {
  const raw = window.location.hash.replace(/^#/, '') || '/'
  return { ...parsePath(raw), state: null, key: 'default' }
}

function createHashHistory() {
  let action: 'POP' | 'PUSH' | 'REPLACE' = 'POP'
  let listeners: Array<(arg: { action: typeof action; location: Location }) => void> = []

  window.addEventListener('hashchange', () => {
    action = 'POP'
    const location = readLocation()
    listeners.forEach(fn => fn({ action, location }))
  })

  function notify(nextAction: typeof action) {
    action = nextAction
    const location = readLocation()
    listeners.forEach(fn => fn({ action, location }))
  }

  return {
    get action() { return action },
    get location() { return readLocation() },
    createHref(to: To) { return '#' + createPath(toPath(to)) },
    encodeLocation(to: To): Path { return toPath(to) },
    push(to: To) { window.location.hash = createPath(toPath(to)); notify('PUSH') },
    replace(to: To) {
      const path = createPath(toPath(to))
      const url = window.location.href.replace(/#.*$/, '') + '#' + path
      window.history.replaceState(null, '', url)
      notify('REPLACE')
    },
    go(n: number) { window.history.go(n) },
    listen(fn: (arg: { action: typeof action; location: Location }) => void) {
      listeners.push(fn)
      return () => { listeners = listeners.filter(f => f !== fn) }
    },
  }
}

let historySingleton: ReturnType<typeof createHashHistory> | null = null

export function HashRouter({ children, basename }: { children: React.ReactNode; basename?: string }) {
  if (!historySingleton) historySingleton = createHashHistory()
  const h = historySingleton
  const [state, setState] = React.useState({ action: h.action, location: h.location })
  React.useLayoutEffect(() => h.listen(setState), [h])
  return React.createElement(
    Router as React.ComponentType<any>,
    {
      basename,
      children,
      location: state.location,
      navigationType: state.action,
      navigator: h,
    }
  )
}
