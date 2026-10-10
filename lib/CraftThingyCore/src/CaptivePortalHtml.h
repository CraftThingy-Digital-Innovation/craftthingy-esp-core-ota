#pragma once
#include <Arduino.h>

static const char CAPTIVE_PORTAL_HTML[] PROGMEM = R"rawliteral(<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1,maximum-scale=1,user-scalable=no">
<title>CraftThingy Device Setup</title>
<style>
*{box-sizing:border-box;margin:0;padding:0}
body{font-family:-apple-system,BlinkMacSystemFont,"Segoe UI",Roboto,sans-serif;background:#0f172a;color:#f8fafc;padding:16px;min-height:100vh;display:flex;justify-content:center;align-items:flex-start}
.box{width:100%;max-width:440px;background:#1e293b;border:1px solid #334155;border-radius:14px;padding:22px;box-shadow:0 8px 24px rgba(0,0,0,0.4)}
.hd{text-align:center;margin-bottom:20px}
.hd h1{font-size:1.4rem;color:#38bdf8;font-weight:700}
.hd p{color:#94a3b8;font-size:0.85rem;margin-top:4px}
.badge{display:inline-block;padding:2px 8px;border-radius:99px;font-size:0.75rem;font-weight:600;background:rgba(56,189,248,0.15);color:#38bdf8;margin-top:6px}
.fg{margin-bottom:14px}
label{display:block;font-size:0.8rem;color:#94a3b8;margin-bottom:6px;font-weight:500;text-transform:uppercase;letter-spacing:0.5px}
input,select{width:100%;background:#0b1120;border:1px solid #334155;border-radius:8px;padding:12px;color:#f8fafc;font-size:1rem;outline:none;-webkit-appearance:none}
input:focus,select:focus{border-color:#38bdf8}
.btn{width:100%;background:#38bdf8;color:#0f172a;border:none;border-radius:8px;padding:13px;font-size:1rem;font-weight:700;cursor:pointer;margin-top:8px;transition:background .2s}
.btn:hover{background:#0284c7;color:#fff}
.btn-scan{background:#334155;color:#f8fafc;padding:10px 14px;font-size:0.85rem;margin-top:0;width:auto}
.btn-scan:hover{background:#475569}
.row{display:flex;gap:8px}
.sec-toggle{font-size:0.85rem;color:#38bdf8;text-align:center;cursor:pointer;margin-top:14px;padding:6px;text-decoration:underline}
.hidden{display:none}
.msg{padding:12px;border-radius:8px;font-size:0.9rem;margin-top:14px;display:none;text-align:center}
.msg-success{background:rgba(34,197,94,0.15);border:1px solid #22c55e;color:#4ade80}
.msg-error{background:rgba(239,68,68,0.15);border:1px solid #ef4444;color:#f87171}
.footer{text-align:center;color:#64748b;font-size:0.75rem;margin-top:20px}
</style>
</head>
<body>
<div class="box">
  <div class="hd">
    <h1>CraftThingy Setup</h1>
    <p>Offline Mobile Provisioning</p>
    <div class="badge" id="roleBadge">Device Ready</div>
  </div>

  <form id="setupForm" action="/setup" method="POST" onsubmit="handleFormSubmit(event)">
    <div class="fg">
      <label>1. Select Nearby Wi-Fi</label>
      <div class="row">
        <select id="ssidSelect" onchange="onSelectSsid(this)">
          <option value="">-- Choose Wi-Fi Network --</option>
        </select>
        <button type="button" class="btn btn-scan" id="scanBtn" onclick="scanNetworks()">Scan</button>
      </div>
    </div>

    <div class="fg">
      <label>Wi-Fi Name (SSID)</label>
      <input type="text" name="ssid" id="ssidInput" placeholder="Home Wi-Fi Network" required>
    </div>

    <div class="fg">
      <label>Wi-Fi Password</label>
      <input type="password" name="pass" id="passInput" placeholder="Wi-Fi Password">
    </div>

    <div class="fg">
      <label>Internal Device Name</label>
      <input type="text" name="name" id="nameInput" placeholder="e.g. Water Pump Ground Floor">
    </div>

    <div class="sec-toggle" onclick="toggleAdvanced()">+ Fallback Hotspot Settings (Optional)</div>

    <div id="advancedSec" class="hidden" style="margin-top:12px;padding-top:12px;border-top:1px dashed #334155;">
      <div class="fg">
        <label>Fallback Hotspot SSID</label>
        <input type="text" name="ap_ssid" id="apSsidInput" placeholder="Leave blank for default">
      </div>
      <div class="fg">
        <label>Fallback WPA2 Password</label>
        <input type="password" name="ap_pass" id="apPassInput" placeholder="Min 8 chars, or blank for open">
      </div>
    </div>

    <button type="submit" class="btn" id="submitBtn">Connect to Home Wi-Fi</button>
  </form>

  <div id="statusBox" class="msg"></div>
  <div class="footer">CraftThingy Digital Innovation &copy; 2026</div>
</div>

<script>
function toggleAdvanced(){
  var el=document.getElementById('advancedSec');
  el.classList.toggle('hidden');
}
function onSelectSsid(sel){
  if(sel.value) document.getElementById('ssidInput').value=sel.value;
}
function showMsg(txt,isErr){
  var el=document.getElementById('statusBox');
  el.className='msg '+(isErr?'msg-error':'msg-success');
  el.textContent=txt;
  el.style.display='block';
}
function scanNetworks(){
  var btn=document.getElementById('scanBtn');
  var sel=document.getElementById('ssidSelect');
  btn.disabled=true;
  btn.textContent='...';
  fetch('/api/v1/wifi/scan').then(function(r){return r.json()}).then(function(res){
    btn.disabled=false;
    btn.textContent='Scan';
    var nets=Array.isArray(res)?res:(res.data||[]);
    sel.innerHTML='<option value="">-- Choose Wi-Fi ('+nets.length+' found) --</option>';
    nets.forEach(function(n){
      var opt=document.createElement('option');
      opt.value=n.ssid;
      opt.textContent=n.ssid+(n.rssi?' ('+n.rssi+' dBm)':'');
      sel.appendChild(opt);
    });
  }).catch(function(e){
    btn.disabled=false;
    btn.textContent='Scan';
  });
}
function handleFormSubmit(e){
  e.preventDefault();
  var ssid=document.getElementById('ssidInput').value.trim();
  var pass=document.getElementById('passInput').value;
  var name=document.getElementById('nameInput').value.trim();
  var apSsid=document.getElementById('apSsidInput').value.trim();
  var apPass=document.getElementById('apPassInput').value.trim();
  if(!ssid){alert('Please enter your Wi-Fi SSID');return;}
  var btn=document.getElementById('submitBtn');
  btn.disabled=true;
  btn.textContent='Saving & Connecting...';
  showMsg('Credentials received! ESP is now connecting to '+ssid+'...',false);
  var body=new URLSearchParams();
  body.append('ssid',ssid);
  body.append('pass',pass);
  if(name)body.append('name',name);
  if(apSsid)body.append('ap_ssid',apSsid);
  if(apPass)body.append('ap_pass',apPass);
  fetch('/setup',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:body.toString()})
  .then(function(r){return r.json()})
  .then(function(d){
    showMsg('Connected! Reconnecting to '+ssid+'... Check your router.',false);
  }).catch(function(err){
    showMsg('Settings sent! The ESP is switching to '+ssid+'.',false);
  });
}
window.onload=function(){
  fetch('/api/v1/system/status').then(function(r){return r.json()}).then(function(d){
    var info=(d.data&&d.data.system)||d.system||{};
    var role=(d.data&&d.data.ota&&d.data.ota.activeRole)||info.activeRole;
    if(info.name)document.getElementById('nameInput').value=info.name;
    if(role)document.getElementById('roleBadge').textContent=role.toUpperCase();
  }).catch(function(){});
  setTimeout(scanNetworks,500);
};
</script>
</body>
</html>
)rawliteral";
