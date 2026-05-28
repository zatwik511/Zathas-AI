import { StrictMode } from 'react'
import { createRoot } from 'react-dom/client'
import './index.css'
import 'highlight.js/styles/atom-one-dark.css'
import App from './App.jsx'
import Prime from './pages/Prime.jsx'

const Page = window.location.pathname === '/prime' ? Prime : App

createRoot(document.getElementById('root')).render(
  <StrictMode>
    <Page />
  </StrictMode>
)
