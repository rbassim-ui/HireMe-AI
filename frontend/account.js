const accountUser = window.HireMeAuth.requireAuth('login.html');
const API_URL = window.HireMeAuth.API_URL;
const accountError = document.getElementById('accountError');
const accountSuccess = document.getElementById('accountSuccess');
const accountName = document.getElementById('accountName');
const accountSubtitle = document.getElementById('accountSubtitle');
const accountNameInput = document.getElementById('accountNameInput');
const accountPasswordInput = document.getElementById('accountPasswordInput');
const summaryGrid = document.getElementById('summaryGrid');
const historyTable = document.getElementById('historyTable');
const deleteAccountBtn = document.getElementById('deleteAccountBtn');

if (!accountUser) {
  console.warn('Account page requires an authenticated user.');
} else {

function showAccountMessage(message, isSuccess = false) {
  if (accountError) accountError.style.display = 'none';
  if (accountSuccess) accountSuccess.style.display = 'none';
  const target = isSuccess ? accountSuccess : accountError;
  if (!target) return;
  target.textContent = message;
  target.style.display = 'block';
}

function formatScore(value) {
  const score = Number(value || 0);
  return Number.isInteger(score) ? `${score}` : score.toFixed(1);
}

function renderHistory(items) {
  if (!items.length) {
    return '<div class="account-empty">Aucune session enregistrée pour ce compte.</div>';
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

async function loadAccount() {
  try {
    const response = await fetch(`${API_URL}/api/account?user_id=${accountUser.user_id}`);
    const data = await response.json();

    if (!response.ok || !data.success) {
      throw new Error(data.message || 'Impossible de charger le compte');
    }

    accountName.textContent = `Bonjour ${data.user.name}`;
    accountSubtitle.textContent = `Compte créé le ${data.user.created_at || '—'} · profil personnel connecté.`;
    accountNameInput.value = data.user.name || '';

    summaryGrid.innerHTML = `
      <div class="account-stat"><span class="value">${data.stats.total_sessions || 0}</span><span class="label">Sessions</span></div>
      <div class="account-stat"><span class="value">${formatScore(data.stats.avg_score)}</span><span class="label">Moyenne</span></div>
      <div class="account-stat"><span class="value">${formatScore(data.stats.best_score)}</span><span class="label">Meilleur score</span></div>
    `;

    historyTable.innerHTML = renderHistory(Array.isArray(data.sessions) ? data.sessions : []);
    window.HireMeAuth.setCurrentUser({
      user_id: accountUser.user_id,
      name: data.user.name,
    });
  } catch (error) {
    showAccountMessage(error.message || 'Erreur de chargement du compte.');
    historyTable.innerHTML = '<div class="account-empty">Impossible de charger l\'historique.</div>';
  }
}

async function postAccount(path, payload) {
  const response = await fetch(`${API_URL}${path}`, {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify(payload),
  });

  const data = await response.json();
  if (!response.ok || !data.success) {
    throw new Error(data.message || 'Action impossible');
  }
  return data;
}

document.getElementById('updateNameForm')?.addEventListener('submit', async (event) => {
  event.preventDefault();
  const nextName = accountNameInput.value.trim();
  if (!nextName) {
    showAccountMessage('Le nom ne peut pas être vide.');
    return;
  }

  try {
    await postAccount('/api/account/update-name', {
      user_id: accountUser.user_id,
      name: nextName,
    });
    showAccountMessage('Nom mis à jour.', true);
    await loadAccount();
  } catch (error) {
    showAccountMessage(error.message || 'Impossible de mettre à jour le nom.');
  }
});

document.getElementById('passwordForm')?.addEventListener('submit', async (event) => {
  event.preventDefault();
  const nextPassword = accountPasswordInput.value.trim();
  if (!nextPassword) {
    showAccountMessage('Le mot de passe ne peut pas être vide.');
    return;
  }

  try {
    await postAccount('/api/account/change-password', {
      user_id: accountUser.user_id,
      password: nextPassword,
    });
    accountPasswordInput.value = '';
    showAccountMessage('Mot de passe mis à jour.', true);
  } catch (error) {
    showAccountMessage(error.message || 'Impossible de changer le mot de passe.');
  }
});

deleteAccountBtn?.addEventListener('click', async () => {
  const confirmed = window.confirm('Supprimer ce compte supprimera aussi les sessions et scores associés. Continuer ?');
  if (!confirmed) return;

  try {
    await postAccount('/api/account/delete', {
      user_id: accountUser.user_id,
    });
    window.HireMeAuth.clearCurrentUser();
    window.location.href = 'login.html';
  } catch (error) {
    showAccountMessage(error.message || 'Impossible de supprimer le compte.');
  }
});

document.getElementById('authWidget')?.addEventListener('click', (event) => {
  if (event.target && event.target.id === 'authSignOutBtn') {
    window.HireMeAuth.signOut();
  }
});

loadAccount();
}
