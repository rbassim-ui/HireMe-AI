const loginError = document.getElementById('authError');
const loginSuccess = document.getElementById('authSuccess');
const registerForm = document.getElementById('registerForm');
const loginForm = document.getElementById('loginForm');

function showAuthMessage(message, isSuccess = false) {
  if (loginSuccess) loginSuccess.style.display = 'none';
  if (loginError) loginError.style.display = 'none';

  const target = isSuccess ? loginSuccess : loginError;
  if (!target) return;
  target.textContent = message;
  target.style.display = 'block';
}

async function submitAuth(path, payload, successMessage) {
  const response = await fetch(`${window.HireMeAuth.API_URL}${path}`, {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify(payload),
  });

  const data = await response.json();
  if (!response.ok || !data.success) {
    throw new Error(data.message || 'Authentication failed');
  }

  window.HireMeAuth.setCurrentUser({
    user_id: data.user_id,
    name: data.name,
  });
  showAuthMessage(successMessage, true);
  window.location.href = 'account.html';
}

if (window.HireMeAuth.getCurrentUser()) {
  showAuthMessage('Vous êtes déjà connecté. Vous pouvez ouvrir votre espace compte.', true);
}

registerForm?.addEventListener('submit', async (event) => {
  event.preventDefault();
  const name = document.getElementById('registerName').value.trim();
  const password = document.getElementById('registerPassword').value.trim();

  if (!name || !password) {
    showAuthMessage("Le nom d'utilisateur et le mot de passe sont requis.");
    return;
  }

  try {
    await submitAuth('/api/register', { name, password }, 'Compte créé avec succès.');
  } catch (error) {
    showAuthMessage(error.message || 'Impossible de créer le compte.');
  }
});

loginForm?.addEventListener('submit', async (event) => {
  event.preventDefault();
  const name = document.getElementById('loginName').value.trim();
  const password = document.getElementById('loginPassword').value.trim();

  if (!name || !password) {
    showAuthMessage("Le nom d'utilisateur et le mot de passe sont requis.");
    return;
  }

  try {
    await submitAuth('/api/login', { name, password }, 'Connexion réussie.');
  } catch (error) {
    showAuthMessage(error.message || 'Connexion impossible.');
  }
});
