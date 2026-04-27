'use client';

import Link from 'next/link';
import { useRouter } from 'next/navigation';
import { TrafficCone, Sun, Wind, Droplets, Power } from 'lucide-react';
import { useSerial } from '@/context/SerialContext';

export default function Home() {
  const { isConnected, connect, sendCommand } = useSerial();
  const router = useRouter();

  const services = [
    {
      id: '1',
      title: 'Traffic Control',
      description: 'AI-powered emergency vehicle detection and priority lane management.',
      icon: <TrafficCone className="w-8 h-8 text-amber-500" />,
      link: '/traffic',
      status: 'Live',
      statusColor: 'bg-green-500',
    },
    {
      id: '2',
      title: 'Street Light Control',
      description: 'Adaptive street lighting system based on ambient light detection.',
      icon: <Sun className="w-8 h-8 text-yellow-400" />,
      link: '/streetlight',
      status: 'Live',
      statusColor: 'bg-green-500',
    },
    {
      id: '3',
      title: 'Air Quality Monitoring',
      description: 'Real-time tracking of pollutants and air purity for urban wellness.',
      icon: <Wind className="w-8 h-8 text-sky-400" />,
      link: '/air-quality',
      status: 'Live',
      statusColor: 'bg-green-500',
    },
    {
      id: '4',
      title: 'Water Quality Monitoring',
      description: 'Advanced TDS sensor analysis for mineral balance and water purity.',
      icon: <Droplets className="w-8 h-8 text-blue-400" />,
      link: '/water-quality',
      status: 'Live',
      statusColor: 'bg-green-500',
    },
  ];

  const handleOpenDashboard = async (serviceId: string, link: string) => {
    if (isConnected) {
      await sendCommand(serviceId);
    }
    router.push(link);
  };

  return (
    <main className="max-w-7xl mx-auto px-8 pt-8 pb-4 min-h-screen flex flex-col">
      <div className="flex-grow">
        <header className="mb-24 text-center pt-12 relative">
          <div className="absolute top-0 right-0 pt-4">
            <button
              onClick={isConnected ? () => {} : connect}
              className={`flex items-center gap-2 px-5 py-2.5 rounded-xl font-bold text-xs uppercase tracking-widest transition-all duration-300 ${
                isConnected 
                  ? 'bg-green-500/10 text-green-500 border border-green-500/30' 
                  : 'bg-white/10 hover:bg-white/20 text-white border border-white/20'
              }`}
            >
              <Power className="w-3.5 h-3.5" />
              {isConnected ? 'Hardware Connected' : 'Connect Hardware'}
            </button>
          </div>
          <h1 className="text-5xl md:text-6xl font-black tracking-[0.15em] uppercase text-white mb-2 drop-shadow-[0_0_15px_rgba(255,255,255,0.1)]">
            Smart City
          </h1>
          <p className="text-[10px] font-bold tracking-[0.5em] uppercase text-blue-400/60 mb-12">
            Command Center
          </p>
          <p className="text-white/40 text-xs font-medium uppercase tracking-[0.2em] max-w-xl mx-auto leading-relaxed">
            Centralized monitoring and management<br />for urban infrastructure
          </p>
        </header>

        <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-4 gap-6">
          {services.map((service) => (
            <div key={service.title} className="glass-card glass-card-hover rounded-2xl p-6 flex flex-col">
              <div className="flex justify-between items-start mb-4">
                <div className="p-3 bg-white/5 rounded-xl border border-white/10">
                  {service.icon}
                </div>
                <span className={`px-3 py-1 rounded-full text-[10px] font-bold tracking-wider uppercase flex items-center gap-1.5 ${service.statusColor} bg-opacity-20 ${service.statusColor.replace('bg-', 'text-')} border border-white/10`}>
                  <span className={`w-1.5 h-1.5 rounded-full ${service.statusColor} animate-pulse`}></span>
                  {service.status}
                </span>
              </div>
              
              <h2 className="text-xl font-bold mb-2 tracking-tight text-white/90">{service.title}</h2>
              <p className="text-white/50 text-sm leading-relaxed mb-6 flex-grow">{service.description}</p>
              
              <button 
                onClick={() => handleOpenDashboard(service.id, service.link)}
                className={`w-full text-center py-2.5 rounded-xl font-bold text-xs uppercase tracking-widest transition-all duration-300 ${
                  service.status === 'Live' 
                    ? 'bg-white/10 hover:bg-white/20 text-white border border-white/20' 
                    : 'bg-white/5 text-white/20 cursor-not-allowed border border-white/5'
                }`}
              >
                {service.status === 'Live' ? 'Open Dashboard' : 'Coming Soon'}
              </button>
            </div>
          ))}
        </div>
      </div>

      <footer className="mt-20 py-8 border-t border-white/10 text-center text-white/30 text-[10px] tracking-[0.2em] uppercase">
        <div>© 2026 Smart City AI Project</div>
      </footer>
    </main>
  );
}
