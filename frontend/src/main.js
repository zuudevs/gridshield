/**
 * GridShield — Main Entry Point
 * Sets up the SPA shell and router.
 */

import './style.css';
import { renderNavbar, initMobileToggle } from './components/navbar.js';
import { route, startRouter } from './router.js';
import { getNotificationSummary } from './api.js';
import renderDashboard from './pages/dashboard.js';
import renderAlerts from './pages/alerts.js';
import renderAnomalies from './pages/anomalies.js';
import renderFleet from './pages/fleet.js';
import renderNotifications from './pages/notifications.js';

// App shell
const app = document.getElementById('app');
app.innerHTML = `
  ${renderNavbar()}
  <main class="main-content" id="page-content"></main>
`;

initMobileToggle();

// Register routes
route('#/', renderDashboard);
route('#/alerts', renderAlerts);
route('#/anomalies', renderAnomalies);
route('#/fleet', renderFleet);
route('#/notifications', renderNotifications);

// Start router
const pageContent = document.getElementById('page-content');
startRouter(pageContent);

// Notification badge polling
async function updateNotifBadge() {
  try {
    const summary = await getNotificationSummary();
    const badge = document.getElementById('nav-notif-badge');
    if (badge) {
      if (summary.unread_count > 0) {
        badge.textContent = summary.unread_count > 99 ? '99+' : summary.unread_count;
        badge.style.display = '';
      } else {
        badge.style.display = 'none';
      }
    }
  } catch (_) { /* ignore */ }
}
updateNotifBadge();
setInterval(updateNotifBadge, 15000);
