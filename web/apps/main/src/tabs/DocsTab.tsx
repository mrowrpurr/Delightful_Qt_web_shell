import { useState } from 'react'
import ReactMarkdown from 'react-markdown'
import remarkGfm from 'remark-gfm'
import { Select, SelectTrigger, SelectContent, SelectItem, SelectValue } from '@shared/components/ui/select'

import readme from '../../../../../README.md?raw'
import gettingStarted from '../../../../../docs/for-humans/01-getting-started.md?raw'
import architecture from '../../../../../docs/for-humans/02-architecture.md?raw'
import tutorial from '../../../../../docs/for-humans/03-tutorial.md?raw'
import testing from '../../../../../docs/for-humans/04-testing.md?raw'

const docs = [
  { value: 'readme', label: 'README', content: readme },
  { value: 'getting-started', label: 'Getting Started', content: gettingStarted },
  { value: 'architecture', label: 'Architecture', content: architecture },
  { value: 'tutorial', label: 'Tutorial', content: tutorial },
  { value: 'testing', label: 'Testing', content: testing },
]

export default function DocsTab() {
  const [selectedDoc, setSelectedDoc] = useState('readme')
  const doc = docs.find(d => d.value === selectedDoc) ?? docs[0]

  return (
    <div className="max-w-3xl mx-auto p-6">
      <div className="flex items-center justify-between mb-6 pb-4 border-b border-border">
        <h2 className="text-lg font-semibold text-primary">Documentation</h2>
        <Select value={selectedDoc} onValueChange={setSelectedDoc}>
          <SelectTrigger className="w-[200px]">
            <SelectValue />
          </SelectTrigger>
          <SelectContent>
            {docs.map(d => (
              <SelectItem key={d.value} value={d.value}>{d.label}</SelectItem>
            ))}
          </SelectContent>
        </Select>
      </div>
      <div className="markdown-body">
        <ReactMarkdown remarkPlugins={[remarkGfm]}>{doc.content}</ReactMarkdown>
      </div>
    </div>
  )
}
