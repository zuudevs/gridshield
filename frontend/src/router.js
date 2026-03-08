/**
 * GridShield — Simple hash-based SPA router
 */

const routes = {};
let currentCleanup = null;

export function route(hash, handler) {
    routes[hash] = handler;
}

export function navigate(hash) {
    window.location.hash = hash;
}

export function startRouter(container) {
    async function render() {
        const hash = window.location.hash || '#/';
        const handler = routes[hash] || routes['#/'];

        // Cleanup previous page (e.g. timers)
        if (currentCleanup && typeof currentCleanup === 'function') {
            currentCleanup();
            currentCleanup = null;
        }

        if (handler) {
            container.innerHTML = '<div class="loading-spinner"></div>';
            try {
                const cleanup = await handler(container);
                if (typeof cleanup === 'function') {
                    currentCleanup = cleanup;
                }
            } catch (err) {
                console.error('Page render error:', err);
                container.innerHTML = `
          <div class="empty-state">
            <div class="empty-icon">⚠️</div>
            <div class="empty-text">Failed to load page: ${err.message}</div>
          </div>
        `;
            }
        }

        // Update nav active state
        document.querySelectorAll('.nav-item').forEach(item => {
            const href = item.dataset.href;
            item.classList.toggle('active', href === hash);
        });
    }

    window.addEventListener('hashchange', render);
    render();
}
