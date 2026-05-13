import { resolve } from 'path'
import { defineConfig } from 'vite'
import react from '@vitejs/plugin-react'

export default defineConfig({
  plugins: [react()],
  build: {
    target: 'esnext',
    assetsInlineLimit: 0,  // never inline assets as data URIs — QWebEngine can choke on them
  },
  server: { port: 5175 },
  resolve: {
    alias: {
      '@': resolve(__dirname, 'src'),
    },
  },
})
