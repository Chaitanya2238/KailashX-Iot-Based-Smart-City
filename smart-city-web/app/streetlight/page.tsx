'use client';

import { useRouter } from 'next/navigation';
import { useSerial } from '@/context/SerialContext';
import { ArrowLeft, Sun, Moon, Terminal, Zap, Info } from 'lucide-react';
import { useEffect, useRef, useState } from 'react';

export default function StreetLightControl() {
  const { isConnected, logs, sendCommand } = useSerial();
  const router = useRouter();
  const logEndRef = useRef<HTMLDivElement>(null);
  const [isDark, setIsDark] = useState<boolean>(false);

  // Parse logs for DATA:LIGHT:ON/OFF
  useEffect(() => {
    const lastLog = logs[logs.length - 1]?.text || '';
    if (lastLog.includes('DATA:LIGHT:ON')) {
      setIsDark(true);
    } else if (lastLog.includes('DATA:LIGHT:OFF')) {
      setIsDark(false);
    }
  }, [logs]);

  useEffect(() => {
    if (isConnected) {
      sendCommand('2');
    }
  }, [isConnected]);

  useEffect(() => {
    if (logEndRef.current) {
      logEndRef.current.scrollIntoView({ behavior: 'smooth' });
    }
  }, [logs]);

  const handleBack = async () => {
    if (isConnected) {
      await sendCommand('0');
    }
    router.push('/');
  };

  return (
    <div className="max-w-7xl mx-auto p-6 flex flex-col h-screen overflow-hidden">
      <header className="flex justify-between items-center mb-8 shrink-0">
        <div className="flex items-center gap-4">
          <button 
            onClick={handleBack}
            className="p-2 hover:bg-white/5 rounded-xl border border-white/10 transition-all duration-300"
          >
            <ArrowLeft className="w-6 h-6 text-white/70" />
          </button>
          <div>
            <h1 className="text-2xl font-bold tracking-tight text-white/90">
              Street Light Control
            </h1>
            <p className="text-white/40 text-[10px] uppercase tracking-[0.2em] font-bold">Adaptive Lighting Monitoring</p>
          </div>
        </div>

        {isConnected && (
          <div className="flex items-center gap-2 px-4 py-2 bg-green-500/10 text-green-500 border border-green-500/20 rounded-xl text-[10px] font-bold uppercase tracking-widest">
            <div className="w-1.5 h-1.5 rounded-full bg-green-500 animate-pulse"></div>
            Hardware Active
          </div>
        )}
      </header>

      <div className="grid grid-cols-1 lg:grid-cols-3 gap-8 flex-grow overflow-hidden mb-6">
        <div className="lg:col-span-2 flex flex-col gap-6 overflow-hidden">
          
          {/* Main Visualizer - Always Dark Theme */}
          <div className="flex-grow glass-card rounded-3xl p-8 flex flex-col items-center justify-center relative overflow-hidden border border-white/5 bg-white/5">
            
            {/* Ambient Background Elements */}
            <div className={`absolute inset-0 transition-opacity duration-1000 ${isDark ? 'opacity-20' : 'opacity-5'}`}>
              <div className="absolute top-10 left-10 w-32 h-32 bg-blue-500 rounded-full blur-[80px]"></div>
              <div className="absolute bottom-10 right-10 w-32 h-32 bg-purple-500 rounded-full blur-[80px]"></div>
            </div>

            <div className="relative z-10 flex flex-col items-center">
              <div className={`w-32 h-32 rounded-full flex items-center justify-center mb-8 transition-all duration-1000 ${isDark ? 'bg-blue-900/40 text-blue-400 scale-110 shadow-[0_0_50px_rgba(59,130,246,0.3)]' : 'bg-yellow-500/10 text-yellow-500 scale-100'}`}>
                {isDark ? <Moon className="w-16 h-16" /> : <Sun className="w-16 h-16" />}
              </div>
              
              <h2 className="text-4xl font-black mb-2 text-white">
                {isDark ? 'NIGHT MODE' : 'DAY MODE'}
              </h2>
              <p className={`text-sm font-medium transition-colors duration-1000 ${isDark ? 'text-blue-400' : 'text-yellow-500'}`}>
                {isDark ? 'Street Lights are currently ACTIVE' : 'Street Lights are currently INACTIVE'}
              </p>
            </div>

            {/* Street Lamp Illustration */}
            <div className="mt-12 relative">
              <div className="w-1 h-32 bg-slate-700"></div>
              <div className="absolute -top-4 -left-6 w-14 h-6 rounded-full bg-slate-700"></div>
              <div className={`absolute top-2 -left-3 w-8 h-4 rounded-b-full transition-all duration-1000 ${isDark ? 'bg-yellow-300 shadow-[0_5px_30px_rgba(253,224,71,0.8)]' : 'bg-slate-600'}`}></div>
            </div>
          </div>

          {/* Info Cards */}
          <div className="grid grid-cols-1 md:grid-cols-2 gap-6">
            <div className="glass-card rounded-2xl p-6 border border-white/5 bg-white/5">
              <div className="flex items-center gap-3 mb-4">
                <div className="p-2 rounded-lg bg-yellow-500/10">
                  <Zap className="w-4 h-4 text-yellow-500" />
                </div>
                <span className="text-[10px] font-bold uppercase tracking-widest text-white/40">System Power</span>
              </div>
              <p className="text-2xl font-bold text-white">{isDark ? 'Active: 100% Load' : 'Standby: 0% Power'}</p>
            </div>

            <div className="glass-card rounded-2xl p-6 border border-white/5 bg-white/5">
              <div className="flex items-center gap-3 mb-4">
                <div className="p-2 rounded-lg bg-blue-500/10">
                  <Info className="w-4 h-4 text-blue-500" />
                </div>
                <span className="text-[10px] font-bold uppercase tracking-widest text-white/40">Automation</span>
              </div>
              <p className="text-2xl font-bold text-white">LDR Active</p>
            </div>
          </div>
        </div>

        {/* Console */}
        <div className="flex flex-col glass-panel rounded-2xl overflow-hidden min-h-0 bg-white/5 border border-white/5">
          <div className="bg-white/5 px-4 py-4 flex items-center gap-2 border-b border-white/10 shrink-0">
            <Terminal className="w-4 h-4 text-blue-400" />
            <span className="text-[10px] font-bold uppercase tracking-[0.3em] text-white/70">Hardware Stream</span>
          </div>
          <div className="flex-grow overflow-y-auto p-4 font-mono text-[11px] leading-relaxed space-y-1 custom-scrollbar">
            {logs.map((log) => (
              <div key={log.id} className="flex gap-3 opacity-90 animate-in fade-in slide-in-from-left-2 duration-300">
                <span className="text-white/20 shrink-0 select-none">[{new Date().toLocaleTimeString([], { hour12: false })}]</span>
                <span className={
                  log.text.includes('ON') ? 'text-yellow-500 font-bold' :
                  log.text.includes('OFF') ? 'text-blue-500' :
                  'text-blue-200/70'
                }>
                  {log.text}
                </span>
              </div>
            ))}
            <div ref={logEndRef} />
          </div>
        </div>
      </div>
    </div>
  );
}
