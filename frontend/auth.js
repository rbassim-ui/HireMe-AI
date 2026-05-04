const AUTH_API_URL = 'http://127.0.0.1:3000';
const AUTH_STORAGE_KEY = 'hireme_user';

function getCurrentUser() {
  try {
    return JSON.parse(localStorage.getItem(AUTH_STORAGE_KEY) || 'null');
  } catch {
    return null;
  }
}

function setCurrentUser(user) {
  localStorage.setItem(AUTH_STORAGE_KEY, JSON.stringify(user));
  try {
    // Keep a short legacy key for pages that check `authUser`
    localStorage.setItem('authUser', JSON.stringify({ user_id: user.user_id, name: user.name }));
  } catch (e) {
    // ignore
  }
}

function clearCurrentUser() {
  localStorage.removeItem(AUTH_STORAGE_KEY);
  try {
    localStorage.removeItem('authUser');
  } catch (e) {}
}

function signOut() {
  clearCurrentUser();
  window.location.href = 'login.html';
}

function requireAuth(redirectTo = 'login.html') {
  const user = getCurrentUser();
  if (!user || !user.user_id) {
    window.location.href = redirectTo;
    return null;
  }
  return user;
}

function escapeHtml(value) {
  return String(value || '')
    .replace(/&/g, '&amp;')
    .replace(/</g, '&lt;')
    .replace(/>/g, '&gt;')
    .replace(/"/g, '&quot;')
    .replace(/'/g, '&#39;');
}

function renderAuthWidget() {
  const target = document.getElementById('authWidget');
  if (!target) return;

  const user = getCurrentUser();
  if (user && user.user_id) {
    target.innerHTML = `
      <a href="account.html" class="auth-link">${escapeHtml(user.name || 'Mon compte')}</a>
      <button type="button" class="auth-button auth-button--ghost" id="authSignOutBtn">Sign out</button>
    `;
    const signOutBtn = document.getElementById('authSignOutBtn');
    if (signOutBtn) {
      signOutBtn.addEventListener('click', signOut);
    }
    return;
  }

  // Don't render a redundant login button on auth pages (login/account)
  try {
    const path = location.pathname.split('/').pop();
    const isAuthPage = path === 'login.html' || path === 'account.html' || document.body.classList.contains('auth-page');
    if (isAuthPage) {
      target.innerHTML = '';
      return;
    }
  } catch (e) {}

  target.innerHTML = `
    <a href="login.html" class="auth-button">Login</a>
  `;
}

window.HireMeAuth = {
  API_URL: AUTH_API_URL,
  getCurrentUser,
  setCurrentUser,
  clearCurrentUser,
  signOut,
  requireAuth,
  renderAuthWidget,
  escapeHtml,
};

document.addEventListener('DOMContentLoaded', renderAuthWidget);
