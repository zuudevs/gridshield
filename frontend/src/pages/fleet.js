/**
 * GridShield — Fleet Page
 * Meter grid + detail modal with historical chart
 */

import { getReadings, getAlerts, getAnomalies } from '../api.js';
import { createLineChart } from '../components/chart.js';

function formatMeterId(id) {
    return id.toString(16).toUpperCase().slice(-8);
}

function formatTime(ts) {
    return new Date(ts).toLocaleString([], {
        month: 'short', day: 'numeric',
        hour: '2-digit', minute: '2-digit',
    });
}

/**
 * Group readings by meter_id and build summary for each.
 */
function buildFleetData(readings, alerts) {
    const meters = {};

    readings.forEach(r => {
        const id = r.meter_id;
        if (!meters[id]) {
            meters[id] = {
                meter_id: id,
                readings: [],
                alert_count: 0,
                last_reading: null,
            };
        }
        meters[id].readings.push(r);
        if (!meters[id].last_reading || new Date(r.timestamp) > new Date(meters[id].last_reading.timestamp)) {
            meters[id].last_reading = r;
        }
    });

    alerts.forEach(a => {
        const id = a.meter_id;
        if (meters[id]) {
            meters[id].alert_count++;
        }
    });

    return Object.values(meters);
}

function renderMeterCard(meter) {
    const lr = meter.last_reading;
    return `
    <div class="meter-card" data-meter-id="${meter.meter_id}">
      <div class="meter-id">⚡ ${formatMeterId(meter.meter_id)}</div>
      <div class="meter-stats">
        <div>
          <div class="meter-stat-label">Energy</div>
          <div class="meter-stat-value">${lr ? lr.energy_wh.toLocaleString() + ' Wh' : '—'}</div>
        </div>
        <div>
          <div class="meter-stat-label">Voltage</div>
          <div class="meter-stat-value">${lr ? (lr.voltage_mv / 1000).toFixed(1) + ' V' : '—'}</div>
        </div>
        <div>
          <div class="meter-stat-label">Last Seen</div>
          <div class="meter-stat-value">${lr ? formatTime(lr.timestamp) : '—'}</div>
        </div>
        <div>
          <div class="meter-stat-label">Alerts</div>
          <div class="meter-stat-value" style="color:${meter.alert_count > 0 ? 'var(--color-red)' : 'var(--color-green)'}">${meter.alert_count}</div>
        </div>
      </div>
    </div>
  `;
}

function openMeterDetail(meter) {
    // Create overlay
    const overlay = document.createElement('div');
    overlay.className = 'meter-detail-overlay';
    overlay.innerHTML = `
    <div class="meter-detail-panel">
      <div style="display:flex;justify-content:space-between;align-items:center;margin-bottom:var(--space-6)">
        <div>
          <h2 style="font-size:var(--text-xl);margin-bottom:var(--space-1)">Meter ${formatMeterId(meter.meter_id)}</h2>
          <p style="color:var(--color-text-muted);font-size:var(--text-sm)">ID: ${meter.meter_id}</p>
        </div>
        <button class="btn" id="close-detail">✕ Close</button>
      </div>

      <div class="stats-grid" style="margin-bottom:var(--space-6)">
        <div class="stat-card accent-cyan">
          <div class="stat-label">Readings</div>
          <div class="stat-value" style="font-size:var(--text-xl)">${meter.readings.length}</div>
        </div>
        <div class="stat-card accent-red">
          <div class="stat-label">Alerts</div>
          <div class="stat-value" style="font-size:var(--text-xl)">${meter.alert_count}</div>
        </div>
        <div class="stat-card accent-green">
          <div class="stat-label">Last Energy</div>
          <div class="stat-value" style="font-size:var(--text-xl)">${meter.last_reading ? meter.last_reading.energy_wh : 0} Wh</div>
        </div>
      </div>

      <div class="glass-panel">
        <div class="panel-header">
          <span class="panel-title">Historical Energy (Wh)</span>
        </div>
        <div class="panel-body">
          <div class="chart-container" id="meter-detail-chart" style="height:260px"></div>
        </div>
      </div>
    </div>
  `;

    document.body.appendChild(overlay);

    // Chart
    const sorted = [...meter.readings].sort((a, b) => new Date(a.timestamp) - new Date(b.timestamp));
    const chartEl = overlay.querySelector('#meter-detail-chart');
    const chart = createLineChart(chartEl, {
        labels: sorted.map(r => formatTime(r.timestamp)),
        datasets: [{
            label: 'Energy (Wh)',
            data: sorted.map(r => r.energy_wh),
            borderColor: '#00f5d4',
            backgroundColor: 'rgba(0, 245, 212, 0.1)',
            fill: true,
        }],
    });

    // Close
    const close = () => {
        chart.destroy();
        overlay.remove();
    };
    overlay.querySelector('#close-detail').addEventListener('click', close);
    overlay.addEventListener('click', (e) => {
        if (e.target === overlay) close();
    });
}

export default async function renderFleet(container) {
    const [readings, alerts] = await Promise.all([
        getReadings({ limit: 200 }),
        getAlerts({ limit: 200 }),
    ]);

    const fleet = buildFleetData(readings, alerts);

    container.innerHTML = `
    <div class="page-enter">
      <div class="page-header">
        <h1>Fleet Management</h1>
        <p class="page-desc">${fleet.length} meters in the network</p>
      </div>

      ${fleet.length === 0 ? `
        <div class="glass-panel">
          <div class="empty-state">
            <div class="empty-icon">🔌</div>
            <div class="empty-text">No meters have reported yet</div>
          </div>
        </div>
      ` : `
        <div class="meter-grid">
          ${fleet.map(m => renderMeterCard(m)).join('')}
        </div>
      `}
    </div>
  `;

    // Bind card clicks
    container.querySelectorAll('.meter-card').forEach(card => {
        card.addEventListener('click', () => {
            const id = Number(card.dataset.meterId);
            const meter = fleet.find(m => m.meter_id === id);
            if (meter) openMeterDetail(meter);
        });
    });
}
