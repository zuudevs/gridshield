/**
 * GridShield — Sidebar Navigation Component
 */

export function renderNavbar() {
  return `
    <button class="mobile-toggle" id="mobile-toggle">☰</button>
    <aside class="sidebar" id="sidebar">
      <div class="sidebar-brand">
        <div class="brand-icon">⚡</div>
        <div class="brand-text">
          <span class="brand-name">GridShield</span>
          <span class="brand-sub">AMI Security v3.3</span>
        </div>
      </div>

      <nav class="sidebar-nav">
        <div class="nav-label">Monitoring</div>
        <a class="nav-item" data-href="#/" href="#/">
          <span class="nav-icon">📊</span>
          <span>Dashboard</span>
        </a>
        <a class="nav-item" data-href="#/alerts" href="#/alerts">
          <span class="nav-icon">🚨</span>
          <span>Tamper Alerts</span>
        </a>
        <a class="nav-item" data-href="#/anomalies" href="#/anomalies">
          <span class="nav-icon">📈</span>
          <span>Anomalies</span>
        </a>

        <div class="nav-label">Management</div>
        <a class="nav-item" data-href="#/fleet" href="#/fleet">
          <span class="nav-icon">🔌</span>
          <span>Fleet</span>
        </a>
        <a class="nav-item" data-href="#/notifications" href="#/notifications">
          <span class="nav-icon">🔔</span>
          <span>Notifications</span>
          <span class="notif-badge" id="nav-notif-badge" style="display:none"></span>
        </a>
      </nav>

      <div class="sidebar-footer">
        <span class="live-dot"></span> System Online — v3.3.0
      </div>
    </aside>
  `;
}

export function initMobileToggle() {
  const toggle = document.getElementById('mobile-toggle');
  const sidebar = document.getElementById('sidebar');
  if (toggle && sidebar) {
    toggle.addEventListener('click', () => {
      sidebar.classList.toggle('open');
    });
    // Close sidebar on nav click (mobile)
    sidebar.querySelectorAll('.nav-item').forEach(item => {
      item.addEventListener('click', () => {
        sidebar.classList.remove('open');
      });
    });
  }
}
