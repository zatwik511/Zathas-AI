import { useMemo } from 'react'
import { marked } from '../lib/markdown'

export default function ChatMessage({ message }) {
  const isUser = message.role === 'user'

  // Only parse markdown when the message is fully received
  const html = useMemo(() => {
    if (isUser || message.streaming) return null
    return marked.parse(message.content || '')
  }, [isUser, message.streaming, message.content])

  if (isUser) {
    return (
      <div className="flex justify-end">
        <div className="max-w-[70%] sm:max-w-[60%] bg-blue-600 text-white px-4 py-2.5 rounded-2xl rounded-br-none text-sm leading-relaxed whitespace-pre-wrap break-words">
          {message.content}
        </div>
      </div>
    )
  }

  return (
    <div className="flex items-start gap-3">
      <div className="shrink-0 w-7 h-7 mt-0.5 rounded-full bg-violet-600 flex items-center justify-center text-white text-xs font-bold select-none">
        Z
      </div>
      <div className="flex-1 min-w-0 text-sm leading-relaxed">
        {message.streaming ? (
          <p className="whitespace-pre-wrap break-words text-gray-900 dark:text-gray-100">
            {message.content}
            <span className="inline-block w-0.5 h-[1em] bg-gray-500 dark:bg-gray-400 animate-cursor ml-0.5 align-middle" />
          </p>
        ) : (
          <div
            className="prose prose-sm dark:prose-invert max-w-none"
            dangerouslySetInnerHTML={{ __html: html || '' }}
          />
        )}
      </div>
    </div>
  )
}
