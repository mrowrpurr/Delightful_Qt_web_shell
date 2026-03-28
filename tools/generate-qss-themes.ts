#!/usr/bin/env bun
/**
 * Generates QSS themes from the same themes.json used by the web side.
 *
 * For each theme, generates:
 *   desktop/styles/compiled/<slug>-dark.qss
 *   desktop/styles/compiled/<slug>-light.qss
 *
 * Uses desktop/styles/shared/widgets.qss.template as the widget map,
 * replacing $variable references with actual color values.
 *
 * Run:  bun run tools/generate-qss-themes.ts
 */

import { readFileSync, writeFileSync, mkdirSync, readdirSync } from 'fs'
import { join, resolve } from 'path'

const ROOT = resolve(import.meta.dir, '..')
const THEMES_JSON = join(ROOT, 'web/apps/main/src/data/themes.json')
const TEMPLATE_PATH = join(ROOT, 'desktop/styles/shared/widgets.qss.template')
const COMPILED_DIR = join(ROOT, 'desktop/styles/compiled')

// ── Types ─────────────────────────────────────────────────────────

interface ThemeEntry {
  name: string
  source: string
  light: Record<string, string>
  dark: Record<string, string>
}

// ── Helpers ───────────────────────────────────────────────────────

function slugify(name: string): string {
  return name
    .toLowerCase()
    .replace(/['']/g, '')
    .replace(/[^a-z0-9]+/g, '-')
    .replace(/^-+|-+$/g, '')
}

/**
 * Convert HSL string to QSS-compatible format.
 * QSS doesn't support modern space-separated HSL: hsl(310 100% 64%)
 * Must be comma-separated: hsl(310, 100%, 64%)
 */
function toQssColor(value: string): string {
  if (!value) return 'transparent'
  if (value.startsWith('#')) return value

  const hslMatch = value.match(/hsl\(\s*([\d.]+)\s+([\d.]+)%\s+([\d.]+)%\s*\)/)
  if (hslMatch) {
    return `hsl(${hslMatch[1]}, ${hslMatch[2]}%, ${hslMatch[3]}%)`
  }

  return value
}

// The variables used in the template
const THEME_VARS = [
  'background', 'foreground',
  'card', 'card-foreground',
  'popover', 'popover-foreground',
  'primary', 'primary-foreground',
  'secondary', 'secondary-foreground',
  'muted', 'muted-foreground',
  'accent', 'accent-foreground',
  'destructive', 'destructive-foreground',
  'border', 'input', 'ring',
]

const FALLBACK_DARK: Record<string, string> = {
  'background': '#242424', 'foreground': '#e0e0e0',
  'card': '#2a2a2a', 'card-foreground': '#e0e0e0',
  'popover': '#2a2a2a', 'popover-foreground': '#e0e0e0',
  'primary': '#7c6ef0', 'primary-foreground': '#ffffff',
  'secondary': '#3a3a3a', 'secondary-foreground': '#e0e0e0',
  'muted': '#2a2a2a', 'muted-foreground': '#888888',
  'accent': '#3a3a4a', 'accent-foreground': '#e0e0e0',
  'destructive': '#ef4444', 'destructive-foreground': '#ffffff',
  'border': '#3a3a3a', 'input': '#3a3a3a', 'ring': '#7c6ef0',
}

const FALLBACK_LIGHT: Record<string, string> = {
  'background': '#ffffff', 'foreground': '#0a0a0a',
  'card': '#ffffff', 'card-foreground': '#0a0a0a',
  'popover': '#ffffff', 'popover-foreground': '#0a0a0a',
  'primary': '#7c6ef0', 'primary-foreground': '#ffffff',
  'secondary': '#f4f4f5', 'secondary-foreground': '#0a0a0a',
  'muted': '#f4f4f5', 'muted-foreground': '#71717a',
  'accent': '#f4f4f5', 'accent-foreground': '#0a0a0a',
  'destructive': '#ef4444', 'destructive-foreground': '#ffffff',
  'border': '#e4e4e7', 'input': '#e4e4e7', 'ring': '#7c6ef0',
}

// ── Generate QSS ──────────────────────────────────────────────────

function generateQss(template: string, theme: ThemeEntry, mode: 'dark' | 'light'): string {
  const vars = mode === 'dark' ? theme.dark : theme.light
  const fallback = mode === 'dark' ? FALLBACK_DARK : FALLBACK_LIGHT
  const hasVars = Object.keys(vars).length > 0

  let qss = `/* Auto-generated — ${theme.name} (${mode}) */\n\n` + template

  // Replace $variable references with actual colors.
  // Sort by longest name first so $card-foreground matches before $card.
  const sortedVars = [...THEME_VARS].sort((a, b) => b.length - a.length)

  for (const name of sortedVars) {
    const raw = hasVars ? (vars[`--${name}`] || fallback[name]) : fallback[name]
    const color = toQssColor(raw)
    // Replace $name that's followed by a non-word char (or end of line)
    const scssVar = `$${name}`
    // Use global string replace — escape the $ for regex
    const regex = new RegExp(`\\$${name.replace(/-/g, '\\-')}(?=[^a-zA-Z0-9-]|$)`, 'g')
    qss = qss.replace(regex, color)
  }

  // Strip SCSS-style // comments (QSS doesn't support them)
  qss = qss.replace(/^\s*\/\/.*$/gm, '')
  // Clean up multiple blank lines
  qss = qss.replace(/\n{3,}/g, '\n\n')

  return qss.trim() + '\n'
}

// ── Main ──────────────────────────────────────────────────────────

console.log('Reading themes.json...')
const themes: ThemeEntry[] = JSON.parse(readFileSync(THEMES_JSON, 'utf8'))

console.log('Reading widget template...')
const template = readFileSync(TEMPLATE_PATH, 'utf8')

mkdirSync(COMPILED_DIR, { recursive: true })

let generated = 0
let skipped = 0

for (const theme of themes) {
  const slug = slugify(theme.name)
  if (!slug) { skipped++; continue }

  for (const mode of ['dark', 'light'] as const) {
    const qss = generateQss(template, theme, mode)
    const qssPath = join(COMPILED_DIR, `${slug}-${mode}.qss`)
    writeFileSync(qssPath, qss)
  }
  generated++
}

console.log(`\n✅ Generated ${generated * 2} QSS files in desktop/styles/compiled/`)
if (skipped > 0) console.log(`   (${skipped} themes skipped — empty name)`)
