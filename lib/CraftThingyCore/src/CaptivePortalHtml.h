#pragma once
#include <Arduino.h>

static const char CAPTIVE_PORTAL_HTML[] PROGMEM = R"rawliteral(<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1,maximum-scale=1,user-scalable=no">
<title>CraftThingy Water Pump & Energy Monitor</title>
<style>
:root{
  --bg:#070b14;
  --surface:#0f172a;
  --surface-glass:rgba(15,23,42,0.82);
  --card-border:rgba(255,255,255,0.08);
  --card-border-glow:rgba(56,189,248,0.25);
  --cyan:#38bdf8;
  --cyan-dark:#0284c7;
  --emerald:#10b981;
  --emerald-dark:#059669;
  --amber:#f59e0b;
  --rose:#f43f5e;
  --violet:#a855f7;
  --text-main:#f8fafc;
  --text-muted:#94a3b8;
  --text-dim:#64748b;
}
*{box-sizing:border-box;margin:0;padding:0}
body{
  font-family:system-ui,-apple-system,BlinkMacSystemFont,"Segoe UI",Roboto,sans-serif;
  background:radial-gradient(ellipse at top,#0f1d36 0%,#070b14 70%);
  color:var(--text-main);
  padding:16px 12px 32px 12px;
  min-height:100vh;
  display:flex;
  flex-direction:column;
  align-items:center;
}
.app-container{
  width:100%;
  max-width:480px;
  display:flex;
  flex-direction:column;
  gap:16px;
}
/* Glassmorphism Card Base */
.card{
  background:var(--surface-glass);
  backdrop-filter:blur(14px);
  -webkit-backdrop-filter:blur(14px);
  border:1px solid var(--card-border);
  border-radius:18px;
  padding:20px;
  box-shadow:0 12px 32px rgba(0,0,0,0.5);
  position:relative;
  overflow:hidden;
}
.card::before{
  content:'';
  position:absolute;
  top:0;left:0;right:0;height:1px;
  background:linear-gradient(90deg,transparent,rgba(255,255,255,0.15),transparent);
}
/* Top App Header */
.header{
  display:flex;
  justify-content:space-between;
  align-items:center;
  padding:6px 4px;
}
.brand{
  display:flex;
  align-items:center;
  gap:10px;
}
.brand-icon{
  width:36px;height:36px;
  border-radius:10px;
  background:linear-gradient(135deg,#0284c7,#38bdf8);
  display:flex;align-items:center;justify-content:center;
  box-shadow:0 0 16px rgba(56,189,248,0.4);
}
.brand-icon svg{width:20px;height:20px;fill:#fff}
.brand-text h1{font-size:1.15rem;font-weight:700;letter-spacing:-0.3px;color:#fff}
.brand-text p{font-size:0.75rem;color:var(--text-muted)}
.live-pill{
  display:inline-flex;
  align-items:center;
  gap:6px;
  padding:4px 10px;
  border-radius:99px;
  font-size:0.72rem;
  font-weight:600;
  background:rgba(16,185,129,0.15);
  border:1px solid rgba(16,185,129,0.3);
  color:#34d399;
}
.live-pill.ap-mode{
  background:rgba(245,158,11,0.15);
  border-color:rgba(245,158,11,0.3);
  color:#fbbf24;
}
.pulse-dot{
  width:6px;height:6px;
  border-radius:50%;
  background:#10b981;
  box-shadow:0 0 8px #10b981;
  animation:pulse 1.8s infinite;
}
.live-pill.ap-mode .pulse-dot{
  background:#f59e0b;
  box-shadow:0 0 8px #f59e0b;
}
@keyframes pulse{
  0%,100%{opacity:1;transform:scale(1)}
  50%{opacity:0.3;transform:scale(0.8)}
}
/* Schema & Info Sub-bar */
.schema-bar{
  display:flex;
  justify-content:space-between;
  align-items:center;
  background:rgba(11,17,32,0.6);
  border:1px solid rgba(255,255,255,0.05);
  border-radius:12px;
  padding:8px 14px;
  font-size:0.78rem;
}
.badge{
  display:inline-block;
  padding:3px 8px;
  border-radius:6px;
  font-size:0.7rem;
  font-weight:700;
  text-transform:uppercase;
  letter-spacing:0.4px;
}
.badge-pzem{background:rgba(168,85,247,0.18);border:1px solid rgba(168,85,247,0.4);color:#c084fc}
.badge-legacy{background:rgba(148,163,184,0.15);border:1px solid rgba(148,163,184,0.3);color:#cbd5e1}

/* Hero Section: Visual Tank + Pump Card */
.hero-tank{
  display:grid;
  grid-template-columns:120px 1fr;
  gap:18px;
  align-items:center;
}
.tank-visual-wrap{
  display:flex;
  flex-direction:column;
  align-items:center;
  gap:8px;
}
.tank-visual{
  width:100px;
  height:140px;
  border:3px solid rgba(56,189,248,0.35);
  border-radius:14px;
  position:relative;
  background:rgba(11,17,32,0.8);
  overflow:hidden;
  box-shadow:inset 0 0 16px rgba(0,0,0,0.6);
}
.tank-water{
  position:absolute;
  bottom:0;left:0;right:0;
  height:40%;
  background:linear-gradient(180deg,#38bdf8 0%,#0369a1 100%);
  transition:height 0.8s cubic-bezier(0.4,0,0.2,1);
}
.tank-water.filling{
  background:linear-gradient(180deg,#34d399 0%,#047857 100%);
}
.tank-water::before{
  content:'';
  position:absolute;
  top:-8px;left:-50%;right:-50%;height:16px;
  background:radial-gradient(circle,rgba(255,255,255,0.4) 20%,transparent 60%);
  animation:wave 2.5s infinite linear;
}
@keyframes wave{
  0%{transform:translateX(0)}
  100%{transform:translateX(50%)}
}
.tank-mark{
  position:absolute;right:4px;
  font-size:0.6rem;color:var(--text-dim);
  font-family:monospace;
}
.tank-mark.top{top:8px}
.tank-mark.mid{top:64px}
.tank-mark.bot{bottom:8px}
.tank-label{
  font-size:0.75rem;
  font-weight:600;
  text-align:center;
  color:var(--text-muted);
}

/* Pump Status Column */
.pump-info{
  display:flex;
  flex-direction:column;
  gap:10px;
}
.pump-status-badge{
  display:inline-flex;
  align-items:center;
  gap:8px;
  padding:6px 12px;
  border-radius:10px;
  font-size:0.85rem;
  font-weight:700;
  letter-spacing:0.3px;
  background:rgba(148,163,184,0.12);
  color:var(--text-muted);
  border:1px solid rgba(148,163,184,0.2);
  width:fit-content;
}
.pump-status-badge.active{
  background:rgba(16,185,129,0.18);
  border-color:rgba(16,185,129,0.5);
  color:#34d399;
  box-shadow:0 0 16px rgba(16,185,129,0.3);
}
.pump-status-badge.interrupted{
  background:rgba(244,63,94,0.18);
  border-color:rgba(244,63,94,0.5);
  color:#f43f5e;
}
.impeller{
  width:16px;height:16px;
  fill:currentColor;
}
.pump-status-badge.active .impeller{
  animation:spin 1.2s infinite linear;
}
@keyframes spin{
  100%{transform:rotate(360deg)}
}
.timer-text{
  font-size:1.35rem;
  font-weight:800;
  font-family:ui-monospace,SFMono-Regular,Menlo,monospace;
  color:#fff;
  letter-spacing:-0.5px;
}
.float-status{
  font-size:0.8rem;
  display:flex;
  align-items:center;
  gap:6px;
  color:var(--text-muted);
}
.float-status span{
  font-weight:600;
  color:var(--text-main);
}
.alarm-banner{
  background:rgba(244,63,94,0.15);
  border:1px solid rgba(244,63,94,0.4);
  color:#fda4af;
  border-radius:10px;
  padding:8px 12px;
  font-size:0.75rem;
  font-weight:600;
  margin-top:6px;
  display:none;
}
.alarm-banner.visible{display:block}

/* Action Controls Grid */
.control-actions{
  display:grid;
  grid-template-columns:1fr 1fr 1fr;
  gap:8px;
  margin-top:14px;
}
.btn-cmd{
  padding:12px 6px;
  border:none;
  border-radius:10px;
  font-size:0.82rem;
  font-weight:700;
  cursor:pointer;
  transition:all 0.2s cubic-bezier(0.4,0,0.2,1);
  display:flex;
  flex-direction:column;
  align-items:center;
  gap:4px;
}
.btn-cmd:active{transform:scale(0.96)}
.btn-auto{
  background:linear-gradient(135deg,rgba(16,185,129,0.25),rgba(5,150,105,0.35));
  border:1px solid rgba(16,185,129,0.4);
  color:#6ee7b7;
}
.btn-auto:hover{background:rgba(16,185,129,0.4);color:#fff}
.btn-start{
  background:linear-gradient(135deg,rgba(56,189,248,0.25),rgba(2,132,199,0.35));
  border:1px solid rgba(56,189,248,0.4);
  color:#7dd3fc;
}
.btn-start:hover{background:rgba(56,189,248,0.4);color:#fff}
.btn-stop{
  background:linear-gradient(135deg,rgba(244,63,94,0.25),rgba(225,29,72,0.35));
  border:1px solid rgba(244,63,94,0.4);
  color:#fda4af;
}
.btn-stop:hover{background:rgba(244,63,94,0.4);color:#fff}

/* PZEM Electrical Telemetry Section */
.telemetry-hd{
  display:flex;
  justify-content:space-between;
  align-items:center;
  margin-bottom:14px;
}
.telemetry-hd h2{
  font-size:0.95rem;
  font-weight:700;
  color:#fff;
  display:flex;align-items:center;gap:6px;
}
.telemetry-grid{
  display:grid;
  grid-template-columns:1fr 1fr;
  gap:10px;
}
.gauge-card{
  background:rgba(11,17,32,0.7);
  border:1px solid rgba(255,255,255,0.06);
  border-radius:14px;
  padding:12px;
  display:flex;
  flex-direction:column;
  gap:4px;
}
.gauge-label{
  font-size:0.7rem;
  font-weight:600;
  text-transform:uppercase;
  letter-spacing:0.5px;
  color:var(--text-dim);
}
.gauge-val{
  font-size:1.45rem;
  font-weight:800;
  font-family:ui-monospace,SFMono-Regular,Menlo,monospace;
  color:#fff;
  letter-spacing:-0.5px;
}
.gauge-val span{font-size:0.8rem;color:var(--text-muted);font-weight:500;margin-left:2px}
.gauge-sub{
  font-size:0.7rem;
  color:var(--text-muted);
}
/* Dynamic Wattage Meter Bar */
.watt-bar-wrap{
  grid-column:1 / -1;
  background:rgba(11,17,32,0.7);
  border:1px solid rgba(255,255,255,0.06);
  border-radius:14px;
  padding:12px;
  display:flex;
  flex-direction:column;
  gap:8px;
}
.watt-bar-bg{
  width:100%;height:8px;
  background:#1e293b;
  border-radius:99px;
  overflow:hidden;
}
.watt-bar-fill{
  height:100%;
  width:0%;
  background:linear-gradient(90deg,#38bdf8,#10b981,#f59e0b);
  border-radius:99px;
  transition:width 0.6s ease;
}

/* Accordion Drawer Section */
.drawer{
  border:1px solid var(--card-border);
  background:rgba(15,23,42,0.5);
  border-radius:14px;
  overflow:hidden;
  transition:border-color 0.2s;
}
.drawer-hd{
  padding:14px 16px;
  display:flex;
  justify-content:space-between;
  align-items:center;
  cursor:pointer;
  user-select:none;
  background:rgba(15,23,42,0.6);
  font-size:0.88rem;
  font-weight:600;
  color:var(--text-main);
}
.drawer-hd:hover{background:rgba(30,41,59,0.5)}
.drawer-arrow{
  width:16px;height:16px;fill:var(--text-muted);
  transition:transform 0.3s;
}
.drawer.open .drawer-arrow{transform:rotate(180deg)}
.drawer-bd{
  display:none;
  padding:16px;
  border-top:1px solid rgba(255,255,255,0.05);
}
.drawer.open .drawer-bd{display:block}

/* Form Styles */
.fg{margin-bottom:12px}
label{display:block;font-size:0.75rem;color:var(--text-muted);margin-bottom:6px;font-weight:600;text-transform:uppercase;letter-spacing:0.5px}
input,select{
  width:100%;
  background:#090d16;
  border:1px solid rgba(255,255,255,0.1);
  border-radius:8px;
  padding:11px 12px;
  color:#fff;
  font-size:0.9rem;
  outline:none;
  transition:border-color 0.2s;
}
input:focus,select:focus{border-color:var(--cyan)}
.row{display:flex;gap:8px}
.btn-primary{
  width:100%;
  background:linear-gradient(135deg,#0284c7,#38bdf8);
  color:#070b14;
  border:none;
  border-radius:8px;
  padding:12px;
  font-size:0.95rem;
  font-weight:700;
  cursor:pointer;
  transition:all 0.2s;
  box-shadow:0 4px 14px rgba(56,189,248,0.3);
}
.btn-primary:hover{background:linear-gradient(135deg,#0369a1,#0284c7);color:#fff}
.btn-secondary{
  background:#1e293b;
  color:var(--text-main);
  border:1px solid rgba(255,255,255,0.1);
  border-radius:8px;
  padding:10px 14px;
  font-size:0.85rem;
  font-weight:600;
  cursor:pointer;
  white-space:nowrap;
}
.btn-secondary:hover{background:#334155}

/* System Details Grid */
.sys-list{
  display:flex;
  flex-direction:column;
  gap:8px;
  font-size:0.8rem;
}
.sys-row{
  display:flex;
  justify-content:space-between;
  padding-bottom:6px;
  border-bottom:1px solid rgba(255,255,255,0.04);
}
.sys-row span:first-child{color:var(--text-dim)}
.sys-row span:last-child{color:var(--text-main);font-family:ui-monospace,SFMono-Regular,monospace}

.msg{
  padding:12px;border-radius:8px;font-size:0.85rem;margin-top:12px;display:none;text-align:center;
}
.msg-success{background:rgba(16,185,129,0.15);border:1px solid #10b981;color:#6ee7b7}
.msg-error{background:rgba(244,63,94,0.15);border:1px solid #f43f5e;color:#fda4af}
.footer{
  text-align:center;color:var(--text-dim);font-size:0.72rem;margin-top:10px;
}
</style>
</head>
<body>
<div class="app-container">

  <!-- Top App Header -->
  <div class="header">
    <div class="brand">
      <div class="brand-icon">
        <svg viewBox="0 0 24 24"><path d="M12 2.69l5.66 5.66a8 8 0 1 1-11.31 0z"/></svg>
      </div>
      <div class="brand-text">
        <h1 id="deviceTitle">Water Pump</h1>
        <p id="ipSubtext">Connecting...</p>
      </div>
    </div>
    <div id="livePill" class="live-pill">
      <span class="pulse-dot"></span>
      <span id="liveText">LIVE</span>
    </div>
  </div>

  <!-- Schema & Connection Details -->
  <div class="schema-bar">
    <div style="display:flex;align-items:center;gap:6px">
      <span style="color:var(--text-dim)">Schema:</span>
      <span id="schemaBadge" class="badge badge-pzem">PROBING...</span>
    </div>
    <div style="display:flex;align-items:center;gap:6px">
      <span style="color:var(--text-dim)">Wi-Fi:</span>
      <span id="wifiSsidBadge" style="font-weight:600;color:#fff">--</span>
    </div>
  </div>

  <!-- Hero Card: Water Tank + Pump State -->
  <div class="card">
    <div class="hero-tank">
      <!-- Water Tank Cylinder Visualizer -->
      <div class="tank-visual-wrap">
        <div class="tank-visual">
          <div class="tank-mark top">FULL</div>
          <div class="tank-mark mid">50%</div>
          <div class="tank-mark bot">LOW</div>
          <div id="tankWater" class="tank-water"></div>
        </div>
        <div id="tankLabel" class="tank-label">NORMAL</div>
      </div>

      <!-- Pump Status & Run Clock -->
      <div class="pump-info">
        <div id="pumpBadge" class="pump-status-badge">
          <svg class="impeller" viewBox="0 0 24 24">
            <path d="M12 2a10 10 0 1 0 10 10A10 10 0 0 0 12 2zm1 14.93V17a1 1 0 0 1-2 0v-.07a4 4 0 0 1-2.86-2.86H8a1 1 0 0 1 0-2h.14A4 4 0 0 1 11 9.21V9a1 1 0 0 1 2 0v.21a4 4 0 0 1 2.86 2.86H16a1 1 0 0 1 0 2h-.14A4 4 0 0 1 13 16.93z"/>
          </svg>
          <span id="pumpBadgeText">STANDBY</span>
        </div>
        <div>
          <div style="font-size:0.7rem;color:var(--text-dim);font-weight:600;text-transform:uppercase">Active Duration</div>
          <div id="pumpTimer" class="timer-text">00:00</div>
        </div>
        <div class="float-status">
          Float Switch: <span id="floatStatusText">Satisfied</span>
        </div>
      </div>
    </div>

    <!-- Dry-run / Safety Warning -->
    <div id="dryRunAlarm" class="alarm-banner">
      ⚠️ <strong>DRY-RUN ALARM:</strong> Continuous max run limit exceeded. Relay stopped to protect motor.
      <button class="btn-secondary" style="margin-top:6px;width:100%;padding:6px" onclick="sendCmd('reset_safety')">Reset Alarm</button>
    </div>

    <!-- Quick Action Controls -->
    <div class="control-actions">
      <button class="btn-cmd btn-auto" onclick="sendCmd('auto')">
        <span style="font-size:1.1rem">🔄</span>
        <span>AUTO</span>
      </button>
      <button class="btn-cmd btn-start" onclick="sendCmd('start')">
        <span style="font-size:1.1rem">▶️</span>
        <span>MANUAL</span>
      </button>
      <button class="btn-cmd btn-stop" onclick="sendCmd('stop')">
        <span style="font-size:1.1rem">⏹️</span>
        <span>STOP</span>
      </button>
    </div>
  </div>

  <!-- PZEM AC Power Meter Telemetry -->
  <div id="pzemCard" class="card">
    <div class="telemetry-hd">
      <h2>
        <svg style="width:18px;height:18px;fill:#a855f7" viewBox="0 0 24 24"><path d="M11 15h2v2h-2zm0-8h2v6h-2zm1-5C6.48 2 2 6.48 2 12s4.48 10 10 10 10-4.48 10-10S17.52 2 12 2zm0 18c-4.41 0-8-3.59-8-8s3.59-8 8-8 8 3.59 8 8-3.59 8-8 8z"/></svg>
        PZEM-004T AC Telemetry
      </h2>
      <span id="acAlertBadge" class="badge" style="background:rgba(16,185,129,0.15);color:#34d399">AC 220V OK</span>
    </div>

    <div class="telemetry-grid">
      <div class="gauge-card">
        <div class="gauge-label">AC Voltage</div>
        <div class="gauge-val" id="valVoltage">0.0 <span>V</span></div>
        <div class="gauge-sub" id="valFreq">50.0 Hz</div>
      </div>
      <div class="gauge-card">
        <div class="gauge-label">Current</div>
        <div class="gauge-val" id="valCurrent">0.00 <span>A</span></div>
        <div class="gauge-sub">RMS Load</div>
      </div>
      <div class="gauge-card">
        <div class="gauge-label">Active Power</div>
        <div class="gauge-val" id="valPower">0.0 <span>W</span></div>
        <div class="gauge-sub" id="valPf">0.00 PF</div>
      </div>
      <div class="gauge-card">
        <div class="gauge-label">Accumulated</div>
        <div class="gauge-val" id="valEnergy">0.00 <span>kWh</span></div>
        <div class="gauge-sub">Total Energy</div>
      </div>
      <div class="watt-bar-wrap">
        <div style="display:flex;justify-content:space-between;font-size:0.72rem">
          <span style="color:var(--text-dim)">House Power Load (1300W Budget)</span>
          <span id="wattPctText" style="color:var(--cyan);font-weight:700">0%</span>
        </div>
        <div class="watt-bar-bg">
          <div id="wattBarFill" class="watt-bar-fill"></div>
        </div>
      </div>
    </div>
  </div>

  <!-- Drawer 1: Wi-Fi Setup & Network Provisioning -->
  <div class="drawer" id="wifiDrawer">
    <div class="drawer-hd" onclick="toggleDrawer('wifiDrawer')">
      <span>📶 Wi-Fi & Station Provisioning</span>
      <svg class="drawer-arrow" viewBox="0 0 24 24"><path d="M7 10l5 5 5-5z"/></svg>
    </div>
    <div class="drawer-bd">
      <form id="setupForm" onsubmit="handleWifiSubmit(event)">
        <div class="fg">
          <label>1. Nearby Wi-Fi</label>
          <div class="row">
            <select id="ssidSelect" onchange="if(this.value)document.getElementById('ssidInput').value=this.value">
              <option value="">-- Tap Scan to Find Networks --</option>
            </select>
            <button type="button" class="btn-secondary" id="scanBtn" onclick="scanWifi()">Scan</button>
          </div>
        </div>
        <div class="fg">
          <label>Network SSID</label>
          <input type="text" id="ssidInput" placeholder="Home Wi-Fi Name" required>
        </div>
        <div class="fg">
          <label>Network Password</label>
          <input type="password" id="passInput" placeholder="Wi-Fi Password">
        </div>
        <div class="fg">
          <label>Device Name (Hostname)</label>
          <input type="text" id="nameInput" placeholder="e.g. Water Pump Ground Floor">
        </div>

        <div style="margin:12px 0 8px 0;font-size:0.8rem;color:var(--cyan);cursor:pointer;text-decoration:underline" onclick="toggleEl('apAdvSec')">
          + Fallback Hotspot Settings (Optional)
        </div>
        <div id="apAdvSec" style="display:none;margin-bottom:12px;padding:10px;background:rgba(0,0,0,0.3);border-radius:8px">
          <div class="fg">
            <label>Fallback SoftAP SSID</label>
            <input type="text" id="apSsidInput" placeholder="Leave blank for CraftThingy-XXXX">
          </div>
          <div class="fg">
            <label>SoftAP WPA2 Password (Min 8 chars)</label>
            <input type="password" id="apPassInput" placeholder="Min 8 characters for security">
          </div>
        </div>

        <button type="submit" class="btn-primary" id="saveWifiBtn">Save & Connect</button>
      </form>
    </div>
  </div>

  <!-- Drawer 2: Hardware & System Maintenance -->
  <div class="drawer" id="diagDrawer">
    <div class="drawer-hd" onclick="toggleDrawer('diagDrawer')">
      <span>⚙️ Hardware & Diagnostics</span>
      <svg class="drawer-arrow" viewBox="0 0 24 24"><path d="M7 10l5 5 5-5z"/></svg>
    </div>
    <div class="drawer-bd">
      <div class="sys-list">
        <div class="sys-row"><span>App Version</span><span id="diagAppVer">1.1.0</span></div>
        <div class="sys-row"><span>Core Version</span><span id="diagCoreVer">--</span></div>
        <div class="sys-row"><span>Hardware Board</span><span id="diagBoard">--</span></div>
        <div class="sys-row"><span>IP Address</span><span id="diagIp">--</span></div>
        <div class="sys-row"><span>MAC Address</span><span id="diagMac">--</span></div>
        <div class="sys-row"><span>Free Memory</span><span id="diagHeap">-- KB</span></div>
        <div class="sys-row"><span>Device Uptime</span><span id="diagUptime">--</span></div>
      </div>
      <div style="display:grid;grid-template-columns:1fr 1fr;gap:8px;margin-top:14px">
        <button class="btn-secondary" onclick="checkOta()">Check OTA</button>
        <button class="btn-secondary" onclick="reprobeHardware()">Re-Probe HW</button>
      </div>
      <button class="btn-secondary" style="width:100%;margin-top:8px;color:#fda4af;border-color:rgba(244,63,94,0.3)" onclick="rebootDevice()">Reboot Device</button>
    </div>
  </div>

  <!-- Drawer 3: Security & Master Secret Key -->
  <div class="drawer" id="secDrawer">
    <div class="drawer-hd" onclick="toggleDrawer('secDrawer')">
      <span>🔐 Master Secret Key / API Auth</span>
      <svg class="drawer-arrow" viewBox="0 0 24 24"><path d="M7 10l5 5 5-5z"/></svg>
    </div>
    <div class="drawer-bd">
      <p style="font-size:0.75rem;color:var(--text-muted);margin-bottom:10px">
        Protected control commands require the Master Key. Save it here to allow direct browser actions.
      </p>
      <div class="fg">
        <label>Master Secret Key</label>
        <input type="password" id="masterKeyInput" placeholder="a1b2c3d4e5f6...">
      </div>
      <button class="btn-secondary" style="width:100%" onclick="saveMasterKey()">Save Key Locally</button>
    </div>
  </div>

  <div id="statusAlert" class="msg"></div>
  <div class="footer">CraftThingy Digital Innovation &copy; 2026 • Firmware v1.1.0</div>
</div>

<script>
var DEFAULT_KEY="a1b2c3d4e5f60718293a4b5c6d7e8f90112233445566778899aabbccddeeff00";
var activeKey=localStorage.getItem('craft_master_key')||DEFAULT_KEY;

function getAuthHeaders(){
  var h={'Content-Type':'application/json'};
  if(activeKey) h['X-Master-Key']=activeKey;
  return h;
}

function toggleDrawer(id){
  var el=document.getElementById(id);
  el.classList.toggle('open');
}
function toggleEl(id){
  var el=document.getElementById(id);
  el.style.display=(el.style.display==='none'||el.style.display==='')?'block':'none';
}
function showAlert(txt,isErr){
  var el=document.getElementById('statusAlert');
  el.className='msg '+(isErr?'msg-error':'msg-success');
  el.textContent=txt;
  el.style.display='block';
  setTimeout(function(){el.style.display='none';},4000);
}

function sendCmd(cmd){
  fetch('/api/v1/device/command',{
    method:'POST',
    headers:getAuthHeaders(),
    body:JSON.stringify({command:cmd})
  }).then(function(r){return r.json();})
  .then(function(d){
    if(d.status==='success') showAlert(d.data.message||'Command executed',false);
    else showAlert(d.error?d.error.message:'Command failed',true);
    pollTelemetry();
  }).catch(function(err){
    showAlert('Network error sending command',true);
  });
}

function scanWifi(){
  var btn=document.getElementById('scanBtn');
  var sel=document.getElementById('ssidSelect');
  btn.disabled=true;
  btn.textContent='...';
  fetch('/api/v1/wifi/scan')
  .then(function(r){return r.json();})
  .then(function(d){
    btn.disabled=false;
    btn.textContent='Scan';
    var nets=Array.isArray(d)?d:(d.data||[]);
    sel.innerHTML='<option value="">-- Found '+nets.length+' Networks --</option>';
    nets.forEach(function(n){
      var opt=document.createElement('option');
      opt.value=n.ssid;
      opt.textContent=n.ssid+(n.rssi?' ('+n.rssi+' dBm)':'');
      sel.appendChild(opt);
    });
  }).catch(function(){
    btn.disabled=false;
    btn.textContent='Scan';
  });
}

function handleWifiSubmit(e){
  e.preventDefault();
  var ssid=document.getElementById('ssidInput').value.trim();
  var pass=document.getElementById('passInput').value;
  var name=document.getElementById('nameInput').value.trim();
  var apSsid=document.getElementById('apSsidInput').value.trim();
  var apPass=document.getElementById('apPassInput').value.trim();
  if(!ssid){alert('Please enter a Wi-Fi SSID');return;}
  var btn=document.getElementById('saveWifiBtn');
  btn.disabled=true;
  btn.textContent='Saving & Connecting...';
  var body=new URLSearchParams();
  body.append('ssid',ssid);
  body.append('pass',pass);
  if(name)body.append('name',name);
  if(apSsid)body.append('ap_ssid',apSsid);
  if(apPass)body.append('ap_pass',apPass);
  fetch('/setup',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:body.toString()})
  .then(function(){
    showAlert('Settings saved! Connecting to '+ssid+'...',false);
    btn.disabled=false;
    btn.textContent='Saved';
  }).catch(function(){
    showAlert('Submitted! Checking connection...',false);
    btn.disabled=false;
    btn.textContent='Save & Connect';
  });
}

function formatSec(s){
  var m=Math.floor(s/60);
  var sec=s%60;
  return (m<10?'0':'')+m+':'+(sec<10?'0':'')+sec;
}

function pollTelemetry(){
  fetch('/api/v1/system/status')
  .then(function(r){return r.json();})
  .then(function(d){
    var sys=d.data?d.data.system:{};
    var net=d.data?d.data.network:{};
    var dev=d.data?d.data.device:{};

    if(sys.name) document.getElementById('deviceTitle').textContent=sys.name;
    var ip=net.ip||'0.0.0.0';
    var isSta=(net.state==='connected');
    document.getElementById('ipSubtext').textContent=(isSta?'IP: '+ip:'SoftAP Mode: '+ip);

    var pill=document.getElementById('livePill');
    var liveText=document.getElementById('liveText');
    if(isSta){
      pill.className='live-pill';
      liveText.textContent='ONLINE';
    }else{
      pill.className='live-pill ap-mode';
      liveText.textContent='AP SETUP';
    }

    if(net.ssid) document.getElementById('wifiSsidBadge').textContent=net.ssid;

    // Pump controller state
    if(dev.hardwareSchema){
      var sBadge=document.getElementById('schemaBadge');
      var sName=dev.hardwareSchema.toUpperCase();
      sBadge.textContent=sName.replace('_',' ');
      sBadge.className=(sName.indexOf('PZEM')>=0)?'badge badge-pzem':'badge badge-legacy';
    }

    // Water level
    var demandsFill=dev.tankDemandsFill;
    var tankEl=document.getElementById('tankWater');
    var tankLbl=document.getElementById('tankLabel');
    if(demandsFill){
      tankEl.style.height='20%';
      tankEl.className='tank-water';
      tankLbl.textContent='LOW WATER';
      tankLbl.style.color='var(--amber)';
      document.getElementById('floatStatusText').textContent='CALLING FOR WATER';
      document.getElementById('floatStatusText').style.color='var(--amber)';
    }else{
      tankEl.style.height='85%';
      tankEl.className='tank-water filling';
      tankLbl.textContent='SATISFIED / FULL';
      tankLbl.style.color='var(--emerald)';
      document.getElementById('floatStatusText').textContent='SATISFIED (HIGH)';
      document.getElementById('floatStatusText').style.color='var(--emerald)';
    }

    // Pump running state
    var isRunning=dev.pumpRunning;
    var pBadge=document.getElementById('pumpBadge');
    var pText=document.getElementById('pumpBadgeText');
    if(isRunning){
      pBadge.className='pump-status-badge active';
      pText.textContent='RUNNING';
    }else if(dev.interrupted){
      pBadge.className='pump-status-badge interrupted';
      pText.textContent='STOPPED';
    }else{
      pBadge.className='pump-status-badge';
      pText.textContent='STANDBY';
    }

    document.getElementById('pumpTimer').textContent=formatSec(dev.runDurationSec||0);

    // Dry run alarm
    var alarmEl=document.getElementById('dryRunAlarm');
    if(dev.dryRunAlarm) alarmEl.className='alarm-banner visible';
    else alarmEl.className='alarm-banner';

    // PZEM AC readings
    var hasPzem=dev.hasPzem;
    var pzemCard=document.getElementById('pzemCard');
    if(!hasPzem){
      pzemCard.style.opacity='0.5';
    }else{
      pzemCard.style.opacity='1';
    }

    document.getElementById('valVoltage').innerHTML=(dev.voltage||0).toFixed(1)+' <span>V</span>';
    document.getElementById('valFreq').textContent=(dev.frequency||50.0).toFixed(1)+' Hz';
    document.getElementById('valCurrent').innerHTML=(dev.current||0).toFixed(3)+' <span>A</span>';
    var pwr=dev.power||0;
    document.getElementById('valPower').innerHTML=pwr.toFixed(1)+' <span>W</span>';
    document.getElementById('valPf').textContent='PF: '+(dev.powerFactor||0).toFixed(2);
    document.getElementById('valEnergy').innerHTML=(dev.energy||0).toFixed(3)+' <span>kWh</span>';

    // 1300W House budget calculation
    var pct=Math.min(100,Math.round((pwr/1300)*100));
    document.getElementById('wattBarFill').style.width=pct+'%';
    document.getElementById('wattPctText').textContent=pct+'% ('+pwr.toFixed(0)+'W / 1300W)';

    var acAlert=document.getElementById('acAlertBadge');
    if(dev.acPowerAlert){
      acAlert.textContent='⚠️ MAINS LOST';
      acAlert.style.background='rgba(244,63,94,0.2)';
      acAlert.style.color='#f43f5e';
    }else{
      acAlert.textContent='AC 220V OK';
      acAlert.style.background='rgba(16,185,129,0.15)';
      acAlert.style.color='#34d399';
    }

    // Diagnostics drawer
    document.getElementById('diagCoreVer').textContent=sys.coreVersion||'--';
    document.getElementById('diagBoard').textContent=sys.boardTarget||'--';
    document.getElementById('diagIp').textContent=net.ip||'--';
    document.getElementById('diagMac').textContent=sys.mac||'--';
    document.getElementById('diagHeap').textContent=Math.round((sys.freeHeap||0)/1024)+' KB';
    var up=d.uptime||0;
    document.getElementById('diagUptime').textContent=Math.floor(up/3600)+'h '+Math.floor((up%3600)/60)+'m '+(up%60)+'s';

  }).catch(function(){});
}

function saveMasterKey(){
  var k=document.getElementById('masterKeyInput').value.trim();
  if(k){
    localStorage.setItem('craft_master_key',k);
    activeKey=k;
    showAlert('Master Key saved to browser storage',false);
  }
}

function reprobeHardware(){
  sendCmd('reprobe');
}
function checkOta(){
  fetch('/api/v1/ota/check',{method:'POST',headers:getAuthHeaders()})
  .then(function(r){return r.json();})
  .then(function(d){showAlert('OTA Check triggered',false);})
  .catch(function(){showAlert('OTA request error',true);});
}
function rebootDevice(){
  if(confirm('Reboot ESP device now?')){
    fetch('/api/v1/system/reboot',{method:'POST',headers:getAuthHeaders()});
    showAlert('Device rebooting...',false);
  }
}

window.onload=function(){
  document.getElementById('masterKeyInput').value=activeKey;
  pollTelemetry();
  setInterval(pollTelemetry,2500);
  setTimeout(scanWifi,800);
};
</script>
</body>
</html>
)rawliteral";
