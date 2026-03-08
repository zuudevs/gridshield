/**
 * GridShield — Notifications Page
 * Notification center with read/unread management
 */

import {
    getNotifications,
    markNotificationRead,
    markAllNotificationsRead,
} from '../api.js';

function timeAgo(ts) {
    const diff = Date.now() - new Date(ts).getTime();
    const mins = Math.floor(diff / 60000);
    if (mins < 1) return 'just now';
    if (mins < 60) return `${mins}m ago`;
    const hrs = Math.floor(mins / 60);
    if (hrs < 24) return `${hrs}h ago`;
    return `${Math.floor(hrs / 24)}d ago`;
}

function severityBadge(sev) {
    const map = {
        critical: 'badge-critical',
        high: 'badge-critical',
        medium: 'badge-warning',
        low: 'badge-info',
        info: 'badge-info',
    };
    return map[sev] || 'badge-info';
}

function typeIcon(type) {
    if (type === 'tamper_alert') return '🚨';
    if (type === 'anomaly') return '⚠️';
    if (type === 'forensics') return '🔍';
    return '🔔';
}

export default async function renderNotifications(container) {
    let filter = null; // null = all, true = unread, false = read

    async function refresh() {
        const opts = {};
        if (filter !== null) opts.isRead = filter === false ? false : true;
        // Invert: filter=true means show unread (is_read=false)
        const fetchOpts = {};
        if (filter === true) fetchOpts.isRead = false;
        else if (filter === false) fetchOpts.isRead = true;

        const notifications = await getNotifications(fetchOpts);

        container.innerHTML = `
      <div class="page-enter">
        <div class="page-header">
          <h1>Notifications</h1>
          <p class="page-desc"><span class="live-dot"></span>Notification center</p>
        </div>

        <div class="notif-toolbar">
          <div class="notif-filters">
            <button class="btn btn-sm ${filter === null ? 'btn-active' : ''}" id="filter-all">All</button>
            <button class="btn btn-sm ${filter === true ? 'btn-active' : ''}" id="filter-unread">Unread</button>
            <button class="btn btn-sm ${filter === false ? 'btn-active' : ''}" id="filter-read">Read</button>
          </div>
          <button class="btn btn-sm btn-primary" id="mark-all-read">✓ Mark all read</button>
        </div>

        <div class="glass-panel">
          <div class="panel-body" id="notif-list">
            ${notifications.length === 0
                ? `<div class="empty-state">
                   <div class="empty-icon">🔔</div>
                   <div class="empty-text">No notifications</div>
                 </div>`
                : notifications.map(n => `
                <div class="notif-item ${n.is_read ? 'notif-read' : 'notif-unread'}" data-id="${n.id}">
                  <span class="notif-icon">${typeIcon(n.notification_type)}</span>
                  <div class="notif-content">
                    <div class="notif-message">${n.message}</div>
                    <div class="notif-meta">
                      <span class="badge ${severityBadge(n.severity)}">${n.severity}</span>
                      <span class="notif-time">${timeAgo(n.timestamp)}</span>
                      ${n.meter_id ? `<span class="notif-meter">Meter ${n.meter_id.toString(16).toUpperCase().slice(-8)}</span>` : ''}
                    </div>
                  </div>
                  ${!n.is_read ? `<button class="btn btn-xs notif-read-btn" data-nid="${n.id}">Mark read</button>` : '<span class="badge badge-success">Read</span>'}
                </div>
              `).join('')
            }
          </div>
        </div>
      </div>
    `;

        // Filter buttons
        document.getElementById('filter-all')?.addEventListener('click', () => {
            filter = null;
            refresh();
        });
        document.getElementById('filter-unread')?.addEventListener('click', () => {
            filter = true;
            refresh();
        });
        document.getElementById('filter-read')?.addEventListener('click', () => {
            filter = false;
            refresh();
        });

        // Mark all read
        document.getElementById('mark-all-read')?.addEventListener('click', async () => {
            await markAllNotificationsRead();
            refresh();
        });

        // Individual mark read buttons
        document.querySelectorAll('.notif-read-btn').forEach(btn => {
            btn.addEventListener('click', async (e) => {
                e.stopPropagation();
                const nid = btn.dataset.nid;
                await markNotificationRead(nid);
                refresh();
            });
        });
    }

    await refresh();
}
