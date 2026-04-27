# Smart City Command Center 🏙️

This is a local Next.js web dashboard for the Smart City AI System. It currently features a **Traffic Control Simulation** that connects to the ESP32-CAM hardware via the Web Serial API.

## Features
- **Central Dashboard**: Switch between different urban services.
- **3-Lane Traffic Simulation**: Digital twin of the intersection.
- **Web Serial Integration**: Real-time communication with ESP32 via USB.
- **Live Command Console**: Stylized logs directly from the hardware.
- **Emergency Priority Logic**: Automatic 5s Yellow warning and 20s Green priority for Lane 1.

## Getting Started

1. **Install Dependencies**:
   ```bash
   npm install
   ```

2. **Run the Dashboard**:
   ```bash
   npm run dev
   ```

3. **Hardware Setup**:
   - Connect your ESP32-CAM via USB.
   - Ensure the **Serial Monitor in VS Code is CLOSED** (only one app can use the port at a time).
   - Click "Connect Hardware" on the Traffic Control page and select your ESP32's COM port.

## Simulation States
- **Lane 1**: Physical ESP32-CAM link.
- **Lanes 2 & 3**: Simulated traffic flow.
- **Emergency Trigger**: Detected by ESP32 via AWS and reported over Serial.
