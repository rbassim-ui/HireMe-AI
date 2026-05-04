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

  // Sauvegarder localement pour le frontend
  const sessionData = { 
    session_id: Math.random().toString(36).substring(7),
    user_id: currentUser?.user_id,
    name, domain, role, level, 
    startedAt: new Date().toISOString()
  };
  
  localStorage.setItem('hireme_session', JSON.stringify(sessionData));
  
  // Appel API optionnel - ne pas bloquer si indisponible
  fetch('http://127.0.0.1:3000/api/session', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify(sessionData)
  }).catch(() => {
    console.warn('API session indisponible, session sauvegardée localement');
  });
  
  window.location.href = 'interview.html';
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