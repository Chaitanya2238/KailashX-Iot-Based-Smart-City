'use client';

import { useState, useEffect, useRef } from 'react';
import { useRouter } from 'next/navigation';
import { useSerial } from '@/context/SerialContext';
import { ArrowLeft, Wind, Terminal, AlertTriangle, CheckCircle2, Activity, TrendingUp } from 'lucide-react';
import { LineChart, Line, XAxis, YAxis, CartesianGrid, Tooltip, ResponsiveContainer } from 'recharts';

interface ChartData {
  time: string;
  aqi: number;
}

export default function AirQualityMonitoring() {
  const { isConnected, logs, sendCommand } = useSerial();
  const router = useRouter();
  const logEndRef = useRef<HTMLDivElement>(null);
  
  const [currentPpm, setCurrentPpm] = useState<number>(0);
  const [currentAqi, setCurrentAqi] = useState<number>(0);
  const [maxAqi, setMaxAqi] = useState<number>(0);
  const [chartData, setChartData] = useState<ChartData[]>([]);
  
  // Categorization Logic (Adjusted for User Project Baseline)
  const getAirStatus = (aqi: number) => {
    if (aqi <= 80) return { label: 'Good', color: 'text-green-400', bg: 'bg-green-400/10', border: 'border-green-400/20', desc: 'Air quality is excellent. Breathe freely.' };
    if (aqi <= 170) return { label: 'Moderate', color: 'text-yellow-400', bg: 'bg-yellow-400/10', border: 'border-yellow-400/20', desc: 'Normal indoor air quality. Acceptable for most people.' };
    if (aqi <= 250) return { label: 'Sensitive', color: 'text-orange-400', bg: 'bg-orange-400/10', border: 'border-orange-400/20', desc: 'Air quality is slightly stuffy. Sensitive individuals should be aware.' };
    if (aqi <= 350) return { label: 'Unhealthy', color: 'text-red-400', bg: 'bg-red-400/10', border: 'border-red-400/20', desc: 'Poor air quality detected. Consider opening a window.' };
    return { label: 'Hazardous', color: 'text-rose-600', bg: 'bg-rose-600/10', border: 'border-rose-600/20', desc: 'Critical air pollution levels! Take immediate action.' };
  };

  const status = getAirStatus(currentAqi);

  // Parse logs for DATA:AIR:ppm:aqi values
  useEffect(() => {
    if (isConnected) {
      sendCommand('3');
    }
  }, [isConnected]);

  useEffect(() => {
    const lastLog = logs[logs.length - 1]?.text || '';
    if (lastLog.includes('DATA:AIR:')) {
      const parts = lastLog.split('DATA:AIR:')[1].split(':');
      const ppm = parseFloat(parts[0]);
      const aqi = parseInt(parts[1]);

      if (!isNaN(ppm) && !isNaN(aqi)) {
        setCurrentPpm(ppm);
        setCurrentAqi(aqi);
        if (aqi > maxAqi) setMaxAqi(aqi);
        
        const now = new Date().toLocaleTimeString([], { hour12: false, minute: '2-digit', second: '2-digit' });
        setChartData(prev => [...prev.slice(-19), { time: now, aqi: aqi }]);
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
              Air Quality Monitoring
            </h1>
            <p className="text-white/40 text-[10px] uppercase tracking-[0.2em] font-bold">MQ135 Real-Time Analysis</p>
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
            {/* Max AQI Card */}
            <div className="glass-card rounded-2xl p-6 border border-white/5 relative overflow-hidden">
              <div className="absolute top-0 right-0 p-4 opacity-10">
                <TrendingUp className="w-12 h-12 text-white" />
              </div>
              <p className="text-[10px] font-bold uppercase tracking-[0.2em] text-white/40 mb-4">Peak AQI Level</p>
              <div className="flex items-baseline gap-2 mb-2">
                <span className="text-4xl font-black text-white">{maxAqi}</span>
                <span className="text-xs font-bold text-white/30 uppercase tracking-widest">AQI</span>
              </div>
              <div className={`inline-flex items-center gap-1.5 px-3 py-1 rounded-lg text-[10px] font-bold uppercase tracking-wider ${getAirStatus(maxAqi).bg} ${getAirStatus(maxAqi).color} border border-white/5`}>
                Category: {getAirStatus(maxAqi).label}
              </div>
            </div>

            {/* Status & Recommendation Card */}
            <div className={`glass-card rounded-2xl p-6 border ${status.border} ${status.bg} transition-all duration-500`}>
              <div className="flex items-center gap-3 mb-4">
                {currentAqi <= 100 ? <CheckCircle2 className={`w-5 h-5 ${status.color}`} /> : <AlertTriangle className={`w-5 h-5 ${status.color}`} />}
                <span className={`text-[10px] font-bold uppercase tracking-[0.2em] ${status.color}`}>Air Quality Advisory</span>
              </div>
              <h3 className={`text-xl font-bold mb-2 ${status.color}`}>Status: {status.label}</h3>
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
                  <h4 className="text-sm font-bold text-white/90">Live AQI Timeline</h4>
                  <p className="text-[9px] uppercase tracking-widest text-white/30">Real-time concentration trend</p>
                </div>
              </div>
              <div className="flex items-center gap-2">
                <span className="text-[10px] font-bold text-white/40 uppercase tracking-widest">Current:</span>
                <span className={`text-sm font-black ${status.color}`}>{currentAqi} AQI</span>
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
                    dataKey="aqi" 
                    stroke="#3b82f6" 
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
                  log.text.includes('✓') || log.text.includes('Pure') ? 'text-green-400' :
                  log.text.includes('Moderate') ? 'text-yellow-400' :
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
