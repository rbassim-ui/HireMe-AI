const rolesMap = {
  tech:      ['Développeur Frontend','Développeur Backend','Fullstack','DevOps / SRE','Mobile (iOS/Android)','Architecte logiciel'],
  design:    ['UX Designer','UI Designer','Product Designer','Motion Designer','Brand Designer'],
  data:      ['Data Analyst','Data Scientist','ML Engineer','Data Engineer','BI Developer'],
  finance:   ['Analyste financier','Contrôleur de gestion','Consultant','Auditeur','CFO / DAF'],
  marketing: ['Growth Hacker','Content Strategist','SEO Manager','Performance Manager','CMO'],
  product:   ['Product Manager','Product Owner','Chief Product Officer','Agile Coach'],
};

function updateRoles() {
  const domain = document.getElementById('domainSelect').value;
  const sel = document.getElementById('roleSelect');
  sel.innerHTML = '';
  if (!domain) {
    sel.innerHTML = "<option value=''>— Domaine d'abord —</option>";
    return;
  }
  rolesMap[domain].forEach(r => {
    const o = document.createElement('option');
    o.textContent = r;
    o.value = r;
    sel.appendChild(o);
  });
}

function setLevel(el) {
  document.querySelectorAll('.level-pills .pill').forEach(p => p.classList.remove('active'));
  el.classList.add('active');
}

function launch() {
  const name   = document.getElementById('nameInput').value.trim();
  const domain = document.getElementById('domainSelect').value;
  const role   = document.getElementById('roleSelect').value;
  const level  = document.querySelector('.level-pills .pill.active')?.textContent || 'Débutant';

  if (!name || !domain || !role) { shake(); return; }

  // Appeler l'API backend pour sauvegarder la session
  const sessionData = { name, domain, role, level, startedAt: new Date().toISOString() };
  
  fetch('http://localhost:3000/api/session', {
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