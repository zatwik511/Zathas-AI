import { useState, useRef, useEffect } from 'react'

const MAX_BYTES = 10 * 1024 * 1024  // 10 MB

export default function ChatInput({ onSend, disabled, accent = 'blue', doc, onDocChange }) {
  const [value, setValue] = useState('')
  const [uploading, setUploading] = useState(false)
  const [uploadPct, setUploadPct] = useState(0)
  const textRef = useRef(null)
  const fileRef = useRef(null)

  useEffect(() => {
    if (!disabled && !uploading) textRef.current?.focus()
  }, [disabled, uploading])

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
    if (!text || disabled || uploading) return
    onSend(text)
    setValue('')
    if (textRef.current) textRef.current.style.height = 'auto'
  }

  function handleFileChange(e) {
    const file = e.target.files?.[0]
    if (!file) return
    e.target.value = ''  // reset so same file can be re-selected
    if (file.size > MAX_BYTES) return  // silently ignore oversized files
    uploadFile(file)
  }

  function uploadFile(file) {
    setUploading(true)
    setUploadPct(0)

    const formData = new FormData()
    formData.append('file', file)

    const xhr = new XMLHttpRequest()
    xhr.upload.addEventListener('progress', (e) => {
      if (e.lengthComputable) setUploadPct(Math.round(e.loaded / e.total * 100))
    })
    xhr.open('POST', '/api/upload')
    xhr.onload = () => {
      setUploading(false)
      if (xhr.status === 200) {
        try {
          const data = JSON.parse(xhr.responseText)
          onDocChange?.({ id: data.doc_id, name: file.name, chars: data.char_count })
        } catch { /* ignore parse error */ }
      }
    }
    xhr.onerror = () => setUploading(false)
    xhr.send(formData)
  }

  const btnClass = accent === 'zinc'
    ? 'bg-zinc-700 hover:bg-zinc-600'
    : 'bg-blue-600 hover:bg-blue-700'

  const borderClass = accent === 'zinc'
    ? 'border-zinc-800'
    : 'border-gray-200 dark:border-gray-700'

  const pillBg = accent === 'zinc'
    ? 'bg-zinc-800 text-zinc-300'
    : 'bg-gray-200 dark:bg-gray-700 text-gray-700 dark:text-gray-300'

  return (
    <div className={`shrink-0 border-t ${borderClass} px-4 py-3`}>
      <div className="flex flex-col gap-2 max-w-3xl mx-auto">

        {/* Doc pill or upload progress */}
        {uploading && (
          <div className={`self-start flex items-center gap-2 px-3 py-1.5 rounded-full text-xs ${pillBg}`}>
            <svg className="w-3 h-3 animate-spin" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2">
              <path d="M21 12a9 9 0 1 1-6.219-8.56"/>
            </svg>
            <span>{uploadPct}%</span>
          </div>
        )}
        {!uploading && doc && (
          <div className={`self-start flex items-center gap-1.5 px-3 py-1.5 rounded-full text-xs ${pillBg} max-w-[260px]`}>
            <svg className="w-3 h-3 shrink-0" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round">
              <path d="M21.44 11.05l-9.19 9.19a6 6 0 0 1-8.49-8.49l9.19-9.19a4 4 0 0 1 5.66 5.66l-9.2 9.19a2 2 0 0 1-2.83-2.83l8.49-8.48"/>
            </svg>
            <span className="truncate">{doc.name}</span>
            <button
              onClick={() => onDocChange?.(null)}
              aria-label="Remove document"
              className="shrink-0 opacity-60 hover:opacity-100 transition-opacity ml-0.5"
            >
              <svg className="w-3 h-3" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2.5" strokeLinecap="round">
                <path d="M18 6L6 18M6 6l12 12"/>
              </svg>
            </button>
          </div>
        )}

        {/* Input row */}
        <div className="flex items-end gap-2">
          {/* Hidden file input */}
          <input
            ref={fileRef}
            type="file"
            accept=".pdf,.txt,text/plain,application/pdf"
            onChange={handleFileChange}
            className="hidden"
          />

          {/* Paperclip button */}
          <button
            onClick={() => fileRef.current?.click()}
            disabled={uploading}
            aria-label="Attach document"
            className="shrink-0 w-9 h-9 rounded-xl border border-gray-200 dark:border-gray-700 hover:bg-gray-100 dark:hover:bg-gray-800 disabled:opacity-40 flex items-center justify-center transition-colors"
          >
            <svg className="w-4 h-4 text-gray-500 dark:text-gray-400" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round">
              <path d="M21.44 11.05l-9.19 9.19a6 6 0 0 1-8.49-8.49l9.19-9.19a4 4 0 0 1 5.66 5.66l-9.2 9.19a2 2 0 0 1-2.83-2.83l8.49-8.48"/>
            </svg>
          </button>

          <textarea
            ref={textRef}
            value={value}
            onChange={handleChange}
            onKeyDown={handleKeyDown}
            disabled={disabled || uploading}
            placeholder="Message Zathas..."
            rows={1}
            className="flex-1 resize-none rounded-xl bg-gray-100 dark:bg-gray-800 px-4 py-2.5 text-sm leading-relaxed outline-none placeholder:text-gray-400 dark:placeholder:text-gray-600 disabled:opacity-50 transition-colors"
          />

          <button
            onClick={submit}
            disabled={disabled || uploading || !value.trim()}
            aria-label="Send"
            className={`shrink-0 w-9 h-9 rounded-xl disabled:opacity-40 disabled:cursor-not-allowed flex items-center justify-center transition-colors ${btnClass}`}
          >
            <svg className="w-4 h-4 text-white" viewBox="0 0 24 24" fill="currentColor">
              <path d="M3.478 2.405a.75.75 0 00-.926.94l2.432 7.905H13.5a.75.75 0 010 1.5H4.984l-2.432 7.905a.75.75 0 00.926.94 60.519 60.519 0 0018.445-8.986.75.75 0 000-1.218A60.517 60.517 0 003.478 2.405z"/>
            </svg>
          </button>
        </div>
      </div>
    </div>
  )
}
