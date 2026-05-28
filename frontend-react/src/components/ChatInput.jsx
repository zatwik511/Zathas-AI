import { useState, useRef, useEffect } from 'react'

export default function ChatInput({ onSend, disabled, accent = 'blue' }) {
  const [value, setValue] = useState('')
  const ref = useRef(null)

  useEffect(() => {
    if (!disabled) ref.current?.focus()
  }, [disabled])

  function handleChange(e) {
    setValue(e.target.value)
    e.target.style.height = 'auto'
    e.target.style.height = Math.min(e.target.scrollHeight, 192) + 'px'
  }

  function handleKeyDown(e) {
    if (e.key === 'Enter' && !e.shiftKey) {
      e.preventDefault()
      submit()
    }
  }

  function submit() {
    const text = value.trim()
    if (!text || disabled) return
    onSend(text)
    setValue('')
    if (ref.current) ref.current.style.height = 'auto'
  }

  return (
    <div className="shrink-0 border-t border-gray-200 dark:border-gray-700 px-4 py-3">
      <div className="flex items-end gap-2 max-w-3xl mx-auto">
        <textarea
          ref={ref}
          value={value}
          onChange={handleChange}
          onKeyDown={handleKeyDown}
          disabled={disabled}
          placeholder="Message Zathas..."
          rows={1}
          className="flex-1 resize-none rounded-xl bg-gray-100 dark:bg-gray-800 px-4 py-2.5 text-sm leading-relaxed outline-none placeholder:text-gray-400 dark:placeholder:text-gray-600 disabled:opacity-50 transition-colors"
        />
        <button
          onClick={submit}
          disabled={disabled || !value.trim()}
          aria-label="Send"
          className={`shrink-0 w-9 h-9 rounded-xl disabled:opacity-40 disabled:cursor-not-allowed flex items-center justify-center transition-colors ${accent === 'zinc' ? 'bg-zinc-700 hover:bg-zinc-600' : 'bg-blue-600 hover:bg-blue-700'}`}
        >
          <svg className="w-4 h-4 text-white" viewBox="0 0 24 24" fill="currentColor">
            <path d="M3.478 2.405a.75.75 0 00-.926.94l2.432 7.905H13.5a.75.75 0 010 1.5H4.984l-2.432 7.905a.75.75 0 00.926.94 60.519 60.519 0 0018.445-8.986.75.75 0 000-1.218A60.517 60.517 0 003.478 2.405z"/>
          </svg>
        </button>
      </div>
    </div>
  )
}
