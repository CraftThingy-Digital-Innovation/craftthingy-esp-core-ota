#pragma once
#include <Arduino.h>

static const char CAPTIVE_PORTAL_HTML[] PROGMEM = R"rawliteral(<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1,maximum-scale=1,user-scalable=no">
<title>CraftThingy Core OTA & Software Switcher</title>
<style>
:root{
  --bg:#070b14;
  --surface:#0f172a;
  --surface-glass:rgba(15,23,42,0.85);
  --card-border:rgba(255,255,255,0.08);
  --cyan:#38bdf8;
  --cyan-dark:#0284c7;
  --emerald:#10b981;
  --amber:#f59e0b;
  --rose:#f43f5e;
  --purple:#a855f7;
  --text-main:#f8fafc;
  --text-muted:#94a3b8;
  --text-dim:#64748b;
}
*{box-sizing:border-box;margin:0;padding:0}
body{
  font-family:system-ui,-apple-system,BlinkMacSystemFont,"Segoe UI",Roboto,sans-serif;
  background:radial-gradient(ellipse at top,#0f1d36 0%,#070b14 70%);
  color:var(--text-main);
  padding:16px 12px 36px 12px;
  min-height:100vh;
  display:flex;
  flex-direction:column;
  align-items:center;
}
.app-container{
  width:100%;
  max-width:500px;
  display:flex;
  flex-direction:column;
  gap:16px;
}
.card{
  background:var(--surface-glass);
  backdrop-filter:blur(14px);
  -webkit-backdrop-filter:blur(14px);
  border:1px solid var(--card-border);
  border-radius:16px;
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
.card h2{
  font-size:1.05rem;
  font-weight:700;
  color:#fff;
  display:flex;
  align-items:center;
  gap:8px;
  margin-bottom:14px;
  padding-bottom:8px;
  border-bottom:1px solid rgba(255,255,255,0.06);
}
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
  width:38px;height:38px;
  border-radius:10px;
  background:linear-gradient(135deg,#0284c7,#38bdf8);
  display:flex;align-items:center;justify-content:center;
  box-shadow:0 0 16px rgba(56,189,248,0.4);
}
.brand-icon svg{width:22px;height:22px;fill:#fff}
.brand-text h1{font-size:1.2rem;font-weight:700;color:#fff}
.brand-text p{font-size:0.75rem;color:var(--text-muted)}
.live-pill{
  display:inline-flex;
  align-items:center;
  gap:6px;
  padding:5px 12px;
  border-radius:99px;
  font-size:0.74rem;
  font-weight:700;
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
.live-pill.ap-mode .pulse-dot{background:#f59e0b;box-shadow:0 0 8px #f59e0b}
@keyframes pulse{0%,100%{opacity:1}50%{opacity:0.3}}

/* Status Grid */
.status-grid{
  display:grid;
  grid-template-columns:1fr 1fr;
  gap:10px;
  font-size:0.82rem;
}
.status-item{
  background:rgba(11,17,32,0.7);
  border:1px solid rgba(255,255,255,0.05);
  padding:10px 12px;
  border-radius:10px;
}
.status-label{
  color:var(--text-dim);
  font-size:0.7rem;
  font-weight:600;
  text-transform:uppercase;
  letter-spacing:0.4px;
}
.status-val{
  font-weight:700;
  margin-top:3px;
  color:#fff;
  font-family:ui-monospace,SFMono-Regular,monospace;
  word-break:break-all;
}
.status-item.full-span{
  grid-column:1 / -1;
}

/* Preset Chips */
.presets-grid{
  display:grid;
  grid-template-columns:1fr 1fr;
  gap:8px;
  margin-bottom:14px;
}
.preset-btn{
  background:rgba(11,17,32,0.8);
  border:1px solid rgba(255,255,255,0.1);
  border-radius:10px;
  padding:10px;
  text-align:left;
  cursor:pointer;
  transition:all 0.2s;
  color:var(--text-main);
}
.preset-btn:hover, .preset-btn.selected{
  border-color:var(--cyan);
  background:rgba(56,189,248,0.12);
}
.preset-name{font-weight:700;font-size:0.85rem;color:#fff}
.preset-desc{font-size:0.72rem;color:var(--text-dim);margin-top:2px}

/* Drawers */
.drawer{
  border:1px solid var(--card-border);
  background:rgba(15,23,42,0.5);
  border-radius:14px;
  overflow:hidden;
}
.drawer-hd{
  padding:14px 16px;
  display:flex;
  justify-content:space-between;
  align-items:center;
  cursor:pointer;
  user-select:none;
  background:rgba(15,23,42,0.6);
  font-size:0.9rem;
  font-weight:600;
}
.drawer-hd:hover{background:rgba(30,41,59,0.5)}
.drawer-arrow{width:16px;height:16px;fill:var(--text-muted);transition:transform 0.3s}
.drawer.open .drawer-arrow{transform:rotate(180deg)}
.drawer-bd{display:none;padding:16px;border-top:1px solid rgba(255,255,255,0.05)}
.drawer.open .drawer-bd{display:block}

/* Forms & Inputs */
.fg{margin-bottom:12px}
label{display:block;font-size:0.74rem;color:var(--text-muted);margin-bottom:6px;font-weight:600;text-transform:uppercase;letter-spacing:0.5px}
input,select{
  width:100%;
  background:#090d16;
  border:1px solid rgba(255,255,255,0.1);
  border-radius:8px;
  padding:11px 12px;
  color:#fff;
  font-size:0.9rem;
  outline:none;
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
  font-size:0.92rem;
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

.msg{padding:12px;border-radius:8px;font-size:0.85rem;margin-top:12px;display:none;text-align:center}
.msg-success{background:rgba(16,185,129,0.15);border:1px solid #10b981;color:#6ee7b7}
.msg-error{background:rgba(244,63,94,0.15);border:1px solid #f43f5e;color:#fda4af}
.footer{text-align:center;color:var(--text-dim);font-size:0.74rem;margin-top:10px}
</style>
</head>
<body>
<div class="app-container">

  <!-- Header -->
  <div class="header">
    <div class="brand">
      <div class="brand-icon">
        <svg viewBox="0 0 24 24"><path d="M12 2L2 7l10 5 10-5-10-5zM2 17l10 5 10-5M2 12l10 5 10-5"/></svg>
      </div>
      <div class="brand-text">
        <h1 id="coreTitle">CraftThingy Core</h1>
        <p id="ipSubtext">Checking...</p>
      </div>
    </div>
    <div id="livePill" class="live-pill">
      <span class="pulse-dot"></span>
      <span id="liveText">ONLINE</span>
    </div>
  </div>

  <!-- Section 1: Live Device Status (Core Telemetry) -->
  <div class="card">
    <h2>
      <svg style="width:18px;height:18px;fill:#38bdf8" viewBox="0 0 24 24"><path d="M13 2.05v3.03c3.39.49 6 3.39 6 6.92 0 .9-.18 1.75-.48 2.54l2.6 1.53c.56-1.24.88-2.62.88-4.07 0-5.18-3.95-9.45-9-9.95zM12 19c-3.87 0-7-3.13-7-7 0-3.53 2.61-6.43 6-6.92V2.05c-5.05.5-9 4.76-9 9.95 0 5.52 4.47 10 9.99 10 3.31 0 6.24-1.61 8.04-4.09l-2.6-1.53C16.18 17.85 14.25 19 12 19z"/></svg>
      2. Live Device Status
    </h2>
    <div class="status-grid">
      <div class="status-item">
        <div class="status-label">Device Name</div>
        <div class="status-val" id="stName">--</div>
      </div>
      <div class="status-item">
        <div class="status-label">Chip Target</div>
        <div class="status-val" id="stTarget">--</div>
      </div>
      <div class="status-item">
        <div class="status-label">Flash Size</div>
        <div class="status-val" id="stFlash">--</div>
      </div>
      <div class="status-item">
        <div class="status-label">Core Version</div>
        <div class="status-val" id="stCoreVer">1.1.0</div>
      </div>
      <div class="status-item">
        <div class="status-label">Active Role</div>
        <div class="status-val" id="stRole" style="color:var(--cyan)">--</div>
      </div>
      <div class="status-item">
        <div class="status-label">WiFi Status</div>
        <div class="status-val" id="stWifi">--</div>
      </div>
      <div class="status-item">
        <div class="status-label">Fallback Hotspot</div>
        <div class="status-val" id="stAp">--</div>
      </div>
      <div class="status-item">
        <div class="status-label">IP Address</div>
        <div class="status-val" id="stIp">--</div>
      </div>
      <div class="status-item">
        <div class="status-label">Rolling Sequence</div>
        <div class="status-val" id="stSeq">--</div>
      </div>
      <div class="status-item">
        <div class="status-label">OTA Status</div>
        <div class="status-val" id="stOta">Idle</div>
      </div>
    </div>
  </div>

  <!-- Section 2: Active Software & Role Switcher -->
  <div class="card">
    <h2>
      <svg style="width:18px;height:18px;fill:#10b981" viewBox="0 0 24 24"><path d="M12 4V1L8 5l4 4V6c3.31 0 6 2.69 6 6 0 1.01-.25 1.97-.7 2.8l1.46 1.46A7.93 7.93 0 0 0 20 12c0-4.42-3.58-8-8-8zm0 14c-3.31 0-6-2.69-6-6 0-1.01.25-1.97.7-2.8L5.24 7.74A7.93 7.93 0 0 0 4 12c0 4.42 3.58 8 8 8v3l4-4-4-4v3z"/></svg>
      Software & Role Switcher
    </h2>
    <p style="font-size:0.75rem;color:var(--text-muted);margin-bottom:12px">
      Select an appliance firmware to download and flash onto this hardware over-the-air from GitHub Releases.
    </p>

    <!-- Quick Module Presets -->
    <div class="presets-grid">
      <div class="preset-btn" onclick="applyPreset('craftthingy-esp-water-pump','water_pump')">
        <div class="preset-name">💧 Water Pump</div>
        <div class="preset-desc">craftthingy-esp-water-pump</div>
      </div>
      <div class="preset-btn" onclick="applyPreset('craftthingy-esp-washing-machine','washing_machine')">
        <div class="preset-name">🧺 Washing Machine</div>
        <div class="preset-desc">craftthingy-esp-washing-machine</div>
      </div>
      <div class="preset-btn" onclick="applyPreset('craftthingy-esp-central-brain','central_brain')">
        <div class="preset-name">🧠 Central Brain</div>
        <div class="preset-desc">1300W Power Arbitrator</div>
      </div>
      <div class="preset-btn" onclick="applyPreset('craftthingy-esp-core-ota','core_ota')">
        <div class="preset-name">⚙️ Reset to Pure Core</div>
        <div class="preset-desc">OTA Shell Only</div>
      </div>
    </div>

    <form id="ghForm" onsubmit="handleSoftwareSwitch(event)">
      <div class="fg">
        <label>GitHub Organization</label>
        <input type="text" id="ghOrgInput" value="CraftThingy-Digital-Innovation" required>
      </div>
      <div class="fg">
        <label>Controller Application Repo</label>
        <input type="text" id="ghRepoInput" placeholder="e.g. craftthingy-esp-water-pump" required>
      </div>
      <div class="fg">
        <label>Active Role Name</label>
        <input type="text" id="ghRoleInput" placeholder="e.g. water_pump" required>
      </div>
      <div class="fg">
        <label>GitHub Token (Optional for Private Repos)</label>
        <input type="password" id="ghTokenInput" placeholder="github_pat_...">
      </div>
      <button type="submit" class="btn-primary" id="switchBtn">Switch Software & Update Now</button>
    </form>
  </div>

  <!-- Section 3: Wi-Fi Provisioning Drawer -->
  <div class="drawer" id="wifiDrawer">
    <div class="drawer-hd" onclick="toggleDrawer('wifiDrawer')">
      <span>📶 Wi-Fi & Station Provisioning</span>
      <svg class="drawer-arrow" viewBox="0 0 24 24"><path d="M7 10l5 5 5-5z"/></svg>
    </div>
    <div class="drawer-bd">
      <form id="wifiForm" onsubmit="handleWifiSubmit(event)">
        <div class="fg">
          <label>1. Nearby Wi-Fi Networks</label>
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
          <input type="text" id="nameInput" placeholder="e.g. CraftThingy-Device">
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

  <!-- Section 4: Security & Master Key Drawer -->
  <div class="drawer" id="secDrawer">
    <div class="drawer-hd" onclick="toggleDrawer('secDrawer')">
      <span>🔐 Master Secret Key & API Security</span>
      <svg class="drawer-arrow" viewBox="0 0 24 24"><path d="M7 10l5 5 5-5z"/></svg>
    </div>
    <div class="drawer-bd">
      <p style="font-size:0.75rem;color:var(--text-muted);margin-bottom:10px">
        Protected commands require the Master Key. Enter it here to authorize browser actions.
      </p>
      <div class="fg">
        <label>Master Secret Key</label>
        <input type="password" id="masterKeyInput" placeholder="a1b2c3d4e5f6...">
      </div>
      <button class="btn-secondary" style="width:100%" onclick="saveMasterKey()">Save Key Locally</button>
    </div>
  </div>

  <!-- Section 5: Hardware Maintenance -->
  <div class="drawer" id="diagDrawer">
    <div class="drawer-hd" onclick="toggleDrawer('diagDrawer')">
      <span>⚙️ Hardware Actions</span>
      <svg class="drawer-arrow" viewBox="0 0 24 24"><path d="M7 10l5 5 5-5z"/></svg>
    </div>
    <div class="drawer-bd">
      <div style="display:grid;grid-template-columns:1fr 1fr;gap:8px">
        <button class="btn-secondary" onclick="checkOta()">Trigger OTA Check</button>
        <button class="btn-secondary" style="color:#fda4af;border-color:rgba(244,63,94,0.3)" onclick="rebootDevice()">Reboot Device</button>
      </div>
    </div>
  </div>

  <div id="statusAlert" class="msg"></div>
  <div class="footer">CraftThingy Digital Innovation &copy; 2026 • Core Framework v1.1.0</div>
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
  document.getElementById(id).classList.toggle('open');
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

function applyPreset(repo,role){
  document.getElementById('ghRepoInput').value=repo;
  document.getElementById('ghRoleInput').value=role;
  showAlert('Preset selected: '+role,false);
}

function pollStatus(){
  fetch('/api/v1/system/status')
  .then(function(r){return r.json();})
  .then(function(d){
    var sys=d.data?d.data.system:{};
    var net=d.data?d.data.network:{};
    var ota=d.data?d.data.ota:{};
    var sec=d.data?d.data.security:{};

    if(sys.name){
      document.getElementById('coreTitle').textContent=sys.name;
      document.getElementById('stName').textContent=sys.name;
    }
    var ip=net.ip||'0.0.0.0';
    var isSta=(net.state==='connected');
    document.getElementById('ipSubtext').textContent=(isSta?'IP: '+ip:'SoftAP: '+ip);

    var pill=document.getElementById('livePill');
    var liveText=document.getElementById('liveText');
    if(isSta){
      pill.className='live-pill';
      liveText.textContent='ONLINE';
    }else{
      pill.className='live-pill ap-mode';
      liveText.textContent='AP SETUP';
    }

    document.getElementById('stTarget').textContent=sys.chip+' ('+(sys.boardTarget||'--')+')';
    document.getElementById('stFlash').textContent=(sys.flashBytes?(sys.flashBytes/(1024*1024)).toFixed(1)+' MB':'1.0 MB');
    document.getElementById('stCoreVer').textContent=sys.coreVersion||'1.1.0';
    document.getElementById('stRole').textContent=ota.activeRole||'unassigned';
    document.getElementById('stWifi').textContent=net.state||'--';
    document.getElementById('stAp').textContent=net.apSsid+' ['+(net.apSecured?'Secured':'Open')+']';
    document.getElementById('stIp').textContent=net.ip||'--';
    document.getElementById('stSeq').textContent=sec.rollingSequence||1;
    document.getElementById('stOta').textContent=ota.status||'Idle';

    if(ota.controllerRepo && !document.getElementById('ghRepoInput').value){
      document.getElementById('ghRepoInput').value=ota.controllerRepo;
    }
    if(ota.activeRole && !document.getElementById('ghRoleInput').value){
      document.getElementById('ghRoleInput').value=ota.activeRole;
    }
  }).catch(function(){});
}

function handleSoftwareSwitch(e){
  e.preventDefault();
  var org=document.getElementById('ghOrgInput').value.trim();
  var repo=document.getElementById('ghRepoInput').value.trim();
  var role=document.getElementById('ghRoleInput').value.trim();
  var token=document.getElementById('ghTokenInput').value.trim();
  var btn=document.getElementById('switchBtn');
  btn.disabled=true;
  btn.textContent='Saving Target & Triggering OTA...';

  var body={
    org:org,
    core_repo:'craftthingy-esp-core-ota',
    controller_repo:repo,
    role:role,
    token:token
  };

  fetch('/api/v1/github/config',{
    method:'POST',
    headers:getAuthHeaders(),
    body:JSON.stringify(body)
  }).then(function(r){return r.json();})
  .then(function(d){
    showAlert('Target updated to '+repo+'! Triggering update download...',false);
    return fetch('/api/v1/ota/check',{method:'POST',headers:getAuthHeaders()});
  }).then(function(){
    showAlert('OTA Firmware download triggered! ESP will restart into '+role+'.',false);
    btn.disabled=false;
    btn.textContent='Switch Software & Update Now';
  }).catch(function(){
    showAlert('Error saving target or triggering OTA',true);
    btn.disabled=false;
    btn.textContent='Switch Software & Update Now';
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
  var btn=document.getElementById('saveWifiBtn');
  btn.disabled=true;
  btn.textContent='Saving...';
  var body=new URLSearchParams();
  body.append('ssid',ssid);
  body.append('pass',pass);
  if(name)body.append('name',name);
  if(apSsid)body.append('ap_ssid',apSsid);
  if(apPass)body.append('ap_pass',apPass);
  fetch('/setup',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:body.toString()})
  .then(function(){
    showAlert('Wi-Fi saved! Connecting to '+ssid+'...',false);
    btn.disabled=false;
    btn.textContent='Saved';
  }).catch(function(){
    showAlert('Settings submitted',false);
    btn.disabled=false;
    btn.textContent='Save & Connect';
  });
}

function saveMasterKey(){
  var k=document.getElementById('masterKeyInput').value.trim();
  if(k){
    localStorage.setItem('craft_master_key',k);
    activeKey=k;
    showAlert('Master Key saved to browser',false);
  }
}
function checkOta(){
  fetch('/api/v1/ota/check',{method:'POST',headers:getAuthHeaders()})
  .then(function(){showAlert('OTA Check triggered',false);})
  .catch(function(){showAlert('OTA Error',true);});
}
function rebootDevice(){
  if(confirm('Reboot ESP device now?')){
    fetch('/api/v1/system/reboot',{method:'POST',headers:getAuthHeaders()});
    showAlert('Device rebooting...',false);
  }
}

window.onload=function(){
  document.getElementById('masterKeyInput').value=activeKey;
  pollStatus();
  setInterval(pollStatus,3000);
  setTimeout(scanWifi,800);
};
</script>
</body>
</html>
)rawliteral";
