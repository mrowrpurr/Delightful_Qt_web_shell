#!/usr/bin/env node
// Run a TypeScript snippet with all cdp functions available as globals.
// Reads code from stdin. All exports from index.ts are globals — no imports needed.
//
// echo 'console.log(await snapshot())' | npx tsx tools/playwright-cdp/run.ts
// echo 'await fill("new-list-input", "Groceries"); await click("create-list-button")' | npx tsx tools/playwright-cdp/run.ts

import { snapshot, screenshot, click, fill, press, eval_js, text, wait, console_messages, disconnect } from "./index.js"

// Read code from stdin
const chunks: Buffer[] = []
for await (const chunk of process.stdin) chunks.push(chunk)
const code = Buffer.concat(chunks).toString().trim()

if (!code) {
  console.error("Usage: echo '<code>' | npx tsx tools/playwright-cdp/run.ts")
  console.error("")
  console.error("  echo 'console.log(await snapshot())' | npx tsx tools/playwright-cdp/run.ts")
  process.exit(1)
}

// Make all exports available as globals for the eval'd code
const globals = { snapshot, screenshot, click, fill, press, eval_js, text, wait, console_messages, disconnect }
const fn = new Function(...Object.keys(globals), `return (async () => { ${code} })()`)

try {
  await fn(...Object.values(globals))
} finally {
  await disconnect()
}
