#!/usr/bin/env bun
// Validates that TypeScript bridge interfaces match C++ Q_INVOKABLE methods.
//
// Usage:
//   xmake run validate-bridges       (starts dev-server automatically)
//   bun run tools/validate-bridges.ts (requires dev-server on :9876)
//
// Connects to the dev-server's WebSocket, calls __meta__ to get the C++ method
// manifest, then parses the TypeScript interface files and checks for drift.

import { readFileSync, readdirSync } from 'fs'
import { join } from 'path'

const WS_URL = process.env.BRIDGE_WS_URL || 'ws://localhost:9876'
const BRIDGES_DIR = join(import.meta.dir, '..', 'web', 'packages', 'bridge', 'lib', 'bridges')

// ── Fetch C++ manifest via __meta__ ─────────────────────────────────

interface MethodMeta {
  name: string
  returnType: string
  paramCount: number
  params: { name: string; type: string }[]
}

interface BridgeMeta {
  methods: MethodMeta[]
  signals: string[]
}

async function fetchMeta(): Promise<Record<string, BridgeMeta>> {
  return new Promise((resolve, reject) => {
    const ws = new WebSocket(WS_URL)
    const timeout = setTimeout(() => {
      ws.close()
      reject(new Error(`Connection timeout. Is the dev-server running on ${WS_URL}?`))
    }, 5000)

    ws.onopen = () => {
      ws.send(JSON.stringify({ method: '__meta__', args: [], id: 0 }))
    }
    ws.onmessage = (e: MessageEvent) => {
      clearTimeout(timeout)
      const msg = JSON.parse(e.data)
      ws.close()
      if (msg.error) reject(new Error(msg.error))
      else resolve(msg.result.bridges)
    }
    ws.onerror = () => {
      clearTimeout(timeout)
      reject(new Error(`Cannot connect to ${WS_URL}. Start the dev-server first:\n  xmake run dev-server`))
    }
  })
}

// ── Parse TypeScript interfaces ─────────────────────────────────────

interface TsMethod {
  name: string
  isSignal: boolean
}

interface TsBridge {
  methods: TsMethod[]
  internalSignals: Set<string>
}

function parseTsBridgeInterfaces(source: string): Record<string, TsBridge> {
  const bridges: Record<string, TsBridge> = {}

  // Find interface blocks with proper brace matching (handles nested { } in types)
  const interfaceStartRe = /export\s+interface\s+(\w+Bridge)\s*\{/g
  let startMatch
  while ((startMatch = interfaceStartRe.exec(source))) {
    const interfaceName = startMatch[1]

    // Look for @internal-signals comment before the interface
    const preceding = source.slice(Math.max(0, startMatch.index - 200), startMatch.index)
    const internalMatch = preceding.match(/@internal-signals\s+(.+)/)
    const internalSignals = new Set(
      internalMatch ? internalMatch[1].trim().split(/[\s,]+/) : []
    )

    let depth = 1
    let pos = startMatch.index + startMatch[0].length
    while (pos < source.length && depth > 0) {
      if (source[pos] === '{') depth++
      else if (source[pos] === '}') depth--
      pos++
    }
    const body = source.slice(startMatch.index + startMatch[0].length, pos - 1)
    const methods: TsMethod[] = []

    for (const line of body.split('\n')) {
      const trimmed = line.trim()
      if (!trimmed || trimmed.startsWith('//')) continue

      const methodMatch = trimmed.match(/^(\w+)\s*\(/)
      if (!methodMatch) continue

      const name = methodMatch[1]

      // Detect signals: the return type ends with "() => void" (subscription pattern)
      const isSignal = trimmed.endsWith('() => void')
        && trimmed.includes('callback:')

      methods.push({ name, isSignal })
    }

    bridges[interfaceName] = { methods, internalSignals }
  }

  return bridges
}

// ── Validation ──────────────────────────────────────────────────────

interface Issue {
  level: 'error' | 'warning'
  bridge: string
  message: string
}

function validate(
  cppBridges: Record<string, BridgeMeta>,
  tsBridges: Record<string, TsBridge>,
): Issue[] {
  const issues: Issue[] = []

  // Map TS interface names to bridge registration names
  // Convention: TodoBridge → "todos" (lowercase, strip "Bridge")
  const tsNameToCppName = new Map<string, string>()

  for (const tsName of Object.keys(tsBridges)) {
    const stripped = tsName.replace(/Bridge$/, '')
    const candidates = [
      stripped.toLowerCase(),
      stripped.toLowerCase() + 's',
      stripped,
    ]
    for (const candidate of candidates) {
      if (cppBridges[candidate]) {
        tsNameToCppName.set(tsName, candidate)
        break
      }
    }
  }

  for (const [tsName, tsBridge] of Object.entries(tsBridges)) {
    const { methods: tsMethods, internalSignals } = tsBridge
    const cppName = tsNameToCppName.get(tsName)
    if (!cppName) {
      issues.push({
        level: 'warning',
        bridge: tsName,
        message: `TypeScript interface "${tsName}" has no matching C++ bridge. Expected a bridge named "${tsName.replace(/Bridge$/, '').toLowerCase()}" or similar.`,
      })
      continue
    }

    const cpp = cppBridges[cppName]
    const cppMethodNames = new Set(cpp.methods.map(m => m.name))
    const cppSignalNames = new Set(cpp.signals)

    // Check each TS method exists in C++
    for (const tsMethod of tsMethods) {
      if (tsMethod.isSignal) {
        if (!cppSignalNames.has(tsMethod.name)) {
          issues.push({
            level: 'error',
            bridge: tsName,
            message: `Signal "${tsMethod.name}" declared in TypeScript but missing from C++ bridge "${cppName}".`,
          })
        }
        continue
      }

      if (!cppMethodNames.has(tsMethod.name)) {
        issues.push({
          level: 'error',
          bridge: tsName,
          message: `Method "${tsMethod.name}" declared in TypeScript but missing from C++ bridge "${cppName}". Did you forget to add Q_INVOKABLE?`,
        })
      }
    }

    // Check each C++ method exists in TS (skip @internal-signals)
    const tsMethodNames = new Set(tsMethods.filter(m => !m.isSignal).map(m => m.name))
    const tsSignalNames = new Set(tsMethods.filter(m => m.isSignal).map(m => m.name))

    for (const cppMethod of cpp.methods) {
      if (!tsMethodNames.has(cppMethod.name)) {
        issues.push({
          level: 'warning',
          bridge: tsName,
          message: `C++ method "${cppMethod.name}" on bridge "${cppName}" has no TypeScript declaration. Add it to ${tsName}.`,
        })
      }
    }

    for (const signal of cpp.signals) {
      if (internalSignals.has(signal)) continue
      if (!tsSignalNames.has(signal)) {
        issues.push({
          level: 'warning',
          bridge: tsName,
          message: `C++ signal "${signal}" on bridge "${cppName}" has no TypeScript subscription. Add it to ${tsName}.`,
        })
      }
    }
  }

  return issues
}

// ── Main ────────────────────────────────────────────────────────────

async function main() {
  console.log('🔍 Validating bridge interfaces...\n')

  // 1. Fetch C++ manifest
  let cppBridges: Record<string, BridgeMeta>
  try {
    cppBridges = await fetchMeta()
  } catch (e: any) {
    console.error(`❌ ${e.message}`)
    process.exit(1)
  }

  const bridgeNames = Object.keys(cppBridges)
  const totalMethods = bridgeNames.reduce((n, b) => n + cppBridges[b].methods.length, 0)
  console.log(`  C++ bridges: ${bridgeNames.join(', ')} (${totalMethods} methods)\n`)

  // 2. Parse TypeScript — read all *-bridge.ts files from @app/bridge
  const bridgeFiles = readdirSync(BRIDGES_DIR).filter(f => f.endsWith('-bridge.ts'))
  const tsSource = bridgeFiles.map(f => readFileSync(join(BRIDGES_DIR, f), 'utf8')).join('\n')
  const tsBridges = parseTsBridgeInterfaces(tsSource)
  const tsNames = Object.keys(tsBridges)
  const tsTotalMethods = tsNames.reduce((n, b) => n + tsBridges[b].methods.length, 0)
  console.log(`  TS interfaces: ${tsNames.join(', ')} (${tsTotalMethods} members)\n`)

  // 3. Validate
  const issues = validate(cppBridges, tsBridges)

  if (issues.length === 0) {
    console.log('✅ All bridge interfaces match! C++ ↔ TypeScript in sync.\n')

    // Print summary table
    for (const [cppName, meta] of Object.entries(cppBridges)) {
      console.log(`  ${cppName}:`)
      for (const m of meta.methods) {
        const params = m.params.map(p => `${p.name}: ${p.type}`).join(', ')
        console.log(`    ${m.name}(${params}) → ${m.returnType}`)
      }
      if (meta.signals.length) {
        console.log(`    signals: ${meta.signals.join(', ')}`)
      }
      console.log()
    }
    process.exit(0)
  }

  // Print issues
  const errors = issues.filter(i => i.level === 'error')
  const warnings = issues.filter(i => i.level === 'warning')

  for (const issue of errors) {
    console.log(`  ❌ [${issue.bridge}] ${issue.message}`)
  }
  for (const issue of warnings) {
    console.log(`  ⚠️  [${issue.bridge}] ${issue.message}`)
  }

  console.log(`\n  ${errors.length} error(s), ${warnings.length} warning(s)`)

  if (errors.length > 0) {
    console.log('\n  Fix: ensure every Q_INVOKABLE method in C++ has a matching')
    console.log('  declaration in the TypeScript interface, and vice versa.')
    process.exit(1)
  }
}

main()
