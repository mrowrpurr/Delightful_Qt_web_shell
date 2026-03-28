/// <reference types="vite/client" />

declare module '*.md?raw' {
  const content: string
  export default content
}

interface ImportMetaEnv {
  readonly VITE_APP_NAME: string
  readonly VITE_APP_SLUG?: string
  readonly VITE_BRIDGE_WS_URL?: string
}

interface ImportMeta {
  readonly env: ImportMetaEnv
}
