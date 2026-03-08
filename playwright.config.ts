import { defineConfig } from '@playwright/test'

// Use C++ test server by default, fall back to Bun server.
// Set BRIDGE_SERVER=bun to use the Bun server instead.
const useBun = process.env.BRIDGE_SERVER === 'bun'

const bridgeServer = useBun
  ? { command: 'bun run test-server/server.ts', port: 9876 }
  : { command: 'xmake run test-server', port: 9876, stdout: 'pipe' as const }

export default defineConfig({
  testDir: './e2e',
  timeout: 30_000,
  use: {
    baseURL: 'http://localhost:5173',
  },
  webServer: [
    {
      command: 'bun run dev',
      cwd: './web',
      port: 5173,
      reuseExistingServer: !process.env.CI,
    },
    {
      ...bridgeServer,
      reuseExistingServer: !process.env.CI,
    },
  ],
})
