'use client';

/// <reference types="w3c-web-serial" />

import { useState, useCallback, useRef } from 'react';

export const useWebSerial = () => {
  const [port, setPort] = useState<SerialPort | null>(null);
  const [isConnected, setIsConnected] = useState(false);
  const [logs, setLogs] = useState<{ id: string; text: string; type: 'info' | 'success' | 'emergency' | 'warning' }[]>([]);
  const readerRef = useRef<ReadableStreamDefaultReader | null>(null);

  const addLog = useCallback((text: string) => {
    const id = Math.random().toString(36).substring(7);
    let type: 'info' | 'success' | 'emergency' | 'warning' = 'info';

    if (text.includes('🚨') || text.includes('EMERGENCY')) type = 'emergency';
    else if (text.includes('✓') || text.includes('✅') || text.includes('SUCCESS')) type = 'success';
    else if (text.includes('⚠️') || text.includes('WARNING')) type = 'warning';

    setLogs((prev) => [...prev.slice(-49), { id, text, type }]);
  }, []);

  const connect = async () => {
    try {
      const selectedPort = await navigator.serial.requestPort();
      await selectedPort.open({ baudRate: 115200 });
      setPort(selectedPort);
      setIsConnected(true);
      addLog('✓ Connected to Hardware Serial Port');
      readLoop(selectedPort);
    } catch (err) {
      console.error('Serial connection failed:', err);
      addLog('❌ Connection Failed: ' + (err as Error).message);
    }
  };

  const readLoop = async (selectedPort: SerialPort) => {
    const textDecoder = new TextDecoder();
    let buffer = '';

    while (selectedPort.readable) {
      const reader = selectedPort.readable.getReader();
      readerRef.current = reader;
      try {
        while (true) {
          const { value, done } = await reader.read();
          if (done) break;
          
          buffer += textDecoder.decode(value);
          const lines = buffer.split(/\r?\n/);
          buffer = lines.pop() || ''; // Keep the last partial line in buffer

          lines.forEach(line => {
            if (line.trim()) addLog(line.trim());
          });
        }
      } catch (err) {
        console.error('Serial Read Error:', err);
        addLog('❌ Read Error: ' + (err as Error).message);
      } finally {
        reader.releaseLock();
      }
    }
  };

  const sendCommand = async (command: string) => {
    if (!port || !port.writable) return;
    const writer = port.writable.getWriter();
    const encoder = new TextEncoder();
    await writer.write(encoder.encode(command + '\n'));
    writer.releaseLock();
    addLog(`⌨️ Command Sent: ${command}`);
  };

  const disconnect = async () => {
    if (readerRef.current) {
      await readerRef.current.cancel();
    }
    if (port) {
      await port.close();
      setPort(null);
      setIsConnected(false);
      addLog('🔌 Disconnected from Hardware');
    }
  };

  return { isConnected, logs, connect, disconnect, sendCommand };
};
