import { useEffect, useState } from 'react'

export default function MemoryPanel({ open, onToggle, token }) {
  const [sessions, setSessions] = useState([])
  const [loading, setLoading] = useState(false)

  function fetchSessions() {
    if (!token) return
    setLoading(true)
    fetch('/api/prime/memory', { headers: { Authorization: `Bearer ${token}` } })
      .then(r => (r.ok ? r.json() : null))
      .then(data => { if (data?.sessions) setSessions(data.sessions) })
      .catch(() => {})
      .finally(() => setLoading(false))
  }

  useEffect(() => {
    if (open) fetchSessions()
  }, [open, token])

  if (!open) return null

  return (
    <aside className="w-56 shrink-0 flex flex-col border-r border-zinc-800 bg-zinc-950">
      <div className="flex items-center justify-between px-3 py-3 border-b border-zinc-800">
        <span className="text-xs font-medium text-zinc-500 uppercase tracking-wider">Memory</span>
        <div className="flex items-center gap-1.5">
          <button
            onClick={fetchSessions}
            aria-label="Refresh"
            className="text-zinc-600 hover:text-zinc-400 transition-colors"
          >
            <svg className="w-3.5 h-3.5" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round">
              <path d="M23 4v6h-6M1 20v-6h6"/>
              <path d="M3.51 9a9 9 0 0114.85-3.36L23 10M1 14l4.64 4.36A9 9 0 0020.49 15"/>
            </svg>
          </button>
          <button
            onClick={onToggle}
            aria-label="Close panel"
            className="text-zinc-600 hover:text-zinc-400 transition-colors"
          >
            <svg className="w-3.5 h-3.5" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round">
              <path d="M15 18l-6-6 6-6"/>
            </svg>
          </button>
        </div>
      </div>

      <div className="flex-1 overflow-y-auto py-2">
        {loading ? (
          <p className="text-xs text-zinc-600 px-4 py-2">Loading...</p>
        ) : sessions.length === 0 ? (
          <p className="text-xs text-zinc-600 px-4 py-2">No session files found</p>
        ) : (
          sessions.map((s, i) => (
            <div key={i} className="px-3 py-2 mx-1 rounded-lg hover:bg-zinc-900 transition-colors">
              <p className="text-xs text-zinc-300 font-mono truncate" title={s.file}>{s.file}</p>
              <p className="text-xs text-zinc-600 mt-0.5">
                {s.lines} lines
                {s.type === 'summary' && (
                  <span className="ml-1.5 text-zinc-700">· summary</span>
                )}
              </p>
            </div>
          ))
        )}
      </div>

      <div className="px-4 py-2.5 border-t border-zinc-800">
        <p className="text-xs text-zinc-700">
          {sessions.length} file{sessions.length !== 1 ? 's' : ''} loaded
        </p>
      </div>
    </aside>
  )
}
