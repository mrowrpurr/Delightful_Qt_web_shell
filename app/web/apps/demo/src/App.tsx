import { useEffect, useRef } from 'react'
import { Routes, Route, Link, useLocation } from 'react-router'
import { SidebarSlotProvider } from '@app/ui/hooks/use-sidebar-slot'
import { signalReady } from '@app/bridge/lib/transport/bridge'
import {
  Sidebar,
  SidebarContent,
  SidebarGroup,
  SidebarHeader,
  SidebarInset,
  SidebarMenu,
  SidebarMenuButton,
  SidebarMenuItem,
  SidebarProvider,
  SidebarTrigger,
} from '@app/ui/components/sidebar'
import { Toaster } from '@app/ui/components/sonner'
import DocsTab from './tabs/DocsTab'
import TodosTab from './tabs/TodosTab'
import FileBrowserTab from './tabs/FileBrowserTab'
import SystemTab from './tabs/SystemTab'
import EditorTab from './tabs/EditorTab'
import SettingsTab from './tabs/SettingsTab'
import ComponentsTab from './tabs/ComponentsTab'
import ChatTab from './tabs/ChatTab'

const TAB_TITLES: Record<string, string> = {
  docs: '📖 Docs',
  editor: '✏️ Editor',
  todos: '✅ Todos',
  files: '📂 Files',
  chat: '💬 Chat',
  system: '⚙️ System',
  settings: '🎨 Settings',
  components: '🧩 Components',
}

const NAV_ITEMS = [
  { id: 'docs', label: 'Docs', icon: '📖' },
  { id: 'editor', label: 'Editor', icon: '✏️' },
  { id: 'todos', label: 'Todos', icon: '✅' },
  { id: 'files', label: 'Files', icon: '📂' },
  { id: 'chat', label: 'Chat', icon: '💬' },
  { id: 'system', label: 'System', icon: '⚙️' },
  { id: 'settings', label: 'Settings', icon: '🎨' },
  { id: 'components', label: 'Components', icon: '🧩' },
] as const

export default function App() {
  // Slot for page-contributed sidebar content. Pages call useSidebarSlot(<JSX/>)
  // and portal their JSX into the div whose ref we expose via context.
  const sidebarSlotRef = useRef<HTMLDivElement>(null)
  const location = useLocation()

  // Current tab derived from the URL path (HashRouter strips the leading `#`).
  // `/` and unknown paths default to docs so the doc tab is the landing page.
  const currentTab = location.pathname.replace(/^\//, '') || 'docs'

  useEffect(() => {
    console.log(`[load-time] web: App mounted, calling signalReady at ${performance.now().toFixed(1)}ms (since page nav)`)
    signalReady()
  }, [])

  // Update document.title from the active tab. The title drives the Qt dock
  // widget's tab label via the web engine's titleChanged signal.
  useEffect(() => {
    document.title = TAB_TITLES[currentTab] ?? import.meta.env.VITE_APP_NAME ?? 'App'
  }, [currentTab])

  return (
    <div className="min-h-screen text-foreground bg-page">
      <SidebarProvider defaultOpen>
        <SidebarSlotProvider value={sidebarSlotRef}>
        <Sidebar collapsible="icon">
          <SidebarHeader>
            <div className="flex items-center justify-between gap-2 group-data-[collapsible=icon]:gap-0">
              <span className="overflow-hidden whitespace-nowrap px-2 text-base font-semibold opacity-100 transition-opacity duration-150 ease-linear delay-200 group-data-[collapsible=icon]:w-0 group-data-[collapsible=icon]:px-0 group-data-[collapsible=icon]:opacity-0 group-data-[collapsible=icon]:delay-0">
                {import.meta.env.VITE_APP_NAME ?? 'App'}
              </span>
              <SidebarTrigger
                data-testid="sidebar-trigger"
                className="group-data-[collapsible=icon]:mx-auto"
              />
            </div>
          </SidebarHeader>
          <SidebarContent>
            <SidebarGroup>
              <SidebarMenu>
                {NAV_ITEMS.map(item => (
                  <SidebarMenuItem key={item.id}>
                    <SidebarMenuButton
                      asChild
                      isActive={currentTab === item.id}
                      tooltip={item.label}
                      data-testid={`sidebar-${item.id}`}
                    >
                      <Link to={`/${item.id}`}>
                        <span
                          aria-hidden
                          className="inline-flex size-4 shrink-0 items-center justify-center text-base leading-none"
                        >
                          {item.icon}
                        </span>
                        <span>{item.label}</span>
                      </Link>
                    </SidebarMenuButton>
                  </SidebarMenuItem>
                ))}
              </SidebarMenu>
            </SidebarGroup>
            <div ref={sidebarSlotRef} />
          </SidebarContent>
        </Sidebar>
        <SidebarInset>
          <Routes>
            <Route path="/" element={<DocsTab />} />
            <Route path="/docs" element={<DocsTab />} />
            <Route path="/editor" element={<EditorTab />} />
            <Route path="/todos" element={<TodosTab />} />
            <Route path="/files" element={<FileBrowserTab />} />
            <Route path="/chat" element={<ChatTab />} />
            <Route path="/system" element={<SystemTab />} />
            <Route path="/settings" element={<SettingsTab />} />
            <Route path="/components" element={<ComponentsTab />} />
            <Route path="*" element={<DocsTab />} />
          </Routes>
        </SidebarInset>
        </SidebarSlotProvider>
      </SidebarProvider>
      <Toaster />
    </div>
  )
}
