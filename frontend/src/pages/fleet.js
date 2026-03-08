/**
 * GridShield — Fleet Page
 * Meter management with registration, status, and detail modal
 */

import { getMeters, createMeter, deleteMeter, getMeterStats, getReadings, exportReadings } from '../api.js';
import { createLineChart } from '../components/chart.js';

function formatMeterId(id) {
  return id.toString(16).toUpperCase().slice(-8);
}

function formatTime(ts) {
  if (!ts) return '—';
  return new Date(ts).toLocaleString([], {
    month: 'short', day: 'numeric',
    hour: '2-digit', minute: '2-digit',
  });
}

function statusIndicator(status) {
  const colors = {
    online: 'var(--color-green)',
    offline: 'var(--color-text-muted)',
    tampered: 'var(--color-red)',
  };
  const color = colors[status] || colors.offline;
  return `<span class="status-dot" style="background:${color}" title="${status}"></span>
            <span class="status-label">${status}</span>`;
}

function renderMeterCard(meter) {
  return `
    <div class="meter-card" data-meter-id="${meter.meter_id}">
      <div style="display:flex;justify-content:space-between;align-items:center;margin-bottom:var(--space-3)">
        <div class="meter-id">⚡ ${formatMeterId(meter.meter_id)}</div>
        <div class="meter-status-badge">${statusIndicator(meter.status)}</div>
      </div>
      <div class="meter-stats">
        <div>
          <div class="meter-stat-label">Name</div>
          <div class="meter-stat-value">${meter.name || '—'}</div>
        </div>
        <div>
          <div class="meter-stat-label">Location</div>
          <div class="meter-stat-value">${meter.location || '—'}</div>
        </div>
        <div>
          <div class="meter-stat-label">FW</div>
          <div class="meter-stat-value">${meter.firmware_version}</div>
        </div>
        <div>
          <div class="meter-stat-label">Last Seen</div>
          <div class="meter-stat-value">${formatTime(meter.last_seen_at)}</div>
        </div>
      </div>
      <div style="display:flex;gap:var(--space-2);margin-top:var(--space-3)">
        <button class="btn btn-sm btn-primary meter-detail-btn" data-id="${meter.meter_id}">Details</button>
        <button class="btn btn-sm btn-danger meter-delete-btn" data-id="${meter.meter_id}">Delete</button>
      </div>
    </div>
  `;
}

function openRegisterModal(onSubmit) {
  const overlay = document.createElement('div');
  overlay.className = 'modal-overlay';
  overlay.innerHTML = `
    <div class="modal-panel">
      <div class="modal-header">
        <h2>Register New Meter</h2>
        <button class="btn btn-sm" id="modal-close">✕</button>
      </div>
      <form id="register-form" class="modal-form">
        <label>
          <span>Meter ID (integer)</span>
          <input type="number" name="meter_id" required placeholder="e.g. 305419896" />
        </label>
        <label>
          <span>Name</span>
          <input type="text" name="name" placeholder="e.g. Building A - Floor 3" maxlength="100" />
        </label>
        <label>
          <span>Location</span>
          <input type="text" name="location" placeholder="e.g. Jakarta, Indonesia" maxlength="200" />
        </label>
        <label>
          <span>Firmware Version</span>
          <input type="text" name="firmware_version" value="3.2.0" maxlength="20" />
        </label>
        <div class="modal-actions">
          <button type="button" class="btn" id="modal-cancel">Cancel</button>
          <button type="submit" class="btn btn-primary">Register</button>
        </div>
      </form>
    </div>
  `;

  document.body.appendChild(overlay);

  const close = () => overlay.remove();
  overlay.querySelector('#modal-close').addEventListener('click', close);
  overlay.querySelector('#modal-cancel').addEventListener('click', close);
  overlay.addEventListener('click', (e) => {
    if (e.target === overlay) close();
  });

  overlay.querySelector('#register-form').addEventListener('submit', async (e) => {
    e.preventDefault();
    const fd = new FormData(e.target);
    const data = {
      meter_id: parseInt(fd.get('meter_id')),
      name: fd.get('name') || '',
      location: fd.get('location') || '',
      firmware_version: fd.get('firmware_version') || 'unknown',
    };
    try {
      await onSubmit(data);
      close();
    } catch (err) {
      alert('Registration failed: ' + err.message);
    }
  });
}

async function openMeterDetail(meterId) {
  let stats;
  try {
    stats = await getMeterStats(meterId);
  } catch {
    alert('Could not load meter statistics.');
    return;
  }

  const readings = await getReadings({ meterId, limit: 100 });

  const overlay = document.createElement('div');
  overlay.className = 'modal-overlay';
  overlay.innerHTML = `
    <div class="meter-detail-panel">
      <div style="display:flex;justify-content:space-between;align-items:center;margin-bottom:var(--space-6)">
        <div>
          <h2 style="font-size:var(--text-xl);margin-bottom:var(--space-1)">Meter ${formatMeterId(meterId)}</h2>
          <p style="color:var(--color-text-muted);font-size:var(--text-sm)">ID: ${meterId}</p>
        </div>
        <button class="btn" id="close-detail">✕ Close</button>
      </div>

      <div class="stats-grid" style="margin-bottom:var(--space-6)">
        <div class="stat-card accent-cyan">
          <div class="stat-label">Readings</div>
          <div class="stat-value" style="font-size:var(--text-xl)">${stats.total_readings}</div>
        </div>
        <div class="stat-card accent-red">
          <div class="stat-label">Alerts</div>
          <div class="stat-value" style="font-size:var(--text-xl)">${stats.total_alerts}</div>
        </div>
        <div class="stat-card accent-amber">
          <div class="stat-label">Anomalies</div>
          <div class="stat-value" style="font-size:var(--text-xl)">${stats.total_anomalies}</div>
        </div>
        <div class="stat-card accent-green">
          <div class="stat-label">Avg Energy</div>
          <div class="stat-value" style="font-size:var(--text-xl)">${stats.avg_energy_wh.toFixed(0)} Wh</div>
        </div>
      </div>

      <div class="glass-panel">
        <div class="panel-header">
          <span class="panel-title">Historical Energy (Wh)</span>
          <button class="btn btn-sm" id="export-meter-csv">📥 Export CSV</button>
        </div>
        <div class="panel-body">
          <div class="chart-container" id="meter-detail-chart" style="height:260px"></div>
        </div>
      </div>
    </div>
  `;

  document.body.appendChild(overlay);

  // Chart
  const sorted = [...readings].sort((a, b) => new Date(a.timestamp) - new Date(b.timestamp));
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

  // Export button
  overlay.querySelector('#export-meter-csv').addEventListener('click', () => {
    exportReadings(meterId);
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
  async function refresh() {
    const meters = await getMeters();

    container.innerHTML = `
      <div class="page-enter">
        <div class="page-header">
          <h1>Fleet Management</h1>
          <p class="page-desc">${meters.length} registered meters</p>
        </div>

        <div class="filter-bar">
          <button class="btn btn-primary" id="register-meter-btn">➕ Register Meter</button>
          <span class="badge badge-info">${meters.length} meters</span>
        </div>

        ${meters.length === 0 ? `
          <div class="glass-panel">
            <div class="empty-state">
              <div class="empty-icon">🔌</div>
              <div class="empty-text">No meters registered yet. Click "Register Meter" to add one.</div>
            </div>
          </div>
        ` : `
          <div class="meter-grid">
            ${meters.map(m => renderMeterCard(m)).join('')}
          </div>
        `}
      </div>
    `;

    // Register button
    container.querySelector('#register-meter-btn')?.addEventListener('click', () => {
      openRegisterModal(async (data) => {
        await createMeter(data);
        await refresh();
      });
    });

    // Detail buttons
    container.querySelectorAll('.meter-detail-btn').forEach(btn => {
      btn.addEventListener('click', (e) => {
        e.stopPropagation();
        const id = Number(btn.dataset.id);
        openMeterDetail(id);
      });
    });

    // Delete buttons
    container.querySelectorAll('.meter-delete-btn').forEach(btn => {
      btn.addEventListener('click', async (e) => {
        e.stopPropagation();
        const id = Number(btn.dataset.id);
        if (confirm(`Delete meter ${formatMeterId(id)}?`)) {
          await deleteMeter(id);
          await refresh();
        }
      });
    });
  }

  await refresh();
}
