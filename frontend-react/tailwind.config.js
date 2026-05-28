import typography from '@tailwindcss/typography'

/** @type {import('tailwindcss').Config} */
export default {
  content: ['./index.html', './src/**/*.{js,jsx}'],
  darkMode: 'class',
  theme: {
    extend: {
      typography: {
        DEFAULT: {
          css: {
            pre: { padding: 0, margin: 0, backgroundColor: 'transparent' },
            'pre code': { padding: 0 },
            'code::before': { content: 'none' },
            'code::after': { content: 'none' },
          }
        },
        invert: {
          css: {
            pre: { backgroundColor: 'transparent' },
          }
        }
      }
    }
  },
  plugins: [typography]
}
