#!/usr/bin/env node
// CLI for driving the Qt app via Chrome DevTools Protocol.
//
// Usage: npx tsx tools/playwright-cdp/cli.ts <command> [args...]
// For programmatic use: import { snapshot, click, fill } from './tools/playwright-cdp'

import { snapshot, screenshot, click, fill, press, eval_js, text, wait, console_messages, disconnect } from "./index.js"

function parseArgs(argv: string[]) {
  const args = argv.slice(2)
  const command = args[0]
  const positional: string[] = []
  const flags: Record<string, string> = {}
  for (let i = 1; i < args.length; i++) {
    if (args[i].startsWith("--")) {
      const key = args[i].slice(2)
      const next = args[i + 1]
      if (next && !next.startsWith("--")) { flags[key] = next; i++ }
      else { flags[key] = "true" }
    } else {
      positional.push(args[i])
    }
  }
  return { command, positional, flags }
}

function printHelp() {
  console.log(`cdp — Drive the Qt app via Chrome DevTools Protocol

Usage: npx tsx tools/playwright-cdp/cli.ts <command> [args...]

Commands:
  snapshot                          Accessibility tree of the page
  screenshot [path]                 Save screenshot (default: screenshot.png)
  click --test-id <id>              Click element by test ID
  click --selector <sel>            Click element by CSS selector
  fill --test-id <id> <value>       Fill input with text
  press <key>                       Press key globally
  press <key> --test-id <id>        Press key on element
  eval <expression>                 Evaluate JavaScript in page context
  text --test-id <id>               Get text content of element
  wait --test-id <id>               Wait for element to appear
  console                           Listen for console messages (3s)
  console --duration <ms>           Listen for specified duration

Programmatic:
  npx tsx -e "import { snapshot } from './tools/playwright-cdp'; console.log(await snapshot())"

Options:
  --help                            Show this help`)
}

async function main() {
  const { command, positional, flags } = parseArgs(process.argv)

  if (!command || command === "--help" || command === "-h" || flags["help"]) {
    printHelp()
    process.exit(0)
  }

  try {
    switch (command) {
      case "snapshot":
        console.log(await snapshot())
        break
      case "screenshot":
        console.log(await screenshot(positional[0]))
        break
      case "click":
        await click(flags["test-id"], { selector: flags["selector"] })
        break
      case "fill":
        await fill(flags["test-id"], positional[0])
        break
      case "press":
        await press(positional[0], flags["test-id"], { selector: flags["selector"] })
        break
      case "eval":
        console.log(JSON.stringify(await eval_js(positional[0]), null, 2))
        break
      case "text":
        console.log(await text(flags["test-id"], { selector: flags["selector"] }))
        break
      case "wait":
        await wait(flags["test-id"], flags["timeout"] ? parseInt(flags["timeout"]) : 5000, { selector: flags["selector"] })
        console.log("Visible")
        break
      case "console": {
        const msgs = await console_messages({
          level: flags["level"],
          count: flags["count"] ? parseInt(flags["count"]) : undefined,
          clear: flags["clear"] === "true",
        })
        console.log(msgs.length ? JSON.stringify(msgs, null, 2) : "(no console messages)")
        break
      }
      default:
        console.error(`Unknown command: ${command}`)
        process.exit(1)
    }
  } catch (err: any) {
    console.error(`cdp error: ${err.message}`)
    process.exit(1)
  } finally {
    await disconnect()
  }
}

main()
