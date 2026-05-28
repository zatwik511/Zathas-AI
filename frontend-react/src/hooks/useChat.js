import { useState } from 'react'

export function useChat(endpoint, extraHeaders = {}, { onUnauthorized } = {}) {
  const [messages, setMessages] = useState([])
  const [streaming, setStreaming] = useState(false)

  async function sendMessage(text) {
    const history = messages.map(({ role, content }) => ({ role, content }))

    setMessages(prev => [
      ...prev,
      { role: 'user', content: text },
      { role: 'assistant', content: '', streaming: true }
    ])
    setStreaming(true)

    try {
      const res = await fetch(endpoint, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json', ...extraHeaders },
        body: JSON.stringify({ message: text, history })
      })

      if (res.status === 401) {
        setMessages([])
        setStreaming(false)
        onUnauthorized?.()
        return
      }

      if (!res.ok) throw new Error(`HTTP ${res.status}`)

      const reader = res.body.getReader()
      const decoder = new TextDecoder()
      let buf = ''

      while (true) {
        const { done, value } = await reader.read()
        if (done) break

        buf += decoder.decode(value, { stream: true })

        let idx
        while ((idx = buf.indexOf('\n\n')) !== -1) {
          const line = buf.slice(0, idx)
          buf = buf.slice(idx + 2)
          if (!line.startsWith('data: ')) continue
          try {
            const data = JSON.parse(line.slice(6))
            if (data.token) {
              setMessages(prev => {
                const next = [...prev]
                const last = next[next.length - 1]
                next[next.length - 1] = { ...last, content: last.content + data.token }
                return next
              })
            }
            if (data.done) {
              setMessages(prev => {
                const next = [...prev]
                next[next.length - 1] = { ...next[next.length - 1], streaming: false }
                return next
              })
            }
            if (data.error) {
              setMessages(prev => {
                const next = [...prev]
                next[next.length - 1] = {
                  ...next[next.length - 1],
                  streaming: false,
                  content: `Error: ${data.error}`
                }
                return next
              })
            }
          } catch { /* skip malformed SSE event */ }
        }
      }
    } catch {
      setMessages(prev => {
        const next = [...prev]
        next[next.length - 1] = {
          ...next[next.length - 1],
          streaming: false,
          content: 'Connection error. Is the server running?'
        }
        return next
      })
    } finally {
      setStreaming(false)
      setMessages(prev => {
        const next = [...prev]
        if (next[next.length - 1]?.streaming) {
          next[next.length - 1] = { ...next[next.length - 1], streaming: false }
        }
        return next
      })
    }
  }

  return { messages, streaming, sendMessage }
}
