import * as React from 'react'
import { cn } from '@shared/lib/utils'
import { Check, ChevronDown } from 'lucide-react'

interface SelectOption {
  value: string
  label: string
}

interface SelectProps {
  value: string
  onChange: (value: string) => void
  options: SelectOption[]
  placeholder?: string
  className?: string
}

function Select({ value, onChange, options, placeholder, className }: SelectProps) {
  const [open, setOpen] = React.useState(false)
  const ref = React.useRef<HTMLDivElement>(null)

  const selected = options.find(o => o.value === value)

  React.useEffect(() => {
    function handleClick(e: MouseEvent) {
      if (ref.current && !ref.current.contains(e.target as Node)) setOpen(false)
    }
    document.addEventListener('mousedown', handleClick)
    return () => document.removeEventListener('mousedown', handleClick)
  }, [])

  return (
    <div ref={ref} className={cn('relative', className)}>
      <button
        type="button"
        onClick={() => setOpen(!open)}
        className="flex h-9 w-full items-center justify-between gap-2 rounded-md border border-border bg-card px-3 py-2 text-sm shadow-sm ring-offset-background transition-colors hover:bg-accent/50 focus:outline-none focus:ring-1 focus:ring-ring cursor-pointer"
      >
        <span className={cn('truncate', selected ? 'text-foreground' : 'text-muted-foreground')}>
          {selected?.label ?? placeholder ?? 'Select...'}
        </span>
        <ChevronDown className={cn(
          'h-4 w-4 shrink-0 text-muted-foreground transition-transform duration-200',
          open && 'rotate-180'
        )} />
      </button>
      {open && (
        <div className="absolute z-50 mt-1 w-full overflow-hidden rounded-md border border-border bg-card shadow-lg animate-in fade-in-0 zoom-in-95">
          <div className="p-1">
            {options.map(option => {
              const isSelected = option.value === value
              return (
                <button
                  key={option.value}
                  type="button"
                  onClick={() => { onChange(option.value); setOpen(false) }}
                  className={cn(
                    'relative flex w-full items-center rounded-sm px-2 py-1.5 pl-8 text-sm outline-none cursor-pointer transition-colors',
                    isSelected
                      ? 'bg-primary/15 text-foreground'
                      : 'text-muted-foreground hover:bg-accent hover:text-accent-foreground',
                  )}
                >
                  {isSelected && (
                    <span className="absolute left-2 flex h-3.5 w-3.5 items-center justify-center">
                      <Check className="h-4 w-4 text-primary" />
                    </span>
                  )}
                  {option.label}
                </button>
              )
            })}
          </div>
        </div>
      )}
    </div>
  )
}

export { Select }
export type { SelectOption, SelectProps }
