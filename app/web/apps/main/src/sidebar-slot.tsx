import { createContext, useContext, useEffect, type ReactNode } from 'react'

const SidebarSlotContext = createContext<(node: ReactNode) => void>(() => {})

export const SidebarSlotProvider = SidebarSlotContext.Provider

// Page-side API: render whatever you want into the sidebar.
//   useSidebarSlot(<SidebarGroup>...</SidebarGroup>)
// Mounts on render, clears on unmount. The page owns the JSX, the state,
// the click handlers — App.tsx just provides the slot.
export function useSidebarSlot(node: ReactNode) {
  const setSlot = useContext(SidebarSlotContext)
  useEffect(() => {
    setSlot(node)
    return () => setSlot(null)
  }, [node, setSlot])
}
