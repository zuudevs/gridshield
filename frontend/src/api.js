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
