import { Card, CardContent, CardDescription, CardHeader, CardTitle } from '@shared/components/ui/card'

export default function ComponentsTab() {
  return (
    <div className="max-w-3xl mx-auto p-6 space-y-4">
      <div>
        <h2 className="text-2xl font-bold mb-2">🧩 Components</h2>
        <p className="text-muted-foreground">
          Every installed shadcn primitive, live against the current theme.
        </p>
      </div>
      <Card>
        <CardHeader>
          <CardTitle>Coming soon</CardTitle>
          <CardDescription>
            This page will render every installed shadcn component in realistic usage
            against the live theme — so theme authors can spot at a glance when a theme
            breaks any primitive across the catalog.
          </CardDescription>
        </CardHeader>
        <CardContent className="text-sm text-muted-foreground">
          Populated in Phase 3.
        </CardContent>
      </Card>
    </div>
  )
}
