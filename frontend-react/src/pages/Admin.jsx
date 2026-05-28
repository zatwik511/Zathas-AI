import { useState, useEffect } from 'react'
import {
  ResponsiveContainer,
  LineChart, Line,
  BarChart, Bar,
  XAxis, YAxis,
  CartesianGrid, Tooltip, Legend
} from 'recharts'

function StatCard({ label, value, sub }) {
  return (
    <div className="bg-zinc-900 border border-zinc-800 rounded-xl p-4">
      <div className="text-2xl font-semibold text-zinc-100">{value}</div>
      <div className="text-xs text-zinc-500 mt-1">{label}</div>
      {sub && <div className="text-xs text-zinc-600 mt-0.5">{sub}</div>}
    </div>
  )
}

const tooltipStyle = {
  background: '#18181b',
  border: '1px solid #3f3f46',
  borderRadius: '8px',
  color: '#f4f4f5',
  fontSize: '11px'
}

function fmtMs(ms) {
  if (!ms) return '0ms'
  return ms >= 1000 ? `${(ms / 1000).toFixed(1)}s` : `${Math.round(ms)}ms`
}

export default function Admin() {
  const [token, setToken]       = useState(() => sessionStorage.getItem('admin_token') || '')
  const [authed, setAuthed]     = useState(() => !!sessionStorage.getItem('admin_token'))
  const [loginInput, setLoginInput] = useState('')
  const [stats, setStats]       = useState(null)
  const [loading, setLoading]   = useState(false)

  function clearAuth() {
    sessionStorage.removeItem('admin_token')
    setToken('')
    setAuthed(false)
    setStats(null)
  }

  function handleLogin(e) {
    e.preventDefault()
    const t = loginInput.trim()
    if (!t) return
    sessionStorage.setItem('admin_token', t)
    setToken(t)
    setAuthed(true)
    setLoginInput('')
  }

  async function fetchStats(tok) {
    setLoading(true)
    try {
      const res = await fetch('/api/admin/stats', {
        headers: { Authorization: `Bearer ${tok}` }
      })
      if (res.status === 401) { clearAuth(); return }
      if (!res.ok) return
      setStats(await res.json())
    } catch { /* network error */ }
    finally { setLoading(false) }
  }

  useEffect(() => {
    document.documentElement.classList.add('dark')
    if (authed && token) fetchStats(token)
  }, [authed])  // eslint-disable-line react-hooks/exhaustive-deps

  if (!authed) {
    return (
      <div className="h-screen flex items-center justify-center bg-black">
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
    <div className="min-h-screen bg-zinc-950 text-zinc-100 p-6">
      <div className="max-w-5xl mx-auto space-y-6">

        <div className="flex items-center justify-between">
          <span className="text-xs text-zinc-600 select-none tracking-widest uppercase">Analytics</span>
          <button
            onClick={() => fetchStats(token)}
            disabled={loading}
            className="text-xs px-3 py-1.5 rounded-lg border border-zinc-800 hover:bg-zinc-800 text-zinc-500 hover:text-zinc-300 transition-colors disabled:opacity-40"
          >
            {loading ? '…' : 'Refresh'}
          </button>
        </div>

        {stats && (
          <>
            {/* Stat cards */}
            <div className="grid grid-cols-2 sm:grid-cols-3 lg:grid-cols-5 gap-3">
              <StatCard label="Total messages"   value={stats.total        ?? 0} />
              <StatCard label="Public"           value={stats.total_public ?? 0} />
              <StatCard label="Prime"            value={stats.total_prime  ?? 0} />
              <StatCard label="Avg response"     value={fmtMs(stats.avg_gen_ms  ?? 0)} />
              <StatCard
                label="Avg msg length"
                value={`${Math.round(stats.avg_msg_len ?? 0)} ch`}
                sub={`Peak hour: ${stats.peak_hour ?? 0}:00 UTC`}
              />
            </div>

            {/* Messages per day — line chart */}
            <div className="bg-zinc-900 border border-zinc-800 rounded-xl p-5">
              <div className="text-xs text-zinc-500 mb-5">Messages per day — last 30 days</div>
              {stats.per_day.length === 0 ? (
                <div className="h-[200px] flex items-center justify-center text-xs text-zinc-700">No data yet</div>
              ) : (
                <ResponsiveContainer width="100%" height={200}>
                  <LineChart data={stats.per_day} margin={{ top: 0, right: 8, left: -24, bottom: 0 }}>
                    <CartesianGrid strokeDasharray="3 3" stroke="#27272a" />
                    <XAxis
                      dataKey="day"
                      tick={{ fill: '#71717a', fontSize: 10 }}
                      tickFormatter={v => v.slice(5)}
                    />
                    <YAxis tick={{ fill: '#71717a', fontSize: 10 }} allowDecimals={false} />
                    <Tooltip contentStyle={tooltipStyle} />
                    <Legend wrapperStyle={{ fontSize: '11px', color: '#71717a', paddingTop: '8px' }} />
                    <Line type="monotone" dataKey="public" stroke="#60a5fa" strokeWidth={1.5} dot={false} />
                    <Line type="monotone" dataKey="prime"  stroke="#a78bfa" strokeWidth={1.5} dot={false} />
                  </LineChart>
                </ResponsiveContainer>
              )}
            </div>

            {/* Messages by hour — bar chart */}
            <div className="bg-zinc-900 border border-zinc-800 rounded-xl p-5">
              <div className="text-xs text-zinc-500 mb-5">Messages by hour — UTC, all time</div>
              <ResponsiveContainer width="100%" height={180}>
                <BarChart data={stats.per_hour} margin={{ top: 0, right: 8, left: -24, bottom: 0 }}>
                  <CartesianGrid strokeDasharray="3 3" stroke="#27272a" />
                  <XAxis
                    dataKey="hour"
                    tick={{ fill: '#71717a', fontSize: 10 }}
                    tickFormatter={v => `${v}h`}
                    interval={1}
                  />
                  <YAxis tick={{ fill: '#71717a', fontSize: 10 }} allowDecimals={false} />
                  <Tooltip
                    contentStyle={tooltipStyle}
                    formatter={(v, _n, p) => [v, `${p.payload.hour}:00 UTC`]}
                    labelFormatter={() => ''}
                  />
                  <Bar dataKey="count" fill="#3f3f46" radius={[2, 2, 0, 0]} />
                </BarChart>
              </ResponsiveContainer>
            </div>
          </>
        )}

        {!stats && !loading && (
          <div className="text-sm text-zinc-700 text-center py-16 select-none">—</div>
        )}

      </div>
    </div>
  )
}
