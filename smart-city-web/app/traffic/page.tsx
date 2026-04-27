'use client';

import { useState, useEffect, useRef } from 'react';
import { useRouter } from 'next/navigation';
import { useSerial } from '@/context/SerialContext';
import { ArrowLeft, Power, RotateCcw, ShieldCheck, Terminal } from 'lucide-react';

type LightState = 'RED' | 'YELLOW' | 'GREEN';

export default function TrafficControl() {
  const { isConnected, logs, connect, sendCommand } = useSerial();
  const router = useRouter();
  const [lane1, setLane1] = useState<LightState>('RED');
  const [lane2, setLane2] = useState<LightState>('GREEN');
  const [lane3, setLane3] = useState<LightState>('GREEN');
  const [timer, setTimer] = useState<number | null>(null);
  const [isEmergency, setIsEmergency] = useState(false);
  const logEndRef = useRef<HTMLDivElement>(null);
  const timerIntervalRef = useRef<NodeJS.Timeout | null>(null);

  useEffect(() => {
    return () => {
      if (timerIntervalRef.current) clearInterval(timerIntervalRef.current);
    };
  }, []);

  useEffect(() => {
    if (logEndRef.current) {
      logEndRef.current.scrollIntoView({ behavior: 'smooth' });
    }
  }, [logs]);

  // Handle Serial State Triggers
  useEffect(() => {
    if (logs.length === 0) return;
    
    // Check the last few logs to ensure we don't miss transitions
    const recentLogs = logs.slice(-3).map(l => l.text);
    const lastLog = recentLogs[recentLogs.length - 1];
    
    if (recentLogs.some(log => log.includes('DATA:STATE:WARNING'))) {
      if (!isEmergency || lane2 !== 'YELLOW') {
        triggerWarning();
      }
    } else if (recentLogs.some(log => log.includes('DATA:STATE:PRIORITY'))) {
      if (lane1 !== 'GREEN') {
        triggerPriority();
      }
    } else if (recentLogs.some(log => log.includes('DATA:STATE:NORMAL') || log.includes('RESET_RECEIVED'))) {
      if (isEmergency || lane1 !== 'RED') {
        resetToNormal();
      }
    }
  }, [logs]);

  const triggerWarning = () => {
    if (timerIntervalRef.current) clearInterval(timerIntervalRef.current);
    setIsEmergency(true);
    // Stage 1: Warning (10s Yellow)
    setLane1('YELLOW'); 
    setLane2('YELLOW');
    setLane3('YELLOW');
    setTimer(10);

    timerIntervalRef.current = setInterval(() => {
      setTimer((prev) => {
        if (prev !== null && prev <= 1) {
          if (timerIntervalRef.current) clearInterval(timerIntervalRef.current);
          return 0;
        }
        return prev ? prev - 1 : 0;
      });
    }, 1000);
  };

  const triggerPriority = () => {
    if (timerIntervalRef.current) clearInterval(timerIntervalRef.current);
    setIsEmergency(true);
    // Stage 2: Priority (30s Green)
    setLane1('GREEN');
    setLane2('RED');
    setLane3('RED');
    setTimer(30);

    timerIntervalRef.current = setInterval(() => {
      setTimer((prev) => {
        if (prev !== null && prev <= 1) {
          if (timerIntervalRef.current) clearInterval(timerIntervalRef.current);
          return 0;
        }
        return prev ? prev - 1 : 0;
      });
    }, 1000);
  };

  const resetToNormal = () => {
    if (timerIntervalRef.current) clearInterval(timerIntervalRef.current);
    setIsEmergency(false);
    setLane1('RED');
    setLane2('GREEN');
    setLane3('GREEN');
    setTimer(null);
  };

  const handleManualReset = async () => {
    await sendCommand('RESET');
    resetToNormal();
  };

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
              Traffic Control Simulation
            </h1>
            <p className="text-white/40 text-[10px] uppercase tracking-[0.2em] font-bold">3-Lane Monitoring for Emergency Situation</p>
          </div>
        </div>

        <div className="flex gap-3">
          {isConnected && (
            <div className="flex items-center gap-2 px-4 py-2 bg-green-500/10 text-green-500 border border-green-500/20 rounded-xl text-[10px] font-bold uppercase tracking-widest">
              <div className="w-1.5 h-1.5 rounded-full bg-green-500 animate-pulse"></div>
              Hardware Active
            </div>
          )}
          <button
            onClick={handleManualReset}
            disabled={!isConnected}
            className="flex items-center gap-2 px-5 py-2.5 bg-white/5 hover:bg-white/10 border border-white/10 disabled:opacity-30 disabled:cursor-not-allowed rounded-xl font-bold text-xs uppercase tracking-widest transition-all duration-300"
          >
            <RotateCcw className="w-3.5 h-3.5" />
            Reset System
          </button>
        </div>
      </header>

      <div className="grid grid-cols-1 lg:grid-cols-3 gap-8 flex-grow overflow-hidden mb-6">
        {/* Lane Monitoring Section - 2/3 width */}
        <div className="lg:col-span-2 flex flex-col gap-6 overflow-hidden">
          {/* Lane 1 (Main - Live) */}
          <div className="flex-1 glass-card rounded-2xl p-8 flex items-center justify-between relative overflow-hidden min-h-0">
            {isEmergency && (
              <div className="absolute top-0 left-0 w-1 h-full bg-red-500 animate-pulse shadow-[0_0_20px_rgba(239,68,68,0.8)]"></div>
            )}
            <div className="flex flex-col">
              <span className="text-blue-400 font-bold text-[10px] tracking-[0.2em] mb-6 uppercase opacity-80">Lane 1 (Live Hardware)</span>
              <div className="bg-black/60 backdrop-blur-md p-4 rounded-[2rem] flex flex-col gap-4 border border-white/10 shadow-2xl">
                <div className={`w-12 h-12 rounded-full transition-all duration-500 ${lane1 === 'RED' ? 'bg-red-500 shadow-[0_0_30px_rgba(239,68,68,0.8)] border border-red-400/50' : 'bg-red-950/40 border border-white/5'}`}></div>
                <div className={`w-12 h-12 rounded-full transition-all duration-500 ${lane1 === 'YELLOW' ? 'bg-amber-500 shadow-[0_0_30px_rgba(245,158,11,0.8)] border border-amber-400/50' : 'bg-amber-950/40 border border-white/5'}`}></div>
                <div className={`w-12 h-12 rounded-full transition-all duration-500 ${lane1 === 'GREEN' ? 'bg-green-500 shadow-[0_0_30px_rgba(34,197,94,0.8)] border border-green-400/50' : 'bg-green-950/40 border border-white/5'}`}></div>
              </div>
            </div>

            <div className="flex flex-col items-end gap-4">
              <span className="text-white/30 text-[10px] font-bold uppercase tracking-[0.2em]">Priority Timer</span>
              <div className="bg-black/40 border border-white/10 rounded-3xl w-48 h-48 flex items-center justify-center shadow-inner">
                <span className={`text-7xl font-mono font-bold tabular-nums tracking-tighter ${
                  lane1 === 'GREEN' ? 'text-green-500 drop-shadow-[0_0_15px_rgba(34,197,94,0.4)]' : 
                  lane1 === 'YELLOW' ? 'text-amber-500 drop-shadow-[0_0_15px_rgba(245,158,11,0.4)]' : 
                  'text-white/5'
                }`}>
                  {isEmergency && timer !== null ? timer : '--'}
                </span>
              </div>
            </div>
          </div>

          {/* Bottom Half - Lanes 2 & 3 */}
          <div className="flex gap-6 h-64 shrink-0">
            {/* Lane 2 */}
            <div className="flex-1 glass-card rounded-2xl p-6 flex items-center justify-between">
              <div className="flex flex-col">
                <span className="text-white/40 font-bold text-[10px] tracking-[0.2em] mb-4 uppercase">Lane 2 (Virtual)</span>
                <div className="bg-black/40 p-3 rounded-[1.5rem] flex flex-col gap-3 border border-white/5">
                  <div className={`w-8 h-8 rounded-full transition-all duration-500 ${lane2 === 'RED' ? 'bg-red-500 shadow-[0_0_20px_rgba(239,68,68,0.6)]' : 'bg-red-950/40 border border-white/5'}`}></div>
                  <div className={`w-8 h-8 rounded-full transition-all duration-500 ${lane2 === 'YELLOW' ? 'bg-amber-500 shadow-[0_0_20px_rgba(245,158,11,0.6)]' : 'bg-amber-950/40 border border-white/5'}`}></div>
                  <div className={`w-8 h-8 rounded-full transition-all duration-500 ${lane2 === 'GREEN' ? 'bg-green-500 shadow-[0_0_20px_rgba(34,197,94,0.6)]' : 'bg-green-950/40 border border-white/5'}`}></div>
                </div>
              </div>
              <div className="bg-black/40 border border-white/10 rounded-2xl w-24 h-24 flex items-center justify-center">
                <span className={`text-3xl font-mono font-bold tabular-nums ${lane2 === 'YELLOW' ? 'text-amber-500 drop-shadow-[0_0_10px_rgba(245,158,11,0.4)]' : 'text-white/5'}`}>
                  {lane2 === 'YELLOW' ? timer : '--'}
                </span>
              </div>
            </div>

            {/* Lane 3 */}
            <div className="flex-1 glass-card rounded-2xl p-6 flex items-center justify-between">
              <div className="flex flex-col">
                <span className="text-white/40 font-bold text-[10px] tracking-[0.2em] mb-4 uppercase">Lane 3 (Virtual)</span>
                <div className="bg-black/40 p-3 rounded-[1.5rem] flex flex-col gap-3 border border-white/5">
                  <div className={`w-8 h-8 rounded-full transition-all duration-500 ${lane3 === 'RED' ? 'bg-red-500 shadow-[0_0_20px_rgba(239,68,68,0.6)]' : 'bg-red-950/40 border border-white/5'}`}></div>
                  <div className={`w-8 h-8 rounded-full transition-all duration-500 ${lane3 === 'YELLOW' ? 'bg-amber-500 shadow-[0_0_20px_rgba(245,158,11,0.6)]' : 'bg-amber-950/40 border border-white/5'}`}></div>
                  <div className={`w-8 h-8 rounded-full transition-all duration-500 ${lane3 === 'GREEN' ? 'bg-green-500 shadow-[0_0_20px_rgba(34,197,94,0.6)]' : 'bg-green-950/40 border border-white/5'}`}></div>
                </div>
              </div>
              <div className="bg-black/40 border border-white/10 rounded-2xl w-24 h-24 flex items-center justify-center">
                <span className={`text-3xl font-mono font-bold tabular-nums ${lane3 === 'YELLOW' ? 'text-amber-500 drop-shadow-[0_0_10px_rgba(245,158,11,0.4)]' : 'text-white/5'}`}>
                  {lane3 === 'YELLOW' ? timer : '--'}
                </span>
              </div>
            </div>
          </div>
        </div>

        {/* Live Command Console - 1/3 width */}
        <div className="flex flex-col glass-panel rounded-2xl overflow-hidden min-h-0">
          <div className="bg-white/5 px-4 py-4 flex items-center justify-between border-b border-white/10 shrink-0">
            <div className="flex items-center gap-2">
              <Terminal className="w-4 h-4 text-blue-400" />
              <span className="text-[10px] font-bold uppercase tracking-[0.3em] text-white/70">Live Command Console</span>
            </div>
            <div className="flex gap-2">
              <div className="w-2 h-2 rounded-full bg-red-500/40 shadow-[0_0_10px_rgba(239,68,68,0.2)]"></div>
              <div className="w-2 h-2 rounded-full bg-amber-500/40 shadow-[0_0_10px_rgba(245,158,11,0.2)]"></div>
              <div className="w-2 h-2 rounded-full bg-green-500/40 shadow-[0_0_10px_rgba(34,197,94,0.2)]"></div>
            </div>
          </div>
          
          <div className="flex-grow overflow-y-auto p-5 font-mono text-[11px] leading-relaxed space-y-3 bg-black/20 min-h-0 custom-scrollbar">
            {logs.length === 0 && (
              <p className="text-white/20 italic tracking-wide">Waiting for hardware connection...</p>
            )}
            {logs.map((log) => (
              <div 
                key={log.id} 
                className={`
                  ${log.type === 'emergency' ? 'text-red-400 font-bold bg-red-500/10 border-l-2 border-red-500 px-3 py-1.5' : ''}
                  ${log.type === 'success' ? 'text-green-400' : ''}
                  ${log.type === 'warning' ? 'text-amber-400' : ''}
                  ${log.type === 'info' ? 'text-white/40' : ''}
                `}
              >
                <span className="text-white/20 mr-3 text-[9px] font-bold">
                  {new Date().toLocaleTimeString([], { hour12: false })}
                </span>
                {log.text}
              </div>
            ))}
            <div ref={logEndRef} />
          </div>
          
          <div className="p-3 bg-white/5 border-t border-white/10 text-[9px] font-bold text-white/20 flex justify-between shrink-0 uppercase tracking-widest">
            <span>HARDWARE STATUS: {isConnected ? 'ONLINE' : 'OFFLINE'}</span>
            <span>PARSING: ENABLED</span>
          </div>
        </div>
      </div>
    </div>
  );
}
