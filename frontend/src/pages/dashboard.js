/**
 * GridShield — Dashboard Page
 * Live monitoring with hero cards, voltage chart, alerts, and reset
 */

import { getStatus, getReadings, getAlerts, getLatestReading, resetAllData, getNotificationSummary } from '../api.js';
import { createLineChart } from '../components/chart.js';

/** Bilingual labels */
const L = {
  voltage:     { id: 'Tegangan',     en: 'Voltage' },
  current:     { id: 'Arus',         en: 'Current' },
  power:       { id: 'Daya',         en: 'Power' },
  energy:      { id: 'Energi',       en: 'Energy' },
  temperature: { id: 'Suhu Panel',   en: 'Temperature' },
  humidity:    { id: 'Kelembapan',   en: 'Humidity' },
  relay:       { id: 'Status Relay', en: 'Relay Status' },
  pf:          { id: 'Faktor Daya',  en: 'Power Factor' },
  liveMonitor: { id: 'Monitor Langsung', en: 'Live Monitor' },
  voltTrend:   { id: 'Tren Tegangan',    en: 'Voltage Trend' },
  alerts:      { id: 'Peringatan Terbaru', en: 'Recent Alerts' },
  stats:       { id: 'Statistik',    en: 'Statistics' },
  readings:    { id: 'Total Pembacaan', en: 'Total Readings' },
  meters:      { id: 'Meter Aktif',  en: 'Active Meters' },
  anomalies:   { id: 'Anomali',      en: 'Anomalies' },
  reset:       { id: 'Reset Data',   en: 'Reset Data' },
  resetConfirm:{ id: 'Hapus semua data? Aksi ini tidak bisa dibatalkan.', en: 'Delete all data? This cannot be undone.' },
  noAlerts:    { id: 'Tidak ada peringatan', en: 'No recent alerts' },
  updated:     { id: 'Diperbarui',   en: 'Updated' },
  secAgo:      { id: 'detik lalu',   en: 'sec ago' },
  relayOn:     { id: 'Hidup',        en: 'ON' },
  relayOff:    { id: 'Mati',         en: 'OFF' },
  na:          { id: 'N/A',          en: 'N/A' },
};

function t(key) {
  const item = L[key];
  if (!item) return key;
  return `<span class="l-id">${item.id}</span><span class="l-en">${item.en}</span>`;
}
function tRaw(key) {
  const item = L[key];
  return item ? item.id : key;
}

/** Threshold color for voltage */
function voltColor(mv) {
  const v = mv / 1000;
  if (v < 190 || v > 250) return 'var(--color-red)';
  if (v < 200 || v > 245) return 'var(--color-amber)';
  return 'var(--color-green)';
}

/** Threshold color for temperature */
function tempColor(c) {
  if (c == null) return 'var(--color-text-secondary)';
  if (c > 55) return 'var(--color-red)';
  if (c > 45) return 'var(--color-amber)';
  return 'var(--color-green)';
}

function severityColor(sev) {
  if (sev >= 3) return 'var(--color-red)';
  if (sev >= 2) return 'var(--color-orange)';
  if (sev >= 1) return 'var(--color-amber)';
  return 'var(--color-green)';
}

function timeAgo(ts) {
  const diff = Date.now() - new Date(ts).getTime();
  const secs = Math.floor(diff / 1000);
  if (secs < 5) return 'just now';
  if (secs < 60) return `${secs}s ago`;
  const mins = Math.floor(secs / 60);
  if (mins < 60) return `${mins}m ago`;
  const hrs = Math.floor(mins / 60);
  if (hrs < 24) return `${hrs}h ago`;
  return `${Math.floor(hrs / 24)}d ago`;
}

export default async function renderDashboard(container) {
  const [status, readings, alerts, notifSummary] = await Promise.all([
    getStatus(),
    getReadings({ limit: 50 }),
    getAlerts({ limit: 5 }),
    getNotificationSummary(),
  ]);

  const latest = readings.length > 0 ? readings[0] : null;

  container.innerHTML = `
    <div class="page-enter">
      <div class="page-header">
        <h1>${t('liveMonitor')}</h1>
        <div class="header-actions">
          <span class="live-indicator" id="live-indicator">
            <span class="live-dot"></span>
            <span id="last-updated">--</span>
          </span>
          <button class="btn btn-sm btn-danger" id="reset-btn" title="${tRaw('reset')}">${t('reset')}</button>
        </div>
      </div>

      <!-- Hero Cards -->
      <div class="hero-grid" id="hero-grid">
        <div class="hero-card hero-voltage" id="card-voltage">
          <div class="hero-value" id="val-voltage" style="color:${latest ? voltColor(latest.voltage_mv) : 'inherit'}">
            ${latest ? (latest.voltage_mv / 1000).toFixed(1) : '--'}
            <span class="hero-unit">V</span>
          </div>
          <div class="hero-label">${t('voltage')}</div>
        </div>

        <div class="hero-card hero-current" id="card-current">
          <div class="hero-value" id="val-current">
            ${latest ? (latest.current_ma / 1000).toFixed(2) : '--'}
            <span class="hero-unit">A</span>
          </div>
          <div class="hero-label">${t('current')}</div>
        </div>

        <div class="hero-card hero-power" id="card-power">
          <div class="hero-value" id="val-power">
            ${latest ? (latest.power_mw / 1000).toFixed(1) : '--'}
            <span class="hero-unit">W</span>
          </div>
          <div class="hero-label">${t('power')}</div>
        </div>

        <div class="hero-card hero-energy" id="card-energy">
          <div class="hero-value" id="val-energy">
            ${latest ? latest.energy_wh : '--'}
            <span class="hero-unit">Wh</span>
          </div>
          <div class="hero-label">${t('energy')}</div>
        </div>

        <div class="hero-card hero-temp" id="card-temp">
          <div class="hero-value" id="val-temp" style="color:${latest ? tempColor(latest.temperature_c) : 'inherit'}">
            ${latest && latest.temperature_c != null ? latest.temperature_c.toFixed(1) : '--'}
            <span class="hero-unit">&deg;C</span>
          </div>
          <div class="hero-label">${t('temperature')}</div>
        </div>

        <div class="hero-card hero-humidity" id="card-humidity">
          <div class="hero-value" id="val-humidity">
            ${latest && latest.humidity_pct != null ? latest.humidity_pct.toFixed(1) : '--'}
            <span class="hero-unit">%</span>
          </div>
          <div class="hero-label">${t('humidity')}</div>
        </div>

        <div class="hero-card hero-relay" id="card-relay">
          <div class="hero-value" id="val-relay">
            ${latest && latest.relay_on != null
              ? `<span class="relay-badge ${latest.relay_on ? 'relay-on' : 'relay-off'}">${latest.relay_on ? 'ON' : 'OFF'}</span>`
              : '--'}
          </div>
          <div class="hero-label">${t('relay')}</div>
        </div>

        <div class="hero-card hero-pf" id="card-pf">
          <div class="hero-value" id="val-pf">
            ${latest ? (latest.power_factor / 100).toFixed(2) : '--'}
          </div>
          <div class="hero-label">${t('pf')}</div>
        </div>
      </div>

      <!-- Charts + Alerts Row -->
      <div class="charts-row">
        <div class="glass-panel chart-panel">
          <div class="panel-header">
            <span class="panel-title">${t('voltTrend')}</span>
            <span class="badge badge-info">${readings.length} data</span>
          </div>
          <div class="panel-body">
            <div class="chart-container" id="voltage-chart"></div>
          </div>
        </div>

        <div class="glass-panel alerts-panel">
          <div class="panel-header">
            <span class="panel-title">${t('alerts')}</span>
            <span class="badge badge-critical">${status.unacknowledged_alerts}</span>
          </div>
          <div class="panel-body" id="recent-alerts"></div>

          <div class="panel-footer stats-footer">
            <div class="stat-mini"><span class="stat-mini-val" id="stat-readings">${status.total_readings.toLocaleString()}</span><span class="stat-mini-label">${t('readings')}</span></div>
            <div class="stat-mini"><span class="stat-mini-val" id="stat-meters">${status.active_meters}</span><span class="stat-mini-label">${t('meters')}</span></div>
            <div class="stat-mini"><span class="stat-mini-val" id="stat-anomalies">${status.total_anomalies}</span><span class="stat-mini-label">${t('anomalies')}</span></div>
          </div>
        </div>
      </div>
    </div>
  `;

  // --- Voltage Chart ---
  const sorted = [...readings].reverse();
  const labels = sorted.map(r => new Date(r.timestamp).toLocaleTimeString([], { hour: '2-digit', minute: '2-digit', second: '2-digit' }));

  const chartContainer = document.getElementById('voltage-chart');
  const chart = createLineChart(chartContainer, {
    labels,
    datasets: [{
      label: 'Voltage (V)',
      data: sorted.map(r => r.voltage_mv / 1000),
      borderColor: '#00f5d4',
      backgroundColor: 'rgba(0, 245, 212, 0.06)',
      fill: true,
    }],
    options: {
      scales: {
        y: {
          beginAtZero: false,
          suggestedMin: 200,
          suggestedMax: 250,
          title: { display: true, text: 'Volt (V)' },
        },
      },
    },
  });

  // --- Recent Alerts ---
  const alertsList = document.getElementById('recent-alerts');
  if (alerts.length === 0) {
    alertsList.innerHTML = `<div class="empty-state"><div class="empty-icon">OK</div><div class="empty-text">${tRaw('noAlerts')}</div></div>`;
  } else {
    alertsList.innerHTML = alerts.map(a => `
      <div class="recent-alert-item">
        <span class="recent-alert-dot" style="background:${severityColor(a.severity)}"></span>
        <div class="recent-alert-info">
          <div class="recent-alert-type">${a.tamper_type.replace(/_/g, ' ')}</div>
          <div class="recent-alert-time">${timeAgo(a.timestamp)}</div>
        </div>
        ${a.acknowledged ? '<span class="badge badge-success">ACK</span>' : '<span class="badge badge-critical">NEW</span>'}
      </div>
    `).join('');
  }

  // --- Reset Button ---
  document.getElementById('reset-btn')?.addEventListener('click', async () => {
    if (!confirm(tRaw('resetConfirm'))) return;
    try {
      const result = await resetAllData();
      alert(`Data reset! Deleted: ${JSON.stringify(result.deleted)}`);
      location.reload();
    } catch (e) {
      alert('Reset failed: ' + e.message);
    }
  });

  // --- Live Update (2s polling) ---
  let lastUpdateTime = Date.now();
  const updateIndicator = () => {
    const el = document.getElementById('last-updated');
    if (el) {
      const sec = Math.floor((Date.now() - lastUpdateTime) / 1000);
      el.textContent = sec < 2 ? 'live' : `${sec}s`;
    }
  };
  const indicatorTimer = setInterval(updateIndicator, 1000);

  const pollTimer = setInterval(async () => {
    try {
      const [r, s] = await Promise.all([getLatestReading(), getStatus()]);

      if (r) {
        lastUpdateTime = Date.now();
        const el = id => document.getElementById(id);

        // Update hero values
        if (el('val-voltage')) {
          const v = (r.voltage_mv / 1000).toFixed(1);
          el('val-voltage').innerHTML = `${v}<span class="hero-unit">V</span>`;
          el('val-voltage').style.color = voltColor(r.voltage_mv);
        }
        if (el('val-current')) el('val-current').innerHTML = `${(r.current_ma / 1000).toFixed(2)}<span class="hero-unit">A</span>`;
        if (el('val-power'))   el('val-power').innerHTML = `${((r.power_mw || 0) / 1000).toFixed(1)}<span class="hero-unit">W</span>`;
        if (el('val-energy'))  el('val-energy').innerHTML = `${r.energy_wh}<span class="hero-unit">Wh</span>`;
        if (el('val-temp') && r.temperature_c != null) {
          el('val-temp').innerHTML = `${r.temperature_c.toFixed(1)}<span class="hero-unit">&deg;C</span>`;
          el('val-temp').style.color = tempColor(r.temperature_c);
        }
        if (el('val-humidity') && r.humidity_pct != null) {
          el('val-humidity').innerHTML = `${r.humidity_pct.toFixed(1)}<span class="hero-unit">%</span>`;
        }
        if (el('val-relay') && r.relay_on != null) {
          el('val-relay').innerHTML = `<span class="relay-badge ${r.relay_on ? 'relay-on' : 'relay-off'}">${r.relay_on ? 'ON' : 'OFF'}</span>`;
        }
        if (el('val-pf')) el('val-pf').textContent = (r.power_factor / 100).toFixed(2);

        // Push to chart
        const time = new Date(r.timestamp).toLocaleTimeString([], { hour: '2-digit', minute: '2-digit', second: '2-digit' });
        const ds = chart.data;
        if (ds.labels[ds.labels.length - 1] !== time) {
          ds.labels.push(time);
          ds.datasets[0].data.push(r.voltage_mv / 1000);
          if (ds.labels.length > 50) {
            ds.labels.shift();
            ds.datasets[0].data.shift();
          }
          chart.update('none');
        }
      }

      // Update stats
      const el = id => document.getElementById(id);
      if (el('stat-readings')) el('stat-readings').textContent = s.total_readings.toLocaleString();
      if (el('stat-meters'))   el('stat-meters').textContent = s.active_meters;
      if (el('stat-anomalies'))el('stat-anomalies').textContent = s.total_anomalies;
    } catch (_) { /* ignore if navigated away */ }
  }, 2000);

  return () => {
    clearInterval(pollTimer);
    clearInterval(indicatorTimer);
    chart.destroy();
  };
}
