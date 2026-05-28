import { StrictMode } from 'react'
import { createRoot } from 'react-dom/client'
import './index.css'
import 'highlight.js/styles/atom-one-dark.css'
import App from './App.jsx'
import Prime from './pages/Prime.jsx'
import Admin from './pages/Admin.jsx'

const path = window.location.pathname
const Page = path === '/prime' ? Prime : path === '/admin' ? Admin : App

createRoot(document.getElementById('root')).render(
  <StrictMode>
    <Page />
  </StrictMode>
)
