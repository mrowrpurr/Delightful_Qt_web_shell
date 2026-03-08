import { defineConfig } from '@playwright/test'

// Use C++ test server by default, fall back to Bun server.
// Set BRIDGE_SERVER=bun to use the Bun server instead.
const useBun = process.env.BRIDGE_SERVER === 'bun'

const bridgeServer = useBun
  ? { command: 'bun run test-server/server.ts', port: 9876 }
  : { command: 'xmake run test-server', port: 9876, stdout: 'pipe' as const }

export default defineConfig({
  timeout: 30_000,
  projects: [
    {
      name: 'e2e',
      testDir: './e2e',
      use: { baseURL: 'http://localhost:5173' },
    },
    {
      name: 'smoke',
      testDir: './smoke',
    },
  ],
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
