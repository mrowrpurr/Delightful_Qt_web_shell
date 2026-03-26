import { useEffect, useState } from 'react'
import { signalReady } from '@shared/api/bridge'
import ReactMarkdown from 'react-markdown'
import remarkGfm from 'remark-gfm'
import { Select } from '@shared/components/ui/select'

// Import docs as raw strings — Vite handles this with ?raw
import readme from '../../../../README.md?raw'
import gettingStarted from '../../../../docs/for-humans/01-getting-started.md?raw'
import architecture from '../../../../docs/for-humans/02-architecture.md?raw'
import tutorial from '../../../../docs/for-humans/03-tutorial.md?raw'
import testing from '../../../../docs/for-humans/04-testing.md?raw'

const docs = [
  { id: 'readme', title: 'README', content: readme },
  { id: 'getting-started', title: 'Getting Started', content: gettingStarted },
  { id: 'architecture', title: 'Architecture', content: architecture },
  { id: 'tutorial', title: 'Tutorial', content: tutorial },
  { id: 'testing', title: 'Testing', content: testing },
]

export default function App() {
  const [selectedDoc, setSelectedDoc] = useState('readme')

  useEffect(() => { signalReady() }, [])

  const doc = docs.find(d => d.id === selectedDoc) ?? docs[0]

  return (
    <div className="docs">
      <div className="docs-header">
        <h1>{import.meta.env.VITE_APP_NAME || 'App'}</h1>
        <Select
          value={selectedDoc}
          onChange={e => setSelectedDoc(e.target.value)}
          className="doc-select"
        >
          {docs.map(d => (
            <option key={d.id} value={d.id}>{d.title}</option>
          ))}
        </Select>
      </div>
      <div className="markdown-body">
        <ReactMarkdown remarkPlugins={[remarkGfm]}>{doc.content}</ReactMarkdown>
      </div>
    </div>
  )
}
