/**
 * GridShield — Chart.js wrapper with themed defaults
 */

import {
    Chart,
    LineController,
    BarController,
    LineElement,
    BarElement,
    PointElement,
    LinearScale,
    CategoryScale,
    TimeScale,
    Filler,
    Tooltip,
    Legend,
} from 'chart.js';

// Register what we need
Chart.register(
    LineController, BarController,
    LineElement, BarElement, PointElement,
    LinearScale, CategoryScale, TimeScale,
    Filler, Tooltip, Legend,
);

// Theme defaults
Chart.defaults.color = '#94a3b8';
Chart.defaults.borderColor = 'rgba(100, 120, 180, 0.1)';
Chart.defaults.font.family = "'Inter', sans-serif";
Chart.defaults.font.size = 11;
Chart.defaults.plugins.legend.labels.usePointStyle = true;
Chart.defaults.plugins.legend.labels.padding = 16;
Chart.defaults.plugins.tooltip.backgroundColor = 'rgba(15, 23, 42, 0.95)';
Chart.defaults.plugins.tooltip.borderColor = 'rgba(100, 120, 180, 0.2)';
Chart.defaults.plugins.tooltip.borderWidth = 1;
Chart.defaults.plugins.tooltip.cornerRadius = 8;
Chart.defaults.plugins.tooltip.padding = 10;

/**
 * Create a line chart inside a container element.
 * @param {HTMLElement} container
 * @param {object} config - { labels, datasets, options }
 * @returns {Chart}
 */
export function createLineChart(container, { labels, datasets, options = {} }) {
    const canvas = document.createElement('canvas');
    container.appendChild(canvas);

    return new Chart(canvas, {
        type: 'line',
        data: { labels, datasets },
        options: {
            responsive: true,
            maintainAspectRatio: false,
            interaction: { mode: 'index', intersect: false },
            scales: {
                x: {
                    grid: { display: false },
                },
                y: {
                    beginAtZero: false,
                    grid: { color: 'rgba(100, 120, 180, 0.06)' },
                },
            },
            plugins: {
                legend: { position: 'top' },
            },
            elements: {
                line: { tension: 0.35, borderWidth: 2 },
                point: { radius: 2, hoverRadius: 5 },
            },
            ...options,
        },
    });
}

/**
 * Create a bar chart inside a container element.
 */
export function createBarChart(container, { labels, datasets, options = {} }) {
    const canvas = document.createElement('canvas');
    container.appendChild(canvas);

    return new Chart(canvas, {
        type: 'bar',
        data: { labels, datasets },
        options: {
            responsive: true,
            maintainAspectRatio: false,
            scales: {
                x: { grid: { display: false } },
                y: {
                    beginAtZero: true,
                    grid: { color: 'rgba(100, 120, 180, 0.06)' },
                },
            },
            plugins: {
                legend: { display: false },
            },
            ...options,
        },
    });
}
