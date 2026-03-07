/**
 * GridShield API Client
 * Wraps fetch() for all backend endpoints.
 */

const BASE = '/api';

async function request(path, options = {}) {
    const res = await fetch(`${BASE}${path}`, {
        headers: { 'Content-Type': 'application/json', ...options.headers },
        ...options,
    });
    if (!res.ok) {
        const text = await res.text();
        throw new Error(`API ${res.status}: ${text}`);
    }
    return res.json();
}

/** System status summary */
export function getStatus() {
    return request('/status');
}

/** List meter readings */
export function getReadings({ meterId, limit = 50 } = {}) {
    const params = new URLSearchParams();
    if (meterId != null) params.set('meter_id', meterId);
    if (limit) params.set('limit', limit);
    const qs = params.toString();
    return request(`/readings${qs ? '?' + qs : ''}`);
}

/** List tamper alerts */
export function getAlerts({ meterId, acknowledged, limit = 50 } = {}) {
    const params = new URLSearchParams();
    if (meterId != null) params.set('meter_id', meterId);
    if (acknowledged != null) params.set('acknowledged', acknowledged);
    if (limit) params.set('limit', limit);
    const qs = params.toString();
    return request(`/alerts${qs ? '?' + qs : ''}`);
}

/** Acknowledge a tamper alert */
export function acknowledgeAlert(id) {
    return request(`/alerts/${id}/acknowledge`, { method: 'PATCH' });
}

/** List anomaly logs */
export function getAnomalies({ meterId, limit = 50 } = {}) {
    const params = new URLSearchParams();
    if (meterId != null) params.set('meter_id', meterId);
    if (limit) params.set('limit', limit);
    const qs = params.toString();
    return request(`/anomalies${qs ? '?' + qs : ''}`);
}

// ============================================================================
// Meter CRUD
// ============================================================================

/** List all registered meters */
export function getMeters({ status } = {}) {
    const params = new URLSearchParams();
    if (status) params.set('status', status);
    const qs = params.toString();
    return request(`/meters${qs ? '?' + qs : ''}`);
}

/** Register a new meter */
export function createMeter(data) {
    return request('/meters', {
        method: 'POST',
        body: JSON.stringify(data),
    });
}

/** Update meter info */
export function updateMeter(meterId, data) {
    return request(`/meters/${meterId}`, {
        method: 'PATCH',
        body: JSON.stringify(data),
    });
}

/** Delete a meter */
export function deleteMeter(meterId) {
    return fetch(`${BASE}/meters/${meterId}`, { method: 'DELETE' });
}

/** Get meter statistics */
export function getMeterStats(meterId) {
    return request(`/meters/${meterId}/stats`);
}

// ============================================================================
// CSV Export — triggers download
// ============================================================================

function triggerDownload(url, fallbackName) {
    const a = document.createElement('a');
    a.href = `${BASE}${url}`;
    a.download = fallbackName;
    document.body.appendChild(a);
    a.click();
    a.remove();
}

/** Export readings as CSV */
export function exportReadings(meterId) {
    const params = meterId != null ? `?meter_id=${meterId}` : '';
    triggerDownload(`/export/readings${params}`, 'gridshield_readings.csv');
}

/** Export alerts as CSV */
export function exportAlerts(meterId) {
    const params = meterId != null ? `?meter_id=${meterId}` : '';
    triggerDownload(`/export/alerts${params}`, 'gridshield_alerts.csv');
}

/** Export anomalies as CSV */
export function exportAnomalies(meterId) {
    const params = meterId != null ? `?meter_id=${meterId}` : '';
    triggerDownload(`/export/anomalies${params}`, 'gridshield_anomalies.csv');
}
