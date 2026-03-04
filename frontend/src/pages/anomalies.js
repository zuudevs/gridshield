/**
 * GridShield — Anomalies Page
 * Anomaly detection logs with deviation visualization
 */

import { getAnomalies } from '../api.js';

function severityBadge(sev) {
    const s = (sev || '').toLowerCase();
    if (s === 'critical') return '<span class="badge badge-critical">CRITICAL</span>';
    if (s === 'high') return '<span class="badge badge-high">HIGH</span>';
    if (s === 'medium') return '<span class="badge badge-medium">MEDIUM</span>';
    return '<span class="badge badge-low">LOW</span>';
}

function deviationBar(pct) {
    const clamped = Math.min(Math.abs(pct), 100);
    let color = 'var(--color-green)';
    if (clamped >= 80) color = 'var(--color-red)';
    else if (clamped >= 50) color = 'var(--color-orange)';
    else if (clamped >= 30) color = 'var(--color-amber)';
    return `
    <div style="display:flex;align-items:center;gap:8px;">
      <div class="deviation-bar-container">
        <div class="deviation-bar" style="width:${clamped}%;background:${color}"></div>
      </div>
      <span style="font-family:var(--font-mono);font-size:var(--text-xs);color:${color}">${pct.toFixed(1)}%</span>
    </div>
  `;
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

export default async function renderAnomalies(container) {
    let typeFilter = 'all';

    async function refresh() {
        const anomalies = await getAnomalies({ limit: 100 });

        const types = [...new Set(anomalies.map(a => a.anomaly_type))];
        const filtered = typeFilter === 'all'
            ? anomalies
            : anomalies.filter(a => a.anomaly_type === typeFilter);

        container.innerHTML = `
      <div class="page-enter">
        <div class="page-header">
          <h1>Anomaly Detection</h1>
          <p class="page-desc">Behavioral analysis and consumption anomaly logs</p>
        </div>

        <div class="filter-bar">
          <select id="anomaly-type-filter">
            <option value="all">All Types</option>
            ${types.map(t => `<option value="${t}" ${typeFilter === t ? 'selected' : ''}>${t.replace(/_/g, ' ')}</option>`).join('')}
          </select>
          <span class="badge badge-info">${filtered.length} events</span>
        </div>

        <div class="glass-panel">
          ${filtered.length === 0 ? `
            <div class="empty-state">
              <div class="empty-icon">🔍</div>
              <div class="empty-text">No anomalies detected</div>
            </div>
          ` : `
            <table class="data-table">
              <thead>
                <tr>
                  <th>Time</th>
                  <th>Meter</th>
                  <th>Type</th>
                  <th>Severity</th>
                  <th>Current</th>
                  <th>Expected</th>
                  <th>Deviation</th>
                  <th>Confidence</th>
                </tr>
              </thead>
              <tbody>
                ${filtered.map(a => `
                  <tr>
                    <td>${formatTime(a.timestamp)}</td>
                    <td style="font-family:var(--font-mono);color:var(--color-accent)">${formatMeterId(a.meter_id)}</td>
                    <td>${a.anomaly_type.replace(/_/g, ' ')}</td>
                    <td>${severityBadge(a.severity)}</td>
                    <td style="font-family:var(--font-mono)">${a.current_value.toFixed(1)}</td>
                    <td style="font-family:var(--font-mono)">${a.expected_value.toFixed(1)}</td>
                    <td>${deviationBar(a.deviation_percent)}</td>
                    <td>
                      <span style="font-family:var(--font-mono)">${a.confidence}%</span>
                    </td>
                  </tr>
                `).join('')}
              </tbody>
            </table>
          `}
        </div>
      </div>
    `;

        document.getElementById('anomaly-type-filter')?.addEventListener('change', (e) => {
            typeFilter = e.target.value;
            refresh();
        });
    }

    await refresh();
}
