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

  localStorage.setItem('hireme_session', JSON.stringify({
    name,
    domain,
    role,
    level,
    startedAt: new Date().toISOString(),
  }));

  window.location.href = 'interview.html';
}

function shake() {
  const card = document.querySelector('.card');
  card.style.transform = 'translateX(-6px)';
  setTimeout(() => card.style.transform = 'translateX(6px)', 80);
  setTimeout(() => card.style.transform = 'translateX(-4px)', 160);
  setTimeout(() => card.style.transform = 'translateX(0)', 240);
}