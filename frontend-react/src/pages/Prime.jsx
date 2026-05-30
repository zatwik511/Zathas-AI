import { useState, useRef, useEffect } from 'react'
import ChatMessage from '../components/ChatMessage'
import ChatInput from '../components/ChatInput'
import MemoryPanel from '../components/MemoryPanel'
import { useChat } from '../hooks/useChat'

export default function Prime() {
  const [token, setToken] = useState(() => sessionStorage.getItem('prime_token') || '')
  const [authed, setAuthed] = useState(() => !!sessionStorage.getItem('prime_token'))
  const [loginInput, setLoginInput] = useState('')
  const [panelOpen, setPanelOpen] = useState(true)
  const [doc, setDoc] = useState(null)   // { id, name } | null
  const bottomRef = useRef(null)

  function clearAuth() {
    sessionStorage.removeItem('prime_token')
    setToken('')
    setAuthed(false)
  }

  const { messages, streaming, sendMessage } = useChat(
    '/api/prime',
    token ? { Authorization: `Bearer ${token}` } : {},
    { onUnauthorized: clearAuth }
  )

  useEffect(() => {
    document.documentElement.classList.add('dark')
  }, [])

  useEffect(() => {
    bottomRef.current?.scrollIntoView({ behavior: 'smooth' })
  }, [messages])

  function handleLogin(e) {
    e.preventDefault()
    const t = loginInput.trim()
    if (!t) return
    sessionStorage.setItem('prime_token', t)
    setToken(t)
    setAuthed(true)
    setLoginInput('')
  }

  function exportChat() {
    const md = messages
      .map(m => `**${m.role === 'user' ? 'You' : 'Zathas'}:**\n\n${m.content}`)
      .join('\n\n---\n\n')
    const blob = new Blob([md], { type: 'text/markdown' })
    const url = URL.createObjectURL(blob)
    const a = document.createElement('a')
    a.href = url
    a.download = `zathas-prime-${new Date().toISOString().slice(0, 10)}.md`
    a.click()
    URL.revokeObjectURL(url)
  }

  if (!authed) {
    return (
      <div className="h-full flex items-center justify-center bg-black">
        <form onSubmit={handleLogin} className="flex gap-2">
          <input
            type="password"
            value={loginInput}
            onChange={e => setLoginInput(e.target.value)}
            autoFocus
            className="w-56 px-4 py-2.5 rounded-xl bg-zinc-900 text-white text-sm outline-none border border-zinc-800 focus:border-zinc-600 transition-colors"
          />
          <button
            type="submit"
            className="px-4 py-2.5 rounded-xl bg-zinc-900 hover:bg-zinc-800 border border-zinc-800 text-zinc-400 hover:text-zinc-200 text-sm transition-colors"
          >
            <svg className="w-4 h-4" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round">
              <path d="M5 12h14M12 5l7 7-7 7"/>
            </svg>
          </button>
        </form>
      </div>
    )
  }

  return (
    <div className="h-full flex bg-zinc-950 text-zinc-100">
      <MemoryPanel open={panelOpen} onToggle={() => setPanelOpen(false)} token={token} />

      <div className="flex-1 flex flex-col min-w-0">
        <header className="shrink-0 flex items-center justify-between px-4 py-3 border-b border-zinc-800">
          {!panelOpen && (
            <button
              onClick={() => setPanelOpen(true)}
              aria-label="Open memory panel"
              className="text-zinc-600 hover:text-zinc-400 transition-colors"
            >
              <svg className="w-4 h-4" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round">
                <rect x="3" y="3" width="18" height="18" rx="2"/>
                <line x1="9" y1="3" x2="9" y2="21"/>
              </svg>
            </button>
          )}
          {panelOpen && <div />}
          <div className="flex items-center gap-2">
            {messages.length > 0 && (
              <button
                onClick={exportChat}
                className="text-xs px-3 py-1.5 rounded-lg border border-zinc-800 hover:bg-zinc-800 text-zinc-500 hover:text-zinc-300 transition-colors"
              >
                Export
              </button>
            )}
          </div>
        </header>

        <main className="flex-1 overflow-y-auto">
          {messages.length === 0 ? (
            <div className="h-full flex items-center justify-center text-sm text-zinc-800 select-none">
              —
            </div>
          ) : (
            <div className="max-w-3xl mx-auto px-4 py-6 space-y-6">
              {messages.map((msg, i) => (
                <ChatMessage key={i} message={msg} accent="zinc" />
              ))}
              <div ref={bottomRef} />
            </div>
          )}
        </main>

        <ChatInput
          onSend={(text) => sendMessage(text, doc?.id)}
          disabled={streaming}
          accent="zinc"
          doc={doc}
          onDocChange={setDoc}
        />
      </div>
    </div>
  )
}
