'use client';

import { useState, useEffect, useRef } from 'react';
import { useRouter } from 'next/navigation';
import { useSerial } from '@/context/SerialContext';
import { ArrowLeft, Droplets, Terminal, AlertTriangle, CheckCircle2, Activity, TrendingUp } from 'lucide-react';
import { LineChart, Line, XAxis, YAxis, CartesianGrid, Tooltip, ResponsiveContainer } from 'recharts';

interface ChartData {
  time: string;
  ppm: number;
}

export default function WaterQualityMonitoring() {
  const { isConnected, logs, sendCommand } = useSerial();
  const router = useRouter();
  const logEndRef = useRef<HTMLDivElement>(null);
  
  const [currentPpm, setCurrentPpm] = useState<number>(0);
  const [maxPpm, setMaxPpm] = useState<number>(0);
  const [chartData, setChartData] = useState<ChartData[]>([]);
  
  // Categorization Logic (Matches main.cpp Service 4)
  const getWaterStatus = (val: number) => {
    if (val <= 10) return { label: 'Dry / No Water', color: 'text-slate-400', bg: 'bg-slate-400/10', border: 'border-slate-400/20', desc: 'Sensor is dry or not submerged in water.' };
    if (val < 50) return { label: 'Ideal (RO)', color: 'text-sky-400', bg: 'bg-sky-400/10', border: 'border-sky-400/20', desc: 'Distilled or RO water. Very low mineral content.' };
    if (val < 150) return { label: 'Excellent', color: 'text-green-400', bg: 'bg-green-400/10', border: 'border-green-400/20', desc: 'Perfect mineral balance for drinking water.' };
    if (val < 300) return { label: 'Good', color: 'text-emerald-400', bg: 'bg-emerald-400/10', border: 'border-emerald-400/20', desc: 'Safe for drinking with pleasant mineral taste.' };
    if (val < 500) return { label: 'Fair', color: 'text-yellow-400', bg: 'bg-yellow-400/10', border: 'border-yellow-400/20', desc: 'Hard water with metallic taste. Acceptable but not ideal.' };
    if (val < 1000) return { label: 'Poor', color: 'text-orange-400', bg: 'bg-orange-400/10', border: 'border-orange-400/20', desc: 'Contaminated / High minerals. Needs treatment before drinking.' };
    return { label: 'Unacceptable', color: 'text-red-500', bg: 'bg-red-500/10', border: 'border-red-500/20', desc: 'Hazardous mineral levels! Unfit for regular consumption.' };
  };

  const status = getWaterStatus(currentPpm);

  // Parse logs for DATA:TDS: values
  useEffect(() => {
    if (isConnected) {
      sendCommand('4');
    }
  }, [isConnected]);

  useEffect(() => {
    const lastLog = logs[logs.length - 1]?.text || '';
    if (lastLog.includes('DATA:TDS:')) {
      const value = parseInt(lastLog.split('DATA:TDS:')[1]);
      if (!isNaN(value)) {
        setCurrentPpm(value);
        if (value > maxPpm) setMaxPpm(value);
        
        const now = new Date().toLocaleTimeString([], { hour12: false, minute: '2-digit', second: '2-digit' });
        setChartData(prev => [...prev.slice(-19), { time: now, ppm: value }]);
      }
    }
  }, [logs]);

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
              Water Quality Monitoring
            </h1>
            <p className="text-white/40 text-[10px] uppercase tracking-[0.2em] font-bold">TDS Sensor Real-Time Analysis</p>
          </div>
        </div>

        {isConnected && (
          <div className="flex items-center gap-2 px-4 py-2 bg-green-500/10 text-green-500 border border-green-500/20 rounded-xl text-[10px] font-bold uppercase tracking-widest">
            <div className="w-1.5 h-1.5 rounded-full bg-green-500 animate-pulse"></div>
            Hardware Active
          </div>
        )}
      </header>

      <div className="grid grid-cols-1 lg:grid-cols-3 gap-6 flex-grow overflow-hidden mb-6">
        {/* Left Control Area */}
        <div className="lg:col-span-2 flex flex-col gap-6 overflow-hidden">
          
          {/* Top Cards */}
          <div className="grid grid-cols-1 md:grid-cols-2 gap-6 shrink-0">
            {/* Max PPM Card */}
            <div className="glass-card rounded-2xl p-6 border border-white/5 relative overflow-hidden">
              <div className="absolute top-0 right-0 p-4 opacity-10">
                <TrendingUp className="w-12 h-12 text-white" />
              </div>
              <p className="text-[10px] font-bold uppercase tracking-[0.2em] text-white/40 mb-4">Peak TDS Level</p>
              <div className="flex items-baseline gap-2 mb-2">
                <span className="text-4xl font-black text-white">{maxPpm}</span>
                <span className="text-xs font-bold text-white/30 uppercase tracking-widest">PPM</span>
              </div>
              <div className={`inline-flex items-center gap-1.5 px-3 py-1 rounded-lg text-[10px] font-bold uppercase tracking-wider ${getWaterStatus(maxPpm).bg} ${getWaterStatus(maxPpm).color} border border-white/5`}>
                Category: {getWaterStatus(maxPpm).label}
              </div>
            </div>

            {/* Status & Recommendation Card */}
            <div className={`glass-card rounded-2xl p-6 border ${status.border} ${status.bg} transition-all duration-500`}>
              <div className="flex items-center gap-3 mb-4">
                {currentPpm < 500 ? <CheckCircle2 className={`w-5 h-5 ${status.color}`} /> : <AlertTriangle className={`w-5 h-5 ${status.color}`} />}
                <span className={`text-[10px] font-bold uppercase tracking-[0.2em] ${status.color}`}>Purity Advisory</span>
              </div>
              <h3 className={`text-xl font-bold mb-2 ${status.color}`}>Water is {status.label}</h3>
              <p className="text-white/60 text-xs leading-relaxed font-medium">
                {status.desc}
              </p>
            </div>
          </div>

          {/* Bottom Graph Area */}
          <div className="flex-grow glass-card rounded-2xl p-6 border border-white/5 flex flex-col h-[400px]">
            <div className="flex items-center justify-between mb-6 shrink-0">
              <div className="flex items-center gap-3">
                <div className="p-2 bg-blue-500/10 rounded-lg border border-blue-500/20">
                  <Activity className="w-4 h-4 text-blue-400" />
                </div>
                <div>
                  <h4 className="text-sm font-bold text-white/90">Live TDS Timeline</h4>
                  <p className="text-[9px] uppercase tracking-widest text-white/30">Total Dissolved Solids trend</p>
                </div>
              </div>
              <div className="flex items-center gap-2">
                <span className="text-[10px] font-bold text-white/40 uppercase tracking-widest">Current:</span>
                <span className={`text-sm font-black ${status.color}`}>{currentPpm} PPM</span>
              </div>
            </div>
            
            <div className="flex-grow min-h-0">
              <ResponsiveContainer width="100%" height="100%">
                <LineChart data={chartData}>
                  <CartesianGrid strokeDasharray="3 3" stroke="#ffffff05" vertical={false} />
                  <XAxis 
                    dataKey="time" 
                    stroke="#ffffff20" 
                    fontSize={10} 
                    tickLine={false} 
                    axisLine={false}
                  />
                  <YAxis 
                    stroke="#ffffff20" 
                    fontSize={10} 
                    tickLine={false} 
                    axisLine={false}
                    domain={['auto', 'auto']}
                  />
                  <Tooltip 
                    contentStyle={{ backgroundColor: '#0f172a', border: '1px solid rgba(255,255,255,0.1)', borderRadius: '12px' }}
                    itemStyle={{ fontSize: '12px', fontWeight: 'bold' }}
                    labelStyle={{ fontSize: '10px', color: 'rgba(255,255,255,0.4)', marginBottom: '4px' }}
                  />
                  <Line 
                    type="monotone" 
                    dataKey="ppm" 
                    stroke="#0ea5e9" 
                    strokeWidth={3} 
                    dot={false}
                    animationDuration={300}
                  />
                </LineChart>
              </ResponsiveContainer>
            </div>
          </div>
        </div>

        {/* Right Logs Area */}
        <div className="flex flex-col glass-panel rounded-2xl overflow-hidden min-h-0">
          <div className="bg-white/5 px-4 py-4 flex items-center gap-2 border-b border-white/10 shrink-0">
            <Terminal className="w-4 h-4 text-blue-400" />
            <span className="text-[10px] font-bold uppercase tracking-[0.3em] text-white/70">Hardware Data Stream</span>
          </div>
          <div className="flex-grow overflow-y-auto p-4 font-mono text-[11px] leading-relaxed space-y-1 custom-scrollbar">
            {logs.map((log) => (
              <div key={log.id} className="flex gap-3 opacity-90 animate-in fade-in slide-in-from-left-2 duration-300">
                <span className="text-white/20 shrink-0 select-none">[{new Date().toLocaleTimeString([], { hour12: false })}]</span>
                <span className={
                  log.text.includes('ALERT') ? 'text-red-400 font-bold' :
                  log.text.includes('✓') || log.text.includes('Excellent') || log.text.includes('Ideal') ? 'text-green-400' :
                  log.text.includes('Fair') ? 'text-yellow-400' :
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
