'use client';

import React, { createContext, useContext, useState, useCallback, useRef, ReactNode } from 'react';

type LogType = 'info' | 'success' | 'emergency' | 'warning';

interface Log {
  id: string;
  text: string;
  type: LogType;
}

interface SerialContextType {
  isConnected: boolean;
  logs: Log[];
  connect: () => Promise<void>;
  disconnect: () => Promise<void>;
  sendCommand: (command: string) => Promise<void>;
}

const SerialContext = createContext<SerialContextType | undefined>(undefined);

export const SerialProvider = ({ children }: { children: ReactNode }) => {
  const [port, setPort] = useState<any | null>(null);
  const [isConnected, setIsConnected] = useState(false);
  const [logs, setLogs] = useState<Log[]>([]);
  const readerRef = useRef<any | null>(null);
  const writerRef = useRef<any | null>(null);

  const addLog = useCallback((text: string) => {
    // Only process actual text from hardware
    if (!text || text.trim() === '') return;

    const id = Math.random().toString(36).substring(7);
    let type: LogType = 'info';

    if (text.includes('🚨') || text.includes('EMERGENCY')) type = 'emergency';
    else if (text.includes('✓') || text.includes('✅') || text.includes('SUCCESS') || text.includes('Switching') || text.includes('READY')) type = 'success';
    else if (text.includes('⚠️') || text.includes('WARNING') || text.includes('❌')) type = 'warning';

    setLogs((prev) => [...prev.slice(-99), { id, text, type }]);
  }, []);

  const connect = async () => {
    try {
      if (!('serial' in navigator)) {
        addLog('❌ Web Serial API not supported in this browser');
        return;
      }
      
      const selectedPort = await (navigator as any).serial.requestPort();
      
      await selectedPort.open({ 
        baudRate: 115200,
        dataBits: 8,
        stopBits: 1,
        parity: 'none',
        flowControl: 'none',
        bufferSize: 255,
      });

      // Stable signals without reset pulse
      try {
        await selectedPort.setSignals({ dataTerminalReady: false, requestToSend: false });
      } catch (e) {
        console.warn('Could not set signals:', e);
      }

      // Initialize persistent writer
      writerRef.current = selectedPort.writable.getWriter();

      setPort(selectedPort);
      setIsConnected(true);
      addLog('✅ Hardware Connected successfully');
      
      // Start reading
      readLoopInternal(selectedPort);

      // Small delay for loop to settle, then request state re-announcement
      setTimeout(async () => {
        if (writerRef.current) {
          const encoder = new TextEncoder();
          await writerRef.current.write(encoder.encode('r\n'));
          addLog('📡 Requesting hardware status...');
        }
      }, 500);

    } catch (err) {
      console.error('Serial connection failed:', err);
      addLog('❌ Connection failed: ' + (err as Error).message);
    }
  };

  const readLoopInternal = async (selectedPort: any) => {
    const textDecoder = new TextDecoder();
    let buffer = '';

    try {
      while (selectedPort.readable) {
        const reader = selectedPort.readable.getReader();
        readerRef.current = reader;
        try {
          while (true) {
            const { value, done } = await reader.read();
            if (done) break;
            
            buffer += textDecoder.decode(value, { stream: true });
            const lines = buffer.split(/\r?\n/);
            buffer = lines.pop() || '';

            lines.forEach(line => {
              if (line.trim()) {
                console.log('Hardware Log:', line.trim());
                addLog(line.trim());
              }
            });
          }
        } catch (err) {
          console.error('Serial Read Error:', err);
          break;
        } finally {
          reader.releaseLock();
        }
      }
    } catch (err) {
      console.error('Outer Read Loop Error:', err);
    } finally {
      setPort(null);
      setIsConnected(false);
      addLog('📡 Connection Closed');
    }
  };

  const sendCommand = async (command: string) => {
    if (!writerRef.current) {
      console.warn('No writer available or not connected');
      return;
    }

    try {
      // For menu selection, we send exactly one character + newline for buffer flushing
      // If the command is 'RESET', we send '0' to go back to menu.
      const cmdToSend = (command === 'RESET' ? '0' : command.substring(0, 1)) + '\n';
      const encoder = new TextEncoder();
      const data = encoder.encode(cmdToSend);

      // Use the persistent writer initialized during connect()
      await writerRef.current.write(data);
      
      console.log(`Sent to hardware: ${cmdToSend} (original: ${command})`);
      addLog(`⌨️ Command Sent: ${cmdToSend}`);
    } catch (err) {
      console.error('Send Error:', err);
      addLog('❌ Send Error: ' + (err as Error).message);
      
      // If a terminal error occurs, we might need to reset the writer
      // but usually for transient errors we keep it.
    }
  };

  const disconnect = async () => {
    if (readerRef.current) {
      await readerRef.current.cancel();
    }
    if (writerRef.current) {
      try {
        writerRef.current.releaseLock();
      } catch (e) {}
      writerRef.current = null;
    }
    if (port) {
      try {
        await port.close();
      } catch (err) {
        console.error('Close Error:', err);
      }
      setPort(null);
      setIsConnected(false);
    }
  };

  return (
    <SerialContext.Provider value={{ isConnected, logs, connect, disconnect, sendCommand }}>
      {children}
    </SerialContext.Provider>
  );
};

export const useSerial = () => {
  const context = useContext(SerialContext);
  if (context === undefined) {
    throw new Error('useSerial must be used within a SerialProvider');
  }
  return context;
};
