/** @type {import('tailwindcss').Config} */
module.exports = {
  content: [
    './pages/**/*.{js,ts,jsx,tsx,mdx}',
    './components/**/*.{js,ts,jsx,tsx,mdx}',
    './app/**/*.{js,ts,jsx,tsx,mdx}',
  ],
  theme: {
    extend: {
      colors: {
        'traffic-red': '#ef4444',
        'traffic-yellow': '#eab308',
        'traffic-green': '#22c55e',
      },
    },
  },
  plugins: [],
}
