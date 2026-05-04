const API_URL = 'http://127.0.0.1:3000';

function formatDomain(domain) {
  if (!domain) return 'Non défini';
  return domain
    .replace(/[_-]+/g, ' ')
    .replace(/\s+/g, ' ')
    .trim()
    .replace(/\b\w/g, c => c.toUpperCase());
}

function formatScore(score) {
  const n = Number(score || 0);
  return Number.isInteger(n) ? `${n}` : n.toFixed(1);
}

function metricIcon(label) {
  if (label === 'Sessions totales') {
    return '<svg viewBox="0 0 24 24" aria-hidden="true"><path d="M4 19h16M7 16V8m5 8V5m5 11v-6" stroke="currentColor" stroke-width="1.6" stroke-linecap="round" stroke-linejoin="round" /></svg>';
  }
  if (label === 'Score moyen') {
    return '<svg viewBox="0 0 24 24" aria-hidden="true"><path d="M12 4a8 8 0 1 0 8 8" stroke="currentColor" stroke-width="1.6" stroke-linecap="round" /><path d="M12 12l4-4" stroke="currentColor" stroke-width="1.6" stroke-linecap="round" stroke-linejoin="round" /></svg>';
  }
  if (label === 'Meilleur score') {
    return '<svg viewBox="0 0 24 24" aria-hidden="true"><path d="M12 3l2.6 5.3 5.8.8-4.2 4.1 1 5.8-5.2-2.7-5.2 2.7 1-5.8-4.2-4.1 5.8-.8L12 3z" stroke="currentColor" stroke-width="1.4" stroke-linejoin="round" /></svg>';
  }
  return '<svg viewBox="0 0 24 24" aria-hidden="true"><path d="M12 12a4 4 0 1 0 0-8 4 4 0 0 0 0 8zM4 20a8 8 0 0 1 16 0" stroke="currentColor" stroke-width="1.6" stroke-linecap="round" stroke-linejoin="round" /></svg>';
}

function renderMetric(label, value, sub, highlight = false) {
  return `
    <div class="metric-card ${highlight ? 'metric-highlight' : ''}">
      <div class="metric-icon">${metricIcon(label)}</div>
      <div class="metric-label">${label}</div>
      <div class="metric-value">${value}</div>
      <div class="metric-sub">${sub}</div>
    </div>
  `;
}

function renderDomains(domains) {
  if (!domains.length) {
    return '<div class="dashboard-empty">Aucune donnée disponible pour le moment.</div>';
  }

  return domains.map((item, index) => {
    const width = Math.max(6, Math.min(100, (item.avg_score || 0) * 10));
    return `
      <div class="domain-item">
        <div class="domain-badge" aria-hidden="true">
          <svg viewBox="0 0 24 24"><path d="M12 4v16M4 12h16" stroke="currentColor" stroke-width="1.5" stroke-linecap="round" /></svg>
        </div>
        <div class="domain-main">
          <div class="domain-head">
            <div>
              <div class="domain-name">${index + 1}. ${formatDomain(item.domain)}</div>
              <div class="domain-meta">${item.sessions} session${item.sessions > 1 ? 's' : ''} • meilleur ${formatScore(item.best_score)}/10</div>
            </div>
            <div class="domain-score">${formatScore(item.avg_score)}</div>
          </div>
          <div class="domain-bar"><span style="width:${width}%"></span></div>
        </div>
      </div>
    `;
  }).join('');
}

async function loadDashboard() {
  const heroMetrics = document.getElementById('heroMetrics');
  const domainList = document.getElementById('domainList');
  const bestDomainChip = document.getElementById('bestDomainChip');
  const avgScoreText = document.getElementById('avgScoreText');
  const summaryNote = document.getElementById('summaryNote');

  try {
    const response = await fetch(`${API_URL}/api/stats`);
    if (!response.ok) throw new Error(`HTTP ${response.status}`);

    const data = await response.json();
    const avgScore = Number(data.avg_score || 0);
    const bestScore = Number(data.best_score || 0);
    const totalSessions = Number(data.total_sessions || 0);
    const bestDomain = formatDomain(data.best_domain);
    const bestDomainAvg = Number(data.best_domain_avg || 0);
    const bestDomainSessions = Number(data.best_domain_sessions || 0);
    const domains = Array.isArray(data.domains) ? data.domains : [];

    heroMetrics.innerHTML = [
      renderMetric('Sessions totales', totalSessions, 'Toutes les sessions finalisées enregistrées'),
      renderMetric('Score moyen', formatScore(avgScore), 'Moyenne globale des sessions scorées', true),
      renderMetric('Meilleur score', `${formatScore(bestScore)}/10`, 'Pic de performance observé'),
      renderMetric('Meilleur domaine', bestDomain, `${bestDomainSessions} session${bestDomainSessions > 1 ? 's' : ''} • moyenne ${formatScore(bestDomainAvg)}/10`)
    ].join('');

    domainList.innerHTML = renderDomains(domains);
    bestDomainChip.textContent = bestDomain !== 'Non défini'
      ? `Top domaine · ${bestDomain}`
      : 'Top domaine indisponible';
    avgScoreText.textContent = formatScore(avgScore);
    summaryNote.textContent = totalSessions
      ? `Les ${totalSessions} sessions analysées donnent une vue d’ensemble stable des performances par domaine.`
      : 'Aucune session finalisée n’est encore disponible pour générer des statistiques.';
  } catch (err) {
    heroMetrics.innerHTML = [
      renderMetric('Sessions totales', '—', 'API REST indisponible'),
      renderMetric('Score moyen', '—', 'Chargement des statistiques impossible', true),
      renderMetric('Meilleur score', '—', 'Aucune donnée chargée'),
      renderMetric('Meilleur domaine', '—', 'Connexion à l’API échouée')
    ].join('');

    domainList.innerHTML = '<div class="dashboard-empty">Impossible de charger les statistiques pour le moment.</div>';
    bestDomainChip.textContent = 'Erreur de chargement';
    avgScoreText.textContent = '—';
    summaryNote.textContent = 'Vérifiez que le backend est démarré sur le port 3000.';
    console.warn('Dashboard stats failed:', err);
  }
}

loadDashboard();
