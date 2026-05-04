function setLevel(el) {
  document.querySelectorAll('.level-pills .pill').forEach(p => p.classList.remove('active'));
  el.classList.add('active');
}

const currentUser = window.HireMeAuth?.getCurrentUser?.() || null;

window.addEventListener('DOMContentLoaded', () => {
  if (!currentUser) return;
  const nameInput = document.getElementById('nameInput');
  if (nameInput) {
    nameInput.value = currentUser.name || '';
    nameInput.readOnly = true;
    nameInput.placeholder = 'Compte connecté';
  }
});

function launch() {
  const nameInput = document.getElementById('nameInput');
  const name   = (currentUser?.name || nameInput.value).trim();
  const domain = document.getElementById('domainInput').value.trim();
  const role   = document.getElementById('roleInput').value.trim();
  const level  = document.querySelector('.level-pills .pill.active')?.textContent || 'Débutant';

  if (!name || !domain || !role) { shake(); return; }

  // Appeler l'API backend pour sauvegarder la session
  const sessionData = { name, domain, role, level, startedAt: new Date().toISOString() };
  if (currentUser?.user_id) {
    sessionData.user_id = currentUser.user_id;
  }
  
  fetch('http://127.0.0.1:3000/api/session', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify(sessionData)
  })
  .then(res => res.json())
  .then(data => {
    if (data.success) {
      // Sauvegarder aussi localement pour le frontend
      localStorage.setItem('hireme_session', JSON.stringify({
        session_id: data.session_id,
        user_id: data.user_id,
        name, domain, role, level
      }));
      window.location.href = 'interview.html';
    } else {
      alert('Erreur: ' + (data.message || 'Impossible de créer la session'));
    }
  })
  .catch(err => {
    console.error('Erreur API:', err);
    alert('Erreur de connexion au serveur. Vérifiez que le backend est lancé sur port 3000.');
  });
}

function shake() {
  const card = document.querySelector('.card');
  card.style.transform = 'translateX(-6px)';
  setTimeout(() => card.style.transform = 'translateX(6px)', 80);
  setTimeout(() => card.style.transform = 'translateX(-4px)', 160);
  setTimeout(() => card.style.transform = 'translateX(0)', 240);
}