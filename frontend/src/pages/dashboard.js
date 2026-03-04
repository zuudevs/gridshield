/**
 * GridShield — Dashboard Page
 * KPI cards + energy chart + recent alerts
 */

import { getStatus, getReadings, getAlerts } from '../api.js';
import { createLineChart } from '../components/chart.js';

/** Helper: format timestamp */
function timeAgo(ts) {
    const diff = Date.now() - new Date(ts).getTime();
    const mins = Math.floor(diff / 60000);
    if (mins < 1) return 'just now';
    if (mins < 60) return `${mins}m ago`;
    const hrs = Math.floor(mins / 60);
    if (hrs < 24) return `${hrs}h ago`;
    return `${Math.floor(hrs / 24)}d ago`;
}

function severityColor(sev) {
    if (sev >= 3) return 'var(--color-red)';
    if (sev >= 2) return 'var(--color-orange)';
    if (sev >= 1) return 'var(--color-amber)';
    return 'var(--color-green)';
}

export default async function renderDashboard(container) {
    // Fetch data in parallel
    const [status, readings, alerts] = await Promise.all([
        getStatus(),
        getReadings({ limit: 100 }),
        getAlerts({ limit: 10 }),
    ]);

    container.innerHTML = `
    <div class="page-enter">
      <div class="page-header">
        <h1>Dashboard</h1>
        <p class="page-desc"><span class="live-dot"></span>Real-time monitoring overview</p>
      </div>

      <!-- KPI Stats -->
      <div class="stats-grid" id="stats-grid">
        <div class="stat-card accent-cyan">
          <div class="stat-header">
            <span class="stat-label">Total Readings</span>
            <span class="stat-icon">📊</span>
          </div>
          <div class="stat-value" id="stat-readings">${status.total_readings.toLocaleString()}</div>
          <div class="stat-sub">All meter data collected</div>
        </div>

        <div class="stat-card accent-purple">
          <div class="stat-header">
            <span class="stat-label">Active Meters</span>
            <span class="stat-icon">🔌</span>
          </div>
          <div class="stat-value" id="stat-meters">${status.active_meters}</div>
          <div class="stat-sub">Reporting devices</div>
        </div>

        <div class="stat-card accent-red">
          <div class="stat-header">
            <span class="stat-label">Unack. Alerts</span>
            <span class="stat-icon">🚨</span>
          </div>
          <div class="stat-value" id="stat-alerts">${status.unacknowledged_alerts}</div>
          <div class="stat-sub">Tamper alerts pending</div>
        </div>

        <div class="stat-card accent-amber">
          <div class="stat-header">
            <span class="stat-label">Anomalies</span>
            <span class="stat-icon">⚠️</span>
          </div>
          <div class="stat-value" id="stat-anomalies">${status.total_anomalies}</div>
          <div class="stat-sub">Detected events</div>
        </div>
      </div>

      <!-- Charts Row -->
      <div class="charts-row">
        <div class="glass-panel">
          <div class="panel-header">
            <span class="panel-title">Energy Consumption (Wh)</span>
            <span class="badge badge-info">Last ${readings.length} readings</span>
          </div>
          <div class="panel-body">
            <div class="chart-container" id="energy-chart"></div>
          </div>
        </div>

        <div class="glass-panel">
          <div class="panel-header">
            <span class="panel-title">Recent Alerts</span>
            <span class="badge badge-critical">${status.unacknowledged_alerts} active</span>
          </div>
          <div class="panel-body" id="recent-alerts"></div>
        </div>
      </div>
    </div>
  `;

    // --- Energy Chart ---
    const sorted = [...readings].reverse();
    const labels = sorted.map((r, i) => {
        const d = new Date(r.timestamp);
        return d.toLocaleTimeString([], { hour: '2-digit', minute: '2-digit' });
    });

    const chartContainer = document.getElementById('energy-chart');
    const chart = createLineChart(chartContainer, {
        labels,
        datasets: [
            {
                label: 'Energy (Wh)',
                data: sorted.map(r => r.energy_wh),
                borderColor: '#00f5d4',
                backgroundColor: 'rgba(0, 245, 212, 0.08)',
                fill: true,
            },
            {
                label: 'Voltage (V)',
                data: sorted.map(r => r.voltage_mv / 1000),
                borderColor: '#7b61ff',
                backgroundColor: 'rgba(123, 97, 255, 0.05)',
                fill: false,
                yAxisID: 'y1',
            },
        ],
        options: {
            scales: {
                y: { position: 'left', title: { display: true, text: 'Wh' } },
                y1: {
                    position: 'right',
                    title: { display: true, text: 'Voltage (V)' },
                    grid: { drawOnChartArea: false },
                },
            },
        },
    });

    // --- Recent Alerts ---
    const alertsList = document.getElementById('recent-alerts');
    if (alerts.length === 0) {
        alertsList.innerHTML = `
      <div class="empty-state">
        <div class="empty-icon">✅</div>
        <div class="empty-text">No recent alerts</div>
      </div>`;
    } else {
        alertsList.innerHTML = alerts.map(a => `
      <div class="recent-alert-item">
        <span class="recent-alert-dot" style="background:${severityColor(a.severity)}"></span>
        <div class="recent-alert-info">
          <div class="recent-alert-type">${a.tamper_type.replace(/_/g, ' ')}</div>
          <div class="recent-alert-time">${timeAgo(a.timestamp)} · Meter ${a.meter_id.toString(16).toUpperCase().slice(-8)}</div>
        </div>
        ${a.acknowledged ? '<span class="badge badge-success">ACK</span>' : '<span class="badge badge-critical">NEW</span>'}
      </div>
    `).join('');
    }

    // --- Auto-refresh ---
    const interval = setInterval(async () => {
        try {
            const s = await getStatus();
            const el = (id) => document.getElementById(id);
            if (el('stat-readings')) el('stat-readings').textContent = s.total_readings.toLocaleString();
            if (el('stat-meters')) el('stat-meters').textContent = s.active_meters;
            if (el('stat-alerts')) el('stat-alerts').textContent = s.unacknowledged_alerts;
            if (el('stat-anomalies')) el('stat-anomalies').textContent = s.total_anomalies;
        } catch (_) { /* ignore if navigated away */ }
    }, 10000);

    // Return cleanup function
    return () => {
        clearInterval(interval);
        chart.destroy();
    };
}
