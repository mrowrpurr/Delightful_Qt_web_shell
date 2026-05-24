#!/usr/bin/env bun
// Generates TypeScript DTO interfaces from C++ def_type structs.
//
// Usage:
//   xmake run generate-dtos
//   bun run tools/generate-dtos.ts
//
// Scans all .hpp files containing `#include <def_type.hpp>` under:
//   - app/bridges/*/include/
//   - lib/*/include/
//   - app/framework/bridge/ (for OkResponse)
//
// Writes generated TS interfaces to:
//   web/packages/bridge/lib/dtos/<slug>.dtos.ts
//
// Idempotent — safe to rerun after any C++ DTO change.

import { readFileSync, writeFileSync, readdirSync, existsSync, mkdirSync } from 'fs'
import { join, basename, dirname } from 'path'
import { globSync } from 'fs'

const ROOT = join(import.meta.dir, '..')
const LIB_ROOT = join(ROOT, '..', 'lib')
const DTOS_OUT = join(ROOT, 'web', 'packages', 'bridge', 'lib', 'dtos')

// ── Discover all def_type headers ────────────────────────────────────

interface SourceFile {
  path: string
  slug: string // bridge/domain name: "system", "theme", "todos"
}

function discoverDefTypeHeaders(): SourceFile[] {
  const files: SourceFile[] = []

  // app/bridges/*/include/*.hpp
  const bridgesDir = join(ROOT, 'bridges')
  if (existsSync(bridgesDir)) {
    for (const slug of readdirSync(bridgesDir)) {
      const includeDir = join(bridgesDir, slug, 'include')
      if (!existsSync(includeDir)) continue
      for (const f of readdirSync(includeDir).filter(f => f.endsWith('.hpp'))) {
        const fullPath = join(includeDir, f)
        const content = readFileSync(fullPath, 'utf8')
        if (content.includes('#include <def_type.hpp>')) {
          files.push({ path: fullPath, slug })
        }
      }
    }
  }

  // lib/*/include/*.hpp
  if (existsSync(LIB_ROOT)) {
    for (const slug of readdirSync(LIB_ROOT)) {
      const includeDir = join(LIB_ROOT, slug, 'include')
      if (!existsSync(includeDir)) continue
      for (const f of readdirSync(includeDir).filter(f => f.endsWith('.hpp'))) {
        const fullPath = join(includeDir, f)
        const content = readFileSync(fullPath, 'utf8')
        if (content.includes('#include <def_type.hpp>')) {
          files.push({ path: fullPath, slug })
        }
      }
    }
  }

  // app/framework/bridge/bridge.hpp (for OkResponse)
  const bridgeHpp = join(ROOT, 'framework', 'bridge', 'bridge.hpp')
  if (existsSync(bridgeHpp)) {
    const content = readFileSync(bridgeHpp, 'utf8')
    if (content.includes('#include <def_type.hpp>')) {
      files.push({ path: bridgeHpp, slug: '_framework' })
    }
  }

  return files
}

// ── Parse C++ structs ────────────────────────────────────────────────

interface CppField {
  name: string
  cppType: string
  hasDefault: boolean
}

interface CppStruct {
  name: string
  fields: CppField[]
  sourceFile: string
}

function parseCppStructs(source: string, sourceFile: string): CppStruct[] {
  const structs: CppStruct[] = []

  // Match top-level structs only (not indented — skips private structs inside classes)
  const structRe = /^struct\s+(\w+)\s*\{([^}]*)\}/gm
  let match
  while ((match = structRe.exec(source))) {
    const name = match[1]
    const body = match[2]
    const fields: CppField[] = []

    // Split on semicolons — handles both one-liner and multi-line structs
    for (const chunk of body.split(';')) {
      const trimmed = chunk.trim()
      if (!trimmed || trimmed.startsWith('//')) continue

      // Parse: type name = default  or  type name
      const fieldMatch = trimmed.match(/^(.+?)\s+(\w+)\s*(?:=\s*(.+))?$/)
      if (!fieldMatch) continue

      const cppType = fieldMatch[1].trim()
      const fieldName = fieldMatch[2]
      const hasDefault = !!fieldMatch[3]

      fields.push({ name: fieldName, cppType, hasDefault })
    }

    structs.push({ name, fields, sourceFile })
  }

  return structs
}

// ── C++ → TypeScript type mapping ────────────────────────────────────

function cppTypeToTs(cppType: string, knownStructs: Set<string>): string {
  // Strip qualifiers
  const clean = cppType.replace(/\bconst\b/g, '').trim()

  if (clean === 'std::string' || clean === 'string') return 'string'
  if (clean === 'bool') return 'boolean'
  if (clean === 'int' || clean === 'int32_t' || clean === 'int64_t' ||
      clean === 'uint32_t' || clean === 'uint64_t' ||
      clean === 'float' || clean === 'double' || clean === 'size_t') return 'number'

  // std::vector<T>
  const vecMatch = clean.match(/^std::vector<(.+)>$/)
  if (vecMatch) {
    const inner = cppTypeToTs(vecMatch[1].trim(), knownStructs)
    return `${inner}[]`
  }

  // Known struct reference
  if (knownStructs.has(clean)) return clean

  // Fallback
  return 'unknown'
}

// ── Generate TypeScript ──────────────────────────────────────────────

function generateTs(structs: CppStruct[], knownStructs: Set<string>): string {
  const lines: string[] = [
    '// Auto-generated from C++ def_type structs — do not edit.',
    '// Regenerate: xmake run generate-dtos',
    '',
  ]

  for (const s of structs) {
    lines.push(`export interface ${s.name} {`)
    for (const f of s.fields) {
      const tsType = cppTypeToTs(f.cppType, knownStructs)
      lines.push(`  ${f.name}: ${tsType}`)
    }
    lines.push('}')
    lines.push('')
  }

  return lines.join('\n')
}

// ── Main ────────────────────────────────────────────────────────────

function main() {
  console.log('🔍 Scanning for def_type headers...\n')

  const headers = discoverDefTypeHeaders()
  if (headers.length === 0) {
    console.log('  No def_type headers found.')
    return
  }

  // Parse all structs from all files
  const allStructs: CppStruct[] = []
  for (const header of headers) {
    const source = readFileSync(header.path, 'utf8')
    const structs = parseCppStructs(source, header.path)
    allStructs.push(...structs)
  }

  const knownStructs = new Set(allStructs.map(s => s.name))

  // Group by slug
  const bySlug = new Map<string, CppStruct[]>()
  for (const header of headers) {
    const source = readFileSync(header.path, 'utf8')
    const structs = parseCppStructs(source, basename(header.path))
    const existing = bySlug.get(header.slug) || []
    existing.push(...structs)
    bySlug.set(header.slug, existing)
  }

  // Merge _framework structs into every other slug's known set
  // (OkResponse is available to all bridges)
  const frameworkStructs = bySlug.get('_framework') || []
  bySlug.delete('_framework')

  // Write output files
  mkdirSync(DTOS_OUT, { recursive: true })

  let totalStructs = 0
  let totalFiles = 0

  // Write framework dtos (shared across all bridges)
  if (frameworkStructs.length > 0) {
    const ts = generateTs(frameworkStructs, knownStructs)
    const outPath = join(DTOS_OUT, 'framework.dtos.ts')
    writeFileSync(outPath, ts)
    totalFiles++
    totalStructs += frameworkStructs.length
    console.log(`  framework.dtos.ts — ${frameworkStructs.length} types (${frameworkStructs.map(s => s.name).join(', ')})`)
  }

  for (const [slug, structs] of bySlug) {
    const ts = generateTs(structs, knownStructs)
    const outPath = join(DTOS_OUT, `${slug}.dtos.ts`)
    writeFileSync(outPath, ts)
    totalFiles++
    totalStructs += structs.length
    console.log(`  ${slug}.dtos.ts — ${structs.length} types (${structs.map(s => s.name).join(', ')})`)
  }

  console.log(`\n✅ Generated ${totalStructs} types across ${totalFiles} files → web/packages/bridge/lib/dtos/\n`)
}

main()
