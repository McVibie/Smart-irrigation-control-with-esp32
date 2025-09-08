#pragma once
const char INDEX_HTML2[] PROGMEM = R"HTML(
<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Smart Irrigation Control</title>
<style>
  @import url('https://fonts.googleapis.com/css2?family=Inter:wght@400;500;600;700;800&display=swap');
  :root{
    --bg-primary: #0a0e27;
    --bg-secondary: #111730;
    --card-bg: #1a2142;
    --card-border: #2a3356;
    --text-primary: #ffffff;
    --text-secondary: #94a3b8;
    --text-muted: #64748b;
    --accent-blue: #3b82f6;
    --accent-green: #10b981;
    --accent-yellow: #f59e0b;
    --accent-red: #ef4444;
    --accent-purple: #8b5cf6;
    --gradient-blue: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
    --gradient-green: linear-gradient(135deg, #10b981 0%, #059669 100%);
    --gradient-red: linear-gradient(135deg, #f43f5e 0%, #dc2626 100%);
    --shadow-lg: 0 10px 40px rgba(0,0,0,0.5);
    --shadow-xl: 0 20px 60px rgba(0,0,0,0.7);
  }
  * { box-sizing: border-box; margin: 0; padding: 0; }
  body { font-family: 'Inter', system-ui, -apple-system, sans-serif; background: linear-gradient(180deg, var(--bg-primary) 0%, var(--bg-secondary) 100%); color: var(--text-primary); min-height: 100vh; line-height: 1.6; }
  header { background: rgba(26, 33, 66, 0.8); backdrop-filter: blur(10px); border-bottom: 1px solid var(--card-border); padding: 1.25rem 1.5rem; position: sticky; top: 0; z-index: 100; box-shadow: 0 4px 20px rgba(0,0,0,0.3); }
  .header-content { max-width: 1400px; margin: 0 auto; display: flex; justify-content: space-between; align-items: center; gap: 1rem; }
  h1 { font-size: 1.5rem; font-weight: 700; background: linear-gradient(135deg, #667eea 0%, #10b981 100%); -webkit-background-clip: text; -webkit-text-fill-color: transparent; display: flex; align-items: center; gap: 0.5rem; }
  .status-badge { display: flex; align-items: center; gap: 0.5rem; padding: 0.5rem 1rem; background: rgba(59, 130, 246, 0.1); border: 1px solid rgba(59, 130, 246, 0.3); border-radius: 999px; font-size: 0.875rem; font-weight: 500; animation: pulse 2s infinite; }
  @keyframes pulse { 0%, 100% { opacity: 1; } 50% { opacity: 0.8; } }
  .status-dot { width: 8px; height: 8px; border-radius: 50%; background: var(--accent-green); animation: blink 2s infinite; }
  @keyframes blink { 0%, 100% { opacity: 1; } 50% { opacity: 0.3; } }
  /* Make layout fluid to autofit browser size */
  .container { max-width: 100%; margin: 0 auto; padding: 1.2rem; }
  .grid { display: grid; gap: 1.5rem; margin-bottom: 1.5rem; }
  .grid-2 { grid-template-columns: repeat(auto-fit, minmax(320px, 1fr)); }
  .grid-3 { grid-template-columns: repeat(auto-fit, minmax(280px, 1fr)); }
  .grid-4 { grid-template-columns: repeat(auto-fit, minmax(240px, 1fr)); }
  .card { background: var(--card-bg); border: 1px solid var(--card-border); border-radius: 1rem; padding: 1.5rem; box-shadow: var(--shadow-lg); transition: all 0.3s ease; position: relative; overflow: hidden; }
  .card::before { content: ''; position: absolute; top: 0; left: 0; right: 0; height: 3px; background: var(--gradient-blue); opacity: 0; transition: opacity 0.3s ease; }
  .card:hover { transform: translateY(-2px); box-shadow: var(--shadow-xl); }
  .card:hover::before { opacity: 1; }
  .card-header { display: flex; justify-content: space-between; align-items: center; margin-bottom: 1rem; }
  .card-title { font-size: 0.875rem; font-weight: 600; color: var(--text-secondary); text-transform: uppercase; letter-spacing: 0.05em; }
  .card-badge { padding: 0.25rem 0.75rem; background: rgba(59, 130, 246, 0.1); border: 1px solid rgba(59, 130, 246, 0.2); border-radius: 999px; font-size: 0.75rem; font-weight: 500; color: var(--accent-blue); }
  .card-value { font-size: 2.25rem; font-weight: 700; margin-bottom: 0.5rem; background: linear-gradient(135deg, #fff 0%, #94a3b8 100%); -webkit-background-clip: text; -webkit-text-fill-color: transparent; }
  .card-subtitle { font-size: 0.875rem; color: var(--text-muted); }
  .sensor-grid { display: grid; grid-template-columns: repeat(auto-fill, minmax(180px, 1fr)); gap: 1rem; }
  .sensor-card { background: rgba(26, 33, 66, 0.5); border: 1px solid var(--card-border); border-radius: 0.75rem; padding: 1rem; text-align: center; transition: all 0.3s ease; }
  .sensor-card:hover { background: rgba(26, 33, 66, 0.8); transform: scale(1.05); }
  .sensor-name { font-size: 0.75rem; color: var(--text-muted); margin-bottom: 0.5rem; white-space: nowrap; overflow: hidden; text-overflow: ellipsis; }
  .sensor-value { font-size: 1.75rem; font-weight: 700; margin-bottom: 0.5rem; }
  .sensor-bar { height: 4px; background: rgba(148, 163, 184, 0.1); border-radius: 999px; overflow: hidden; }
  .sensor-fill { height: 100%; border-radius: 999px; transition: all 0.5s ease; }
  .pump-card { background: var(--card-bg); border: 1px solid var(--card-border); border-radius: 1rem; padding: 1.25rem; margin-bottom: 1rem; transition: all 0.3s ease; }
  .pump-card.locked { background: rgba(239, 68, 68, 0.05); border-color: rgba(239, 68, 68, 0.3); }
  .pump-header { display: flex; justify-content: space-between; align-items: center; margin-bottom: 1rem; }
  .pump-title { font-size: 1.125rem; font-weight: 600; display: flex; align-items: center; gap: 0.5rem; }
  .pump-sensors { font-size: 0.75rem; color: var(--text-muted); }
  .pump-status { display: flex; gap: 0.5rem; flex-wrap: wrap; }
  .status-chip { padding: 0.25rem 0.75rem; border-radius: 999px; font-size: 0.75rem; font-weight: 500; }
  .chip-enabled { background: rgba(16, 185, 129, 0.1); border: 1px solid rgba(16, 185, 129, 0.3); color: var(--accent-green); }
  .chip-disabled { background: rgba(148, 163, 184, 0.1); border: 1px solid rgba(148, 163, 184, 0.3); color: var(--text-muted); }
  .chip-manual { background: rgba(139, 92, 246, 0.1); border: 1px solid rgba(139, 92, 246, 0.3); color: var(--accent-purple); }
  .chip-auto { background: rgba(59, 130, 246, 0.1); border: 1px solid rgba(59, 130, 246, 0.3); color: var(--accent-blue); }
  .chip-locked { background: rgba(239, 68, 68, 0.1); border: 1px solid rgba(239, 68, 68, 0.3); color: var(--accent-red); animation: pulse 1s infinite; }
  .pump-controls { display: grid; grid-template-columns: 1fr auto; gap: 1rem; align-items: center; margin-bottom: 1rem; }
  .slider-container { display: flex; align-items: center; gap: 1rem; }
  .slider { flex: 1; -webkit-appearance: none; appearance: none; height: 6px; border-radius: 999px; background: rgba(148, 163, 184, 0.2); outline: none; transition: all 0.3s ease; }
  .slider:hover { background: rgba(148, 163, 184, 0.3); }
  .slider::-webkit-slider-thumb { -webkit-appearance: none; appearance: none; width: 20px; height: 20px; border-radius: 50%; background: var(--accent-blue); cursor: pointer; transition: all 0.3s ease; box-shadow: 0 2px 10px rgba(59, 130, 246, 0.5); }
  .slider::-webkit-slider-thumb:hover { transform: scale(1.2); box-shadow: 0 4px 20px rgba(59, 130, 246, 0.7); }
  .slider:disabled { opacity: 0.5; cursor: not-allowed; }
  .slider-value { min-width: 60px; text-align: right; font-weight: 600; font-size: 1.125rem; }
  .pump-buttons { display: flex; gap: 0.5rem; }
  .pump-settings { display: grid; grid-template-columns: 1fr auto; gap: 0.5rem; align-items: center; padding: 0.75rem; background: rgba(26, 33, 66, 0.3); border-radius: 0.5rem; margin-top: 0.5rem; }
  .setting-label { font-size: 0.875rem; color: var(--text-secondary); }
  .setting-input { display: flex; gap: 0.5rem; align-items: center; }
  /* City search group */
  .city-group{ display:flex; gap:.5rem; align-items:center; }
  .city-input{ flex:0 0 150px; min-width:120px; }
  .btn { padding: 0.5rem 1rem; border-radius: 0.5rem; font-weight: 500; font-size: 0.875rem; border: none; cursor: pointer; transition: all 0.3s ease; outline: none; }
  .btn-primary { background: var(--accent-blue); color: white; }
  .btn-primary:hover { background: #2563eb; transform: translateY(-1px); box-shadow: 0 4px 12px rgba(59, 130, 246, 0.4); }
  .btn-success { background: var(--accent-green); color: white; }
  .btn-success:hover { background: #059669; transform: translateY(-1px); box-shadow: 0 4px 12px rgba(16, 185, 129, 0.4); }
  .btn-danger { background: var(--accent-red); color: white; }
  .btn-danger:hover { background: #dc2626; transform: translateY(-1px); box-shadow: 0 4px 12px rgba(239, 68, 68, 0.4); }
  .btn-ghost { background: transparent; color: var(--text-secondary); border: 1px solid var(--card-border); }
  .btn-ghost:hover { background: rgba(148, 163, 184, 0.1); border-color: var(--text-secondary); }
  .btn-ghost.active { background: var(--accent-blue); color: white; border-color: var(--accent-blue); }
  .btn:disabled { opacity: 0.5; cursor: not-allowed; }
  .btn-sm { padding: .35rem .6rem; font-size: .8rem; border-radius: .4rem; }
  input[type="text"], input[type="number"] { padding: 0.5rem 0.75rem; background: rgba(26, 33, 66, 0.5); border: 1px solid var(--card-border); border-radius: 0.5rem; color: var(--text-primary); font-size: 0.875rem; transition: all 0.3s ease; outline: none; }
  input[type="text"]:focus, input[type="number"]:focus { border-color: var(--accent-blue); background: rgba(26, 33, 66, 0.8); box-shadow: 0 0 0 3px rgba(59, 130, 246, 0.1); }
  input[type="number"] { width: 80px; }
  .weather-widget { display: grid; grid-template-columns: auto 1fr; gap: 1.5rem; align-items: center; }
  .weather-icon { width: 80px; height: 80px; background: var(--gradient-blue); border-radius: 1rem; display: flex; align-items: center; justify-content: center; font-size: 2.5rem; }
  .weather-details { display: grid; gap: 0.5rem; }
  .weather-temp { font-size: 2.5rem; font-weight: 700; line-height: 1; }
  .weather-desc { font-size: 1rem; color: var(--text-secondary); }
  .weather-stats { display: flex; gap: 1rem; margin-top: 0.5rem; }
  .weather-stat { display: flex; align-items: center; gap: 0.25rem; font-size: 0.875rem; color: var(--text-muted); }
  
  .forecast-list { display: grid; gap: 0.5rem; max-height: 400px; overflow-y: auto; padding-right: 0.5rem; }
  .forecast-item { display: grid; grid-template-columns: 100px 40px 1fr auto; gap: 1rem; align-items: center; padding: 0.75rem; background: rgba(26, 33, 66, 0.3); border-radius: 0.5rem; transition: all 0.3s ease; }
  .forecast-item:hover { background: rgba(26, 33, 66, 0.5); }
  .forecast-time { font-size: 0.875rem; color: var(--text-muted); }
  .forecast-icon { width: 32px; height: 32px; }
  .forecast-desc { font-size: 0.875rem; color: var(--text-secondary); }
  .forecast-temp { font-weight: 600; font-size: 1rem; }
  .toast { position: fixed; bottom: 2rem; right: 2rem; padding: 1rem 1.5rem; background: var(--card-bg); border: 1px solid var(--card-border); border-radius: 0.75rem; box-shadow: var(--shadow-xl); display: flex; align-items: center; gap: 0.75rem; min-width: 300px; transform: translateY(120%); opacity: 0; transition: all 0.3s ease; z-index: 1000; }
  .toast.show { transform: translateY(0); opacity: 1; }
  .toast-icon { width: 20px; height: 20px; border-radius: 50%; display: flex; align-items: center; justify-content: center; flex-shrink: 0; }
  .toast.success .toast-icon { background: var(--accent-green); }
  .toast.error .toast-icon { background: var(--accent-red); }
  .toast.info .toast-icon { background: var(--accent-blue); }
  .toast-message { flex: 1; font-size: 0.875rem; }
  .timer { display: inline-flex; align-items: center; gap: 0.25rem; padding: 0.25rem 0.5rem; background: rgba(239, 68, 68, 0.1); border-radius: 0.25rem; font-size: 0.75rem; color: var(--accent-red); font-weight: 500; }
  .timer.manual { background: rgba(139, 92, 246, 0.1); color: var(--accent-purple); }
  @media (max-width: 768px) { .header-content { flex-direction: column; text-align: center; } .grid-2, .grid-3, .grid-4 { grid-template-columns: 1fr; } .sensor-grid { grid-template-columns: repeat(auto-fill, minmax(150px, 1fr)); } .pump-controls { grid-template-columns: 1fr; } .pump-buttons { justify-content: stretch; } .btn { flex: 1; } }
  footer { text-align: center; padding: 2rem 1.5rem; color: var(--text-muted); font-size: 0.875rem; border-top: 1px solid var(--card-border); margin-top: 3rem; }

  /* Mobile refinements */
  @media (max-width: 480px) {
    .container { padding: 0.9rem; }
    header { padding: 0.9rem 1rem; }
    .card { padding: 1rem; }
    .card-header { flex-wrap: wrap; gap: .5rem; }
    .card-title { font-size: 0.75rem; }
    .card-value { font-size: 1.8rem; }
    .weather-widget { grid-template-columns: 1fr; gap: .75rem; }
    .status-badge { flex-wrap: wrap; row-gap: .25rem; }
    .setting-input { flex-wrap: wrap; gap: .4rem; }
    input[type="text"], input[type="number"] { width: 100%; }
    .city-group{ flex-direction:column; align-items:stretch; }
    .city-group .btn{ width:100%; }
    .pump-header { flex-wrap: wrap; gap: .5rem; }
    .pump-controls { grid-template-columns: 1fr; gap: .75rem; }
    .pump-buttons { flex-direction: column; }
    .btn { width: 100%; }
    .slider-value { min-width: auto; font-size: 1rem; }
    .forecast-list { max-height: 300px; }
  }

  /* Irrigation card polish */
  #irrigation-status-card{ border:1px solid rgba(148,163,184,.18); background:linear-gradient(180deg, rgba(26,33,66,.7), rgba(17,23,48,.6)); }
  #irrigation-status-card .card-value{ font-size:2rem; }
  #irrigation-status-card .pump-settings{ background:rgba(26,33,66,.45); border:1px solid var(--card-border); border-radius:.6rem; }
  #irrigation-status-card .setting-label{ min-width:160px; }
</style>
</head>
<body>
<header>
  <div class="header-content">
    <h1 id="app-title"><span>🌱</span><span>Smart Irrigation Control</span></h1>
    <div style="display:flex; gap:.5rem; align-items:center; flex-wrap:wrap">
      <div class="status-badge"><span class="status-dot"></span><span id="connection-status">Connected</span><span>•</span><span id="ip">connecting…</span></div>
      <select id="lang-select" class="btn-ghost" style="padding:.4rem .6rem; border-radius:.5rem">
        <option value="en">English</option>
        <option value="hr">Hrvatski</option>
      </select>
    </div>
  </div>
</header>
<div class="container">
  <div class="grid grid-3">
    <div class="card">
      <div class="card-header"><h3 class="card-title" id="title-weather">Current Weather</h3><span class="card-badge" id="weather-city">Loading...</span></div>
      <div class="weather-widget"><div class="weather-icon">☀️</div><div class="weather-details"><div style="display:flex; align-items:center; justify-content:space-between; gap:.5rem"><div class="weather-temp" id="weather-temp">--°C</div><div class="city-group"><input id="city-input" class="city-input" type="text" placeholder="Search city or lat,lon"/><button id="btn-set-city" class="btn btn-primary btn-sm" onclick="applyCity()">Set</button></div></div><div class="weather-desc" id="weather-desc">—</div><div class="weather-stats"><div class="weather-stat">💧 <span id="weather-humidity">--%</span></div></div></div></div>
      
    </div>
    <div class="card">
      <div class="card-header"><h3 class="card-title" id="title-air">Air Conditions</h3><span class="card-badge" id="badge-from">From Transmitter</span></div>
      <div style="display: grid; gap: 1rem;"><div><div class="card-subtitle" id="label-air-temp">Temperature</div><div class="card-value" id="air-temp">--°C</div></div><div><div class="card-subtitle" id="label-air-hum">Humidity</div><div class="card-value" id="air-humidity">--%</div></div></div>
    </div>
    <div class="card">
      <div class="card-header"><h3 class="card-title" id="title-power">Power Monitor</h3><span class="card-badge">INA219</span></div>
      <div style="display: grid; gap: 0.75rem;">
        <div style="display: flex; justify-content: space-between;"><span class="card-subtitle" id="label-voltage">Voltage</span><span style="font-weight: 600;" id="power-voltage">-- V</span></div>
        <div style="display: flex; justify-content: space-between;"><span class="card-subtitle" id="label-current">Current</span><span style="font-weight: 600;" id="power-current">-- mA</span></div>
        <div style="display: flex; justify-content: space-between;"><span class="card-subtitle" id="label-power">Power</span><span style="font-weight: 600;" id="power-power">-- mW</span></div>
      </div>
    </div>
  </div>
  <div class="grid grid-2">
    <div class="card" id="irrigation-status-card">
      <div class="card-header"><h3 class="card-title" id="title-irrig">🌧️ Irrigation Status</h3><span class="card-badge" id="irrigation-status-badge">Checking...</span></div>
      <div style="display: grid; gap: 1rem;">
        <div>
          <div class="card-value" id="irrigation-status">Checking...</div>
          <div class="card-subtitle" id="irrigation-reason">Evaluating conditions...</div>
        </div>
        <div class="pump-settings">
          <span class="setting-label" id="label-max-temp">Max Temperature Limit</span>
          <div class="setting-input">
            <input type="number" id="max-temp-input" step="0.5" min="-20" max="60" value="22">
            <span>°C</span>
            <button id="btn-save-maxtemp" class="btn btn-primary" onclick="saveMaxTemp()">Save</button>
          </div>
        </div>
        <div class="pump-settings">
          <span class="setting-label" id="label-durations">Cutoff Lockout & Manual Timeout</span>
          <div class="setting-input">
            <input type="number" id="lockout-min" min="1" max="240" step="1" value="10"> <span>min</span>
            <span style="opacity:.7;margin:0 .5rem;">•</span>
            <input type="number" id="manual-min" min="1" max="240" step="1" value="20"> <span>min</span>
            <button id="btn-save-durations" class="btn btn-primary" onclick="saveDurations()">Save</button>
          </div>
        </div>
      </div>
    </div>
    <div class="card"><div class="card-header"><h3 class="card-title" id="title-forecast">36-Hour Forecast</h3><span class="card-badge" id="forecast-city">—</span></div><div class="forecast-list" id="forecast-list"></div></div>
  </div>
  <div class="card"><div class="card-header"><h3 class="card-title" id="title-pumps">🚰 Pump Control System</h3><span class="card-badge" id="pump-system-status">All Systems Normal</span></div><div id="pump-controls"></div></div>
  <div class="card"><div class="card-header"><h3 class="card-title" id="title-sensors">💧 Soil Moisture Sensors</h3><span class="card-badge" id="badge-live">Live Data</span></div><div class="sensor-grid" id="sensor-grid"></div></div>
  <div class="card"><div class="card-header"><h3 class="card-title" id="title-config">⚙️ Sensor Configuration</h3><span class="card-badge" id="badge-persist">Persistent Storage</span></div><div style="display: grid; grid-template-columns: repeat(auto-fit, minmax(200px, 1fr)); gap: 0.75rem;" id="sensor-names"></div><div style="display: flex; justify-content: flex-end; margin-top: 1rem;"><button class="btn btn-success" id="btn-save-names" onclick="saveNames()">Save All Names</button></div></div>
</div>

<footer><div>Smart Irrigation System v2.0 • Real-time updates every 200ms</div><div style="margin-top: 0.5rem; opacity: 0.7;">Designed with ❤️ for sustainable farming</div></footer>

<div id="toast" class="toast"><div class="toast-icon"></div><div class="toast-message"></div></div>

<script>
let maxTempSetting = 22.0; let rainNext24h = false; let lastTempC = 20.0; let sensorNames = [];
let gForecastLat = null, gForecastLon = null;
// i18n dictionaries (minimal set)
const i18n = {
  en: {
    app: 'Smart Irrigation Control', connected: 'Connected', fromTx: 'From Transmitter',
    currentWeather: 'Current Weather', air: 'Air Conditions', power: 'Power Monitor',
    voltage: 'Voltage', current: 'Current', powerW: 'Power',
    irrig: 'Irrigation Status', inhibited:'INHIBITED', checking: 'Checking...', maxTempLimit: 'Max Temperature Limit',
    durations: 'Cutoff Lockout & Manual Timeout', forecast: '36-Hour Forecast',
    pumps: 'Pump Control System', sensors: 'Soil Moisture Sensors', live: 'Live Data',
    config: 'Sensor Configuration', persist: 'Persistent Storage', saveNames: 'Save All Names',
    searchCity: 'Search city or lat,lon', btnSet:'Set', btnSave:'Save',
    enabled:'ENABLED', disabled:'DISABLED', manual:'MANUAL', auto:'AUTO', pump:'Pump',
    btnEnable:'Enable', btnDisable:'Disable', btnManual:'Manual', btnAuto:'Auto'
  },
  hr: {
    app: 'Pametno Navodnjavanje', connected: 'Povezano', fromTx: 'S predajnika',
    currentWeather: 'Trenutno vrijeme', air: 'Uvjeti zraka', power: 'Nadzor napajanja',
    voltage: 'Napon', current: 'Struja', powerW: 'Snaga',
    irrig: 'Status navodnjavanja', inhibited:'INHIBIRANO', checking: 'Provjera...', maxTempLimit: 'Maksimalna temperatura',
    durations: 'Zaključavanje i ručni timeout', forecast: 'Prognoza 36h',
    pumps: 'Upravljanje pumpama', sensors: 'Senzori vlage tla', live: 'Uživo',
    config: 'Konfiguracija senzora', persist: 'Trajna pohrana', saveNames: 'Spremi nazive',
    searchCity: 'Traži grad ili lat,lon', btnSet:'Postavi', btnSave:'Spremi',
    enabled:'UKLJUČENA', disabled:'ISKLJUČENA', manual:'RUČNO', auto:'AUTO', pump:'Pumpa',
    btnEnable:'Uključi', btnDisable:'Isključi', btnManual:'Ručno', btnAuto:'Auto'
  }
};
let lang = localStorage.getItem('lang') || 'en';
const elements = { ip: document.getElementById('ip'), connectionStatus: document.getElementById('connection-status'), weatherCity: document.getElementById('weather-city'), weatherTemp: document.getElementById('weather-temp'), weatherDesc: document.getElementById('weather-desc'), weatherHumidity: document.getElementById('weather-humidity'), airTemp: document.getElementById('air-temp'), airHumidity: document.getElementById('air-humidity'), powerVoltage: document.getElementById('power-voltage'), powerCurrent: document.getElementById('power-current'), powerPower: document.getElementById('power-power'), irrigationStatus: document.getElementById('irrigation-status'), irrigationReason: document.getElementById('irrigation-reason'), irrigationBadge: document.getElementById('irrigation-status-badge'), irrigationCard: document.getElementById('irrigation-status-card'), maxTempInput: document.getElementById('max-temp-input'), forecastList: document.getElementById('forecast-list'), pumpControls: document.getElementById('pump-controls'), sensorGrid: document.getElementById('sensor-grid'), sensorNamesContainer: document.getElementById('sensor-names'), toast: document.getElementById('toast') };
function showToast(message, type='info'){ const toast=elements.toast, icon=toast.querySelector('.toast-icon'), msg=toast.querySelector('.toast-message'); if(type==='success'){icon.innerHTML='✓'; toast.className='toast success';} else if(type==='error'){icon.innerHTML='✕'; toast.className='toast error';} else {icon.innerHTML='ℹ'; toast.className='toast info';} msg.textContent=message; toast.classList.add('show'); setTimeout(()=>toast.classList.remove('show'),3000); }
function applyI18n(){
  const t = i18n[lang] || i18n.en;
  document.getElementById('app-title')?.querySelectorAll('span')[1].replaceWith((()=>{ const s=document.createElement('span'); s.textContent=t.app; return s; })());
  document.getElementById('connection-status')?.replaceChildren(document.createTextNode(t.connected));
  document.getElementById('title-weather')?.replaceChildren(document.createTextNode(t.currentWeather));
  const cityInp = document.getElementById('city-input'); if (cityInp) cityInp.placeholder = t.searchCity;
  document.getElementById('btn-set-city')?.replaceChildren(document.createTextNode(t.btnSet || 'Set'));
  document.getElementById('title-air')?.replaceChildren(document.createTextNode(t.air));
  document.getElementById('badge-from')?.replaceChildren(document.createTextNode(t.fromTx));
  document.getElementById('label-air-temp')?.replaceChildren(document.createTextNode(t.voltage ? 'Temperature' : 'Temperature'));
  document.getElementById('label-air-hum')?.replaceChildren(document.createTextNode('Humidity'));
  document.getElementById('title-power')?.replaceChildren(document.createTextNode(t.power));
  document.getElementById('label-voltage')?.replaceChildren(document.createTextNode(t.voltage));
  document.getElementById('label-current')?.replaceChildren(document.createTextNode(t.current));
  document.getElementById('label-power')?.replaceChildren(document.createTextNode(t.powerW));
  document.getElementById('title-irrig')?.replaceChildren(document.createTextNode(t.irrig));
  document.getElementById('irrigation-status-badge')?.replaceChildren(document.createTextNode(t.inhibited));
  document.getElementById('label-max-temp')?.replaceChildren(document.createTextNode(t.maxTempLimit));
  document.getElementById('label-durations')?.replaceChildren(document.createTextNode(t.durations));
  document.getElementById('btn-save-maxtemp')?.replaceChildren(document.createTextNode(t.btnSave || 'Save'));
  document.getElementById('btn-save-durations')?.replaceChildren(document.createTextNode(t.btnSave || 'Save'));
  document.getElementById('title-forecast')?.replaceChildren(document.createTextNode(t.forecast));
  document.getElementById('title-pumps')?.replaceChildren(document.createTextNode(t.pumps));
  document.getElementById('title-sensors')?.replaceChildren(document.createTextNode(t.sensors));
  document.getElementById('badge-live')?.replaceChildren(document.createTextNode(t.live));
  document.getElementById('title-config')?.replaceChildren(document.createTextNode(t.config));
  document.getElementById('badge-persist')?.replaceChildren(document.createTextNode(t.persist));
  document.getElementById('btn-save-names')?.replaceChildren(document.createTextNode(t.saveNames));
  const sel = document.getElementById('lang-select'); if (sel) sel.value = lang;
}
function buildSensorGrid(){ const grid=elements.sensorGrid, names=elements.sensorNamesContainer; grid.innerHTML=''; names.innerHTML=''; for(let i=0;i<10;i++){ const name=sensorNames[i]||`Sensor ${i+1}`; grid.innerHTML += `<div class=\"sensor-card\" id=\"sensor-${i}\"><div class=\"sensor-name\">${name}</div><div class=\"sensor-value\" id=\"sensor-${i}-value\">--%</div><div class=\"sensor-bar\"><div class=\"sensor-fill\" id=\"sensor-${i}-bar\"></div></div></div>`; names.innerHTML += `<input type=\"text\" id=\"name-${i}\" value=\"${name}\" placeholder=\"Sensor ${i+1}\">`; } }
function updateSensorValues(soil){ for(let i=0;i<10;i++){ const v=Math.max(0,Math.min(100,soil[i]||0)); const val=document.getElementById(`sensor-${i}-value`), bar=document.getElementById(`sensor-${i}-bar`); if(val) val.textContent=`${v}%`; if(bar){ bar.style.width=`${v}%`; if(v>=60){ bar.style.background='var(--gradient-green)'; } else if(v>=30){ bar.style.background='linear-gradient(135deg, #f59e0b 0%, #d97706 100%)'; } else { bar.style.background='var(--gradient-red)'; } } } }
function buildFallbackMapDataURI(city, lat, lon){
  const w=800, h=300;
  const title = `${city || 'Unknown'}${(lat!=null && lon!=null)? `  (${lat}, ${lon})`:''}`;
  const svg = `<?xml version="1.0" encoding="UTF-8"?>
  <svg xmlns="http://www.w3.org/2000/svg" width="${w}" height="${h}" viewBox="0 0 ${w} ${h}">
    <defs>
      <linearGradient id="g" x1="0" y1="0" x2="1" y2="1">
        <stop offset="0%" stop-color="#0b1430"/>
        <stop offset="100%" stop-color="#111b3a"/>
      </linearGradient>
    </defs>
    <rect x="0" y="0" width="${w}" height="${h}" fill="url(#g)"/>
    <g stroke="#2a3356" opacity="0.6">
      ${Array.from({length:16}).map((_,i)=>`<line x1="${i*(w/15)}" y1="0" x2="${i*(w/15)}" y2="${h}"/>`).join('')}
      ${Array.from({length:7}).map((_,i)=>`<line x1="0" y1="${i*(h/6)}" x2="${w}" y2="${i*(h/6)}"/>`).join('')}
    </g>
    <circle cx="${w/2}" cy="${h/2}" r="8" fill="#3b82f6" stroke="#ffffff" stroke-width="2"/>
    <text x="20" y="30" fill="#cfe0ff" font-family="Inter,Segoe UI,Arial" font-size="18">${title}</text>
    <text x="20" y="56" fill="#8ea0c3" font-family="Inter,Segoe UI,Arial" font-size="12">Map preview unavailable (offline)</text>
  </svg>`;
  return 'data:image/svg+xml;utf8,' + encodeURIComponent(svg);
}
function formatTime(seconds){ const m=Math.floor(seconds/60), s=seconds%60; return `${m}:${String(s).padStart(2,'0')}`; }
function getSensorGroupText(i){ return i===0?'Sensors 1-2': i===1?'Sensors 3-4': i===2?'Sensors 5-7':'Sensors 8-10'; }
function renderPumpControls(d){ 
  const c=elements.pumpControls; 
  const t=i18n[lang]||i18n.en; 
  
  // Check if we need to rebuild the entire HTML or just update values
  const needsRebuild = !c.children.length || c.children.length !== 4;
  
  if (needsRebuild) {
    console.log('[PumpControls] Rebuilding entire HTML');
    c.innerHTML=''; 
    for(let i=0;i<4;i++){ 
      const enabled=!!d.pumpEnabled[i]; 
      const power=Number(d.pumpPower[i]||0); 
      const manual=!!d.pumpManual[i]; 
      const override=Number(d.pumpOverride[i]||0); 
      const remMs=Array.isArray(d.lockoutMs)? Number(d.lockoutMs[i]||0):0; 
      const locked = remMs>0; 
      const lockoutTime = Math.ceil(remMs/1000); 
      const cutoff = Array.isArray(d.cutoffPct)? Number(d.cutoffPct[i]||85):85; 
      const card=document.createElement('div'); 
      card.className=`pump-card ${locked?'locked':''}`; 
      card.innerHTML = `<div class=\"pump-header\"><div><div class=\"pump-title\">${t.pump} ${i+1} <span class=\"pump-sensors\">(${getSensorGroupText(i)})</span></div></div><div class=\"pump-status\"><span class=\"status-chip ${enabled?'chip-enabled':'chip-disabled'}\">${enabled?t.enabled:t.disabled}</span><span class=\"status-chip ${manual?'chip-manual':'chip-auto'}\">${manual?t.manual:t.auto}</span>${locked?`<span class=\"status-chip chip-locked\">LOCKED ${formatTime(lockoutTime)}</span>`:''}</div></div><div class=\"pump-controls\"><div class=\"slider-container\"><input type=\"range\" class=\"slider\" id=\"slider-${i}\" min=\"0\" max=\"100\" value=\"${override}\" ${!manual?'disabled':''} oninput=\"updatePumpOverride(${i}, this.value)\"><div class=\"slider-value\"><span id=\"pump-${i}-power\">${power}%</span></div></div><div class=\"pump-buttons\"><button class=\"btn ${enabled?'btn-danger':'btn-success'}\" onclick=\"togglePumpEnabled(${i}, ${!enabled})\">${enabled?t.btnDisable:t.btnEnable}</button><button class=\"btn ${manual?'btn-ghost active':'btn-ghost'}\" onclick=\"togglePumpManual(${i}, ${!manual})\">${manual?t.btnManual:t.btnAuto}</button></div></div><div class=\"pump-settings\"><span class=\"setting-label\">Moisture Cutoff</span><div class=\"setting-input\"><input type=\"number\" id=\"cutoff-${i}\" step=\"1\" min=\"0\" max=\"100\" value=\"${cutoff.toFixed(0)}\" onfocus=\"console.log('[CutoffInput] Focus on pump ${i} cutoff')\" onblur=\"console.log('[CutoffInput] Blur on pump ${i} cutoff, value='+this.value)\"><span>%</span><button class=\"btn btn-primary\" onclick=\"savePumpCutoff(${i})\">${t.btnSet||'Set'}</button></div></div>`; 
      c.appendChild(card); 
    }
  } else {
    // Just update the dynamic values without rebuilding HTML
    console.log('[PumpControls] Updating values only (preserving user input)');
    for(let i=0;i<4;i++){ 
      const enabled=!!d.pumpEnabled[i]; 
      const power=Number(d.pumpPower[i]||0); 
      const manual=!!d.pumpManual[i]; 
      const override=Number(d.pumpOverride[i]||0); 
      const remMs=Array.isArray(d.lockoutMs)? Number(d.lockoutMs[i]||0):0; 
      const locked = remMs>0; 
      const lockoutTime = Math.ceil(remMs/1000); 
      
      // Update status chips
      const statusChips = c.children[i].querySelectorAll('.status-chip');
      if (statusChips.length >= 2) {
        statusChips[0].className = `status-chip ${enabled?'chip-enabled':'chip-disabled'}`;
        statusChips[0].textContent = enabled?t.enabled:t.disabled;
        statusChips[1].className = `status-chip ${manual?'chip-manual':'chip-auto'}`;
        statusChips[1].textContent = manual?t.manual:t.auto;
        
        // Handle lockout status chip
        let lockoutChip = statusChips[2];
        if (locked) {
          if (!lockoutChip) {
            // Create lockout chip if it doesn't exist
            lockoutChip = document.createElement('span');
            lockoutChip.className = 'status-chip chip-locked';
            c.children[i].querySelector('.pump-status').appendChild(lockoutChip);
          }
          lockoutChip.className = 'status-chip chip-locked';
          lockoutChip.textContent = `LOCKED ${formatTime(lockoutTime)}`;
          lockoutChip.style.display = 'inline-block';
        } else if (lockoutChip) {
          // Hide lockout chip if it exists but no lockout
          lockoutChip.style.display = 'none';
        }
      }
      
      // Update power display
      const powerSpan = c.children[i].querySelector(`#pump-${i}-power`);
      if (powerSpan) powerSpan.textContent = `${power}%`;
      
      // Update slider (only if not being dragged)
      const slider = c.children[i].querySelector(`#slider-${i}`);
      if (slider && !slider.matches(':focus')) {
        slider.value = override;
        slider.disabled = !manual;
      }
      
      // Update cutoff input (only if not being edited)
      const cutoffInput = c.children[i].querySelector(`#cutoff-${i}`);
      if (cutoffInput && document.activeElement !== cutoffInput) {
        const cutoff = Array.isArray(d.cutoffPct)? Number(d.cutoffPct[i]||85):85;
        cutoffInput.value = cutoff.toFixed(0);
        console.log(`[PumpControls] Updated cutoff ${i} to ${cutoff}% (input not focused)`);
      } else if (cutoffInput && document.activeElement === cutoffInput) {
        console.log(`[PumpControls] Preserving cutoff ${i} input value (user is typing)`);
      }
      
      // Update card locked state
      c.children[i].className = `pump-card ${locked?'locked':''}`;
    }
  }
}
function togglePumpEnabled(p,st){ fetch(`/setPump?pump=${p}&state=${st?1:0}`).then(()=>{ showToast(`Pump ${p+1} ${st?'enabled':'disabled'}`,'success'); fetchData(); }).catch(()=>showToast('Failed to update pump state','error')); }
function togglePumpManual(p,m){ const v = 0; fetch(`/setPumpOverride?pump=${p}&manual=${m?1:0}&value=${v}`).then(()=>{ showToast(`Pump ${p+1} set to ${m?'manual':'automatic'} mode`,'success'); fetchData(); }).catch(()=>showToast('Failed to update pump mode','error')); }
function updatePumpOverride(p,v){ clearTimeout(window[`pump${p}Timeout`]); window[`pump${p}Timeout`] = setTimeout(()=>{ fetch(`/setPumpOverride?pump=${p}&manual=1&value=${v}`).then(()=>showToast(`Pump ${p+1} set to ${v}%`,'info')).catch(()=>showToast('Failed to update pump override','error')); },200); }
function savePumpCutoff(p){ 
  const input=document.getElementById(`cutoff-${p}`); 
  const value=parseFloat(input.value); 
  console.log(`[SaveCutoff] Attempting to save pump ${p} cutoff: input value="${input.value}", parsed=${value}`);
  if(isNaN(value)||value<0||value>100){ 
    console.log(`[SaveCutoff] Invalid value: ${value}`);
    showToast('Please enter a valid percentage (0-100)','error'); 
    return; 
  } 
  console.log(`[SaveCutoff] Sending request to /setCutoff?pump=${p}&value=${value}`);
  fetch(`/setCutoff?pump=${p}&value=${value}`).then(()=>{ 
    console.log(`[SaveCutoff] Success! Pump ${p+1} cutoff set to ${value}%`);
    showToast(`Pump ${p+1} cutoff set to ${value}%`,'success'); 
    fetchData(); 
  }).catch((error)=>{ 
    console.error(`[SaveCutoff] Error:`, error);
    showToast('Failed to update cutoff','error'); 
  }); 
}
const OWM_KEY = "54b024e422c5cf82aef04a4ad16e9b92", OWM_LAT=45.7142, OWM_LON=16.0752;
function updateWeather(d){
  if(d.weather){
    const city = d.weather.city || 'Unknown';
    elements.weatherCity.textContent = city;
    elements.weatherTemp.textContent = d.weather.t!=null? `${Number(d.weather.t).toFixed(1)}°C`:'--°C';
    elements.weatherDesc.textContent = d.weather.desc || '—';
    elements.weatherHumidity.textContent = d.weather.h!=null? `${Math.round(d.weather.h)}%`:'--%';
    lastTempC = d.weather.t;

    // Update static map image (no API key required)
    // map removed per request
  }
}
async function fetchForecast(){ try{ const lat=(typeof gForecastLat==='number')?gForecastLat:OWM_LAT; const lon=(typeof gForecastLon==='number')?gForecastLon:OWM_LON; const url=`https://api.openweathermap.org/data/2.5/forecast?lat=${lat}&lon=${lon}&appid=${OWM_KEY}&units=metric&cnt=12`; const r=await fetch(url); const j=await r.json(); const list=j.list||[]; rainNext24h=false; for(let i=0;i<Math.min(8,list.length);i++){ const it=list[i]; const main=it.weather?.[0]?.main?.toLowerCase?.()||''; const desc=it.weather?.[0]?.description?.toLowerCase?.()||''; if(it.rain || main.includes('rain') || desc.includes('rain')){ rainNext24h=true; break; } } elements.forecastList.innerHTML = list.map(it=>{ const d=new Date((it.dt||0)*1000); const time=`${String(d.getDate()).padStart(2,'0')}.${String(d.getMonth()+1).padStart(2,'0')} ${String(d.getHours()).padStart(2,'0')}:00`; const temp=it.main?.temp; const icon=it.weather?.[0]?.icon||'01d'; const desc=it.weather?.[0]?.description||''; return `<div class=\"forecast-item\"><div class=\"forecast-time\">${time}</div><img class=\"forecast-icon\" src=\"https://openweathermap.org/img/wn/${icon}@2x.png\" alt=\"${desc}\"><div class=\"forecast-desc\">${desc}</div><div class=\"forecast-temp\">${temp!=null? Math.round(temp)+'°C':'--°C'}</div></div>`; }).join(''); updateIrrigationStatus(); }catch(e){ console.error('forecast',e); } }
function updateIrrigationStatus(){ const t=i18n[lang]||i18n.en; const tempOK = (typeof lastTempC==='number')? (lastTempC < maxTempSetting) : true; const rainOK = !rainNext24h; const allowed = tempOK && rainOK; elements.irrigationStatus.textContent = allowed ? (lang==='hr'?'Navodnjavanje dopušteno ✅':'Irrigation Allowed ✅') : (lang==='hr'?'Navodnjavanje blokirano ⛔':'Irrigation Blocked ⛔'); elements.irrigationBadge.textContent = allowed ? (lang==='hr'?'AKTIVNO':'ACTIVE') : t.inhibited; const reasons=[]; if(!tempOK) reasons.push(lang==='hr'?`Temperatura ${Number(lastTempC).toFixed(1)}°C prelazi limit ${maxTempSetting}°C`:`Temperature ${Number(lastTempC).toFixed(1)}°C exceeds ${maxTempSetting}°C limit`); if(!rainOK) reasons.push(lang==='hr'?'Očekuje se kiša u iduća 24h':'Rain expected in next 24 hours'); elements.irrigationReason.textContent = reasons.length ? reasons.join(' • ') : (lang==='hr'?`Uvjeti su OK: Temp < ${maxTempSetting}°C i bez kiše`:`All conditions met: Temp < ${maxTempSetting}°C and no rain forecast`); elements.irrigationCard.style.background = allowed? 'linear-gradient(135deg, rgba(16, 185, 129, 0.1) 0%, rgba(5, 150, 105, 0.05) 100%)' : 'linear-gradient(135deg, rgba(239, 68, 68, 0.1) 0%, rgba(220, 38, 38, 0.05) 100%)'; elements.irrigationCard.style.borderColor = allowed? 'rgba(16, 185, 129, 0.3)' : 'rgba(239, 68, 68, 0.3)'; }
function saveMaxTemp(){ const v=parseFloat(elements.maxTempInput.value); if(isNaN(v)||v<-20||v>60){ showToast('Please enter a valid temperature (-20 to 60°C)','error'); return; } maxTempSetting=v; fetch(`/setMaxTemp?value=${v}`).then(()=>{ showToast(`Max temperature set to ${v}°C`,'success'); updateIrrigationStatus(); }).catch(()=>showToast('Failed to save temperature setting','error')); }
async function loadNames(){ try{ const r=await fetch('/names'); const t=await r.text(); sensorNames = t.split('|').map(s=>s.trim()); buildSensorGrid(); } catch(e){ sensorNames = Array.from({length:10},(_,i)=>`Sensor ${i+1}`); buildSensorGrid(); } }
async function saveNames(){ const p=new URLSearchParams(); for(let i=0;i<10;i++){ const input=document.getElementById(`name-${i}`); p.append(`name${i}`, input.value || `Sensor ${i+1}`); } try{ await fetch('/names',{method:'POST', headers:{'Content-Type':'application/x-www-form-urlencoded'}, body:p.toString()}); showToast('Sensor names saved successfully','success'); await loadNames(); } catch(e){ showToast('Failed to save sensor names','error'); } }
async function fetchData(){ try{ const r=await fetch('/data',{cache:'no-store'}); const d=await r.json(); if(d.settings && d.settings.maxTemp!=null){ maxTempSetting=d.settings.maxTemp; if(document.activeElement!==elements.maxTempInput){ elements.maxTempInput.value = Number(maxTempSetting).toFixed(1); } }
  updateWeather(d);
  if (typeof d.weather?.lat==='number' && typeof d.weather?.lon==='number') { gForecastLat = Number(d.weather.lat); gForecastLon = Number(d.weather.lon); }
  if (elements.weatherCity && document.getElementById('forecast-city')) {
    document.getElementById('forecast-city').textContent = elements.weatherCity.textContent || '—';
  }
  elements.airTemp.textContent = d.t!=null? `${Number(d.t).toFixed(1)}°C`:'--°C';
  elements.airHumidity.textContent = d.h!=null? `${Math.round(Number(d.h))}%`:'--%';
  elements.powerVoltage.textContent = d.v!=null? `${Number(d.v).toFixed(2)} V`:'-- V';
  elements.powerCurrent.textContent = d.i!=null? `${Number(d.i).toFixed(1)} mA`:'-- mA';
  elements.powerPower.textContent = d.p!=null? `${Number(d.p).toFixed(1)} mW`:'-- mW';
  // populate durations if provided (our backend exposes durations.lockoutMs/manualMs)
  if (d.durations){
    const loMin = Math.round(Number(d.durations.lockoutMs||600000)/60000);
    const maMin = Math.round(Number(d.durations.manualMs||1200000)/60000);
    const loEl = document.getElementById('lockout-min');
    const maEl = document.getElementById('manual-min');
    if (loEl && document.activeElement !== loEl) loEl.value = loMin;
    if (maEl && document.activeElement !== maEl) maEl.value = maMin;
  }
  if(d.soil) updateSensorValues(d.soil);
  if(d.pumpEnabled && d.pumpPower) renderPumpControls(d);
  updateIrrigationStatus();
} catch(e){ console.error('fetch /data',e); elements.connectionStatus.textContent='Disconnected'; elements.connectionStatus.style.color='var(--accent-red)'; } }
async function init(){ elements.ip.textContent = location.host; applyI18n(); const sel=document.getElementById('lang-select'); if(sel){ sel.onchange=()=>{ lang=sel.value; localStorage.setItem('lang', lang); applyI18n(); }; }
  await loadNames(); await fetchData(); await fetchForecast(); setInterval(fetchData, 300); setInterval(fetchForecast, 10*60*1000); }
window.addEventListener('load', init);

// Save durations (uses existing protected /setDurations endpoint added earlier)
function saveDurations(){
  const lo = Math.max(1, Number(document.getElementById('lockout-min').value||10));
  const ma = Math.max(1, Number(document.getElementById('manual-min').value||20));
  fetch(`/setDurations?lockout=${lo*60*1000}&manual=${ma*60*1000}`)
    .then(()=>{ showToast('Durations saved','success'); fetchData(); })
    .catch(()=> showToast('Failed to save durations','error'));
}

// City/location setter: accepts "City name" or "lat,lon"
function applyCity(){
  const inp = document.getElementById('city-input');
  if (!inp) return;
  const v = inp.value.trim();
  if (!v){ showToast('Enter city or lat,lon', 'error'); return; }
  // if lat,lon pattern
  const m = v.match(/^\s*(-?\d+(?:\.\d+)?)\s*,\s*(-?\d+(?:\.\d+)?)\s*$/);
  if (m){
    const lat = parseFloat(m[1]), lon = parseFloat(m[2]);
    fetch(`/setLocation?lat=${lat}&lon=${lon}`)
      .then(()=>{ showToast('Location set', 'success'); fetchForecast(); fetchData(); })
      .catch(()=> showToast('Failed to set location', 'error'));
    return;
  }
  // city name only, try a lightweight geocode via Nominatim
  const q = encodeURIComponent(v);
  fetch(`https://nominatim.openstreetmap.org/search?format=json&limit=1&q=${q}`, {headers:{'Accept-Language':'en'}})
    .then(r=>r.json())
    .then(arr=>{
      if (!Array.isArray(arr) || arr.length===0) { showToast('City not found', 'error'); return; }
      const lat = parseFloat(arr[0].lat), lon = parseFloat(arr[0].lon);
      fetch(`/setLocation?lat=${lat}&lon=${lon}&city=${encodeURIComponent(v)}`)
        .then(()=>{ showToast('Location set', 'success'); gForecastLat=lat; gForecastLon=lon; if (document.getElementById('forecast-city')) document.getElementById('forecast-city').textContent = v; fetchForecast(); fetchData(); })
        .catch(()=> showToast('Failed to save location', 'error'));
    })
    .catch(()=> showToast('Geocoding failed', 'error'));
}
</script>

</body>
</html>
)HTML";

