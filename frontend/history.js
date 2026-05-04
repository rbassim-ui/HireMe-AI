const historyUser = window.HireMeAuth.requireAuth('login.html');
const historyApiUrl = window.HireMeAuth.API_URL;
const historyError = document.getElementById('historyError');
const historyTitle = document.getElementById('historyTitle');
const historySubtitle = document.getElementById('historySubtitle');
const historyTable = document.getElementById('historyTable');
const historyMetrics = document.getElementById('historyMetrics');
const historySummary = document.getElementById('historySummary');
const domainFilter = document.getElementById('domainFilter');
const searchFilter = document.getElementById('searchFilter');
const resetFiltersBtn = document.getElementById('resetFiltersBtn');

function showHistoryError(message) {
  if (!historyError) return;
  historyError.textContent = message;
  historyError.style.display = 'block';
}

function clearHistoryError() {
  if (!historyError) return;
  historyError.style.display = 'none';
  historyError.textContent = '';
}

function formatScore(value) {
  const score = Number(value || 0);
  return Number.isInteger(score) ? `${score}` : score.toFixed(1);
}

function renderTable(items) {
  if (!items.length) {
    return '<div class="account-empty">Aucune session ne correspond à ces filtres.</div>';
  }

  const rows = items.map((item) => `
    <tr>
      <td>${window.HireMeAuth.escapeHtml(item.date || '')}</td>
      <td>${window.HireMeAuth.escapeHtml(item.domain || '')}</td>
      <td>${window.HireMeAuth.escapeHtml(item.role || '')}</td>
      <td>${window.HireMeAuth.escapeHtml(item.level || '')}</td>
      <td><span class="account-badge">${formatScore(item.score)}/10</span></td>
      <td>${window.HireMeAuth.escapeHtml(item.badge || '')}</td>
    </tr>
  `).join('');

  return `
    <table>
      <thead>
        <tr>
          <th>Date</th>
          <th>Domaine</th>
          <th>Rôle</th>
          <th>Niveau</th>
          <th>Score</th>
          <th>Badge</th>
        </tr>
      </thead>
      <tbody>${rows}</tbody>
    </table>
  `;
}

async function loadHistory() {
  clearHistoryError();
  const params = new URLSearchParams();
  params.set('user_id', historyUser.user_id);

  const domainValue = domainFilter.value.trim();
  const searchValue = searchFilter.value.trim();
  if (domainValue) params.set('domain', domainValue);
  if (searchValue) params.set('q', searchValue);
  params.set('limit', '100');

  try {
    const response = await fetch(`${historyApiUrl}/api/account/history?${params.toString()}`);
    const data = await response.json();

    if (!response.ok || !data.success) {
      throw new Error(data.message || 'Impossible de charger l\'historique');
    }

    historyTitle.textContent = `Historique de ${historyUser.name || 'mon compte'}`;
    historySubtitle.textContent = searchValue || domainValue
      ? `Filtres actifs : ${[domainValue, searchValue].filter(Boolean).join(' • ')}`
      : 'Toutes vos sessions finalisées sont affichées ici.';

    historyMetrics.innerHTML = `
      <div class="account-stat"><span class="value">${data.total_sessions || 0}</span><span class="label">Sessions</span></div>
      <div class="account-stat"><span class="value">${formatScore(data.avg_score)}</span><span class="label">Moyenne</span></div>
      <div class="account-stat"><span class="value">${formatScore(data.best_score)}</span><span class="label">Meilleur score</span></div>
    `;

    historySummary.textContent = data.total_sessions
      ? `${data.total_sessions} session${data.total_sessions > 1 ? 's' : ''} trouvée${data.total_sessions > 1 ? 's' : ''}.`
      : 'Aucune session trouvée pour ces filtres.';

    historyTable.innerHTML = renderTable(Array.isArray(data.items) ? data.items : []);
  } catch (error) {
    showHistoryError(error.message || 'Erreur de chargement de l\'historique.');
    historyTable.innerHTML = '<div class="account-empty">Impossible de charger l\'historique.</div>';
  }
}

document.getElementById('filtersForm')?.addEventListener('submit', async (event) => {
  event.preventDefault();
  await loadHistory();
});

resetFiltersBtn?.addEventListener('click', async () => {
  domainFilter.value = '';
  searchFilter.value = '';
  await loadHistory();
});

loadHistory();