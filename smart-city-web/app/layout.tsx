import type { Metadata } from 'next'
import { Inter } from 'next/font/google'
import './globals.css'

import { SerialProvider } from '@/context/SerialContext'

const inter = Inter({ subsets: ['latin'] })

export const metadata: Metadata = {
  title: 'Smart City Command Center',
  description: 'AI-Powered Urban Management Dashboard',
}

export default function RootLayout({
  children,
}: {
  children: React.ReactNode
}) {
  return (
    <html lang="en">
      <body className={`${inter.className} bg-slate-950 text-slate-50 min-h-screen`}>
        <SerialProvider>
          {children}
        </SerialProvider>
      </body>
    </html>
  )
}
