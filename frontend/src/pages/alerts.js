/**
 * GridShield — Alerts Page
 * Tamper alert management with filtering and acknowledge action
 */

import { getAlerts, acknowledgeAlert, exportAlerts } from '../api.js';

const TAMPER_ICONS = {
  CasingOpened: '📦',
  MagneticInterference: '🧲',
  PowerCutAttempt: '🔌',
  PhysicalShock: '💥',
  VibrationDetected: '📳',
  TemperatureAnomaly: '🌡️',
};

function severityBadge(sev) {
  const map = {
    4: ['CRITICAL', 'badge-critical'],
    3: ['HIGH', 'badge-high'],
    2: ['MEDIUM', 'badge-medium'],
    1: ['LOW', 'badge-low'],
    0: ['INFO', 'badge-info'],
  };
  const [label, cls] = map[sev] || map[0];
  return `<span class="badge ${cls}">${label}</span>`;
}

function formatTime(ts) {
  return new Date(ts).toLocaleString([], {
    month: 'short', day: 'numeric',
    hour: '2-digit', minute: '2-digit',
  });
}

function formatMeterId(id) {
  return id.toString(16).toUpperCase().slice(-8);
}

export default async function renderAlerts(container) {
  let filter = 'all';

  async function refresh() {
    const params = {};
    if (filter === 'unacknowledged') params.acknowledged = false;
    if (filter === 'acknowledged') params.acknowledged = true;
    params.limit = 100;

    const alerts = await getAlerts(params);

    container.innerHTML = `
      <div class="page-enter">
        <div class="page-header">
          <h1>Tamper Alerts</h1>
          <p class="page-desc">Physical tamper events from field devices</p>
        </div>

        <div class="filter-bar">
          <select id="alert-filter">
            <option value="all" ${filter === 'all' ? 'selected' : ''}>All Alerts</option>
            <option value="unacknowledged" ${filter === 'unacknowledged' ? 'selected' : ''}>Unacknowledged</option>
            <option value="acknowledged" ${filter === 'acknowledged' ? 'selected' : ''}>Acknowledged</option>
          </select>
          <span class="badge badge-info">${alerts.length} results</span>
          <button class="btn btn-sm" id="export-alerts-btn">📥 Export CSV</button>
        </div>

        <div class="glass-panel">
          ${alerts.length === 0 ? `
            <div class="empty-state">
              <div class="empty-icon">✅</div>
              <div class="empty-text">No alerts match the current filter</div>
            </div>
          ` : `
            <table class="data-table">
              <thead>
                <tr>
                  <th>Time</th>
                  <th>Meter ID</th>
                  <th>Type</th>
                  <th>Severity</th>
                  <th>Status</th>
                  <th>Action</th>
                </tr>
              </thead>
              <tbody>
                ${alerts.map(a => `
                  <tr>
                    <td>${formatTime(a.timestamp)}</td>
                    <td style="font-family:var(--font-mono);color:var(--color-accent)">${formatMeterId(a.meter_id)}</td>
                    <td>
                      <span>${TAMPER_ICONS[a.tamper_type] || '🔒'}</span>
                      ${a.tamper_type.replace(/_/g, ' ')}
                    </td>
                    <td>${severityBadge(a.severity)}</td>
                    <td>
                      ${a.acknowledged
        ? '<span class="badge badge-success">Acknowledged</span>'
        : '<span class="badge badge-critical">⏳ Pending</span>'}
                    </td>
                    <td>
                      ${a.acknowledged
        ? '—'
        : `<button class="btn btn-primary btn-sm" data-ack-id="${a.id}">Acknowledge</button>`}
                    </td>
                  </tr>
                `).join('')}
              </tbody>
            </table>
          `}
        </div>
      </div>
    `;

    // Bind filter
    document.getElementById('alert-filter')?.addEventListener('change', (e) => {
      filter = e.target.value;
      refresh();
    });

    // Export button
    document.getElementById('export-alerts-btn')?.addEventListener('click', () => {
      exportAlerts();
    });

    // Bind acknowledge buttons
    container.querySelectorAll('[data-ack-id]').forEach(btn => {
      btn.addEventListener('click', async (e) => {
        const id = e.target.dataset.ackId;
        e.target.disabled = true;
        e.target.textContent = '...';
        try {
          await acknowledgeAlert(id);
          await refresh();
        } catch (err) {
          alert('Failed to acknowledge: ' + err.message);
          e.target.disabled = false;
          e.target.textContent = 'Acknowledge';
        }
      });
    });
  }

  await refresh();
}
