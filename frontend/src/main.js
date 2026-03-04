/**
 * GridShield — Main Entry Point
 * Sets up the SPA shell and router.
 */

import './style.css';
import { renderNavbar, initMobileToggle } from './components/navbar.js';
import { route, startRouter } from './router.js';
import renderDashboard from './pages/dashboard.js';
import renderAlerts from './pages/alerts.js';
import renderAnomalies from './pages/anomalies.js';
import renderFleet from './pages/fleet.js';

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

// Start router
const pageContent = document.getElementById('page-content');
startRouter(pageContent);
