const result = JSON.parse(localStorage.getItem('hireme_result') || '{}');

function getBadge(avg) {
  if (avg >= 8.5) return { icon: '🏆', label: 'Excellent Candidate', sub: 'Top performer' };
  if (avg >= 7)   return { icon: '⭐', label: 'Good Potential',       sub: 'Solid performance' };
  if (avg >= 5)   return { icon: '📈', label: 'Rising Profile',        sub: 'À développer' };
  return              { icon: '🎯', label: 'Needs Practice',         sub: 'Keep going!' };
}

function getScoreClass(s) {
  if (s >= 8) return 'score-high';
  if (s >= 5) return 'score-mid';
  return 'score-low';
}

function render() {
  const avg     = parseFloat(result.average) || 0;
  const scores  = result.scores   || [];
  const answers = result.answers  || [];
  const questions = result.questions || [];
  const badge   = getBadge(avg);

  document.getElementById('hero').innerHTML = `
    <div class="badge-wrap">
      <div class="badge">
        ${avg >= 8.5
          ? `<div class="badge-icon">${badge.icon}</div>`
          : `<div class="badge-score">${avg}</div><div class="badge-max">/ 10</div>`}
      </div>
      <div>
        <div class="badge-label">${badge.label}</div>
        <div class="badge-sublabel">${badge.sub}</div>
      </div>
    </div>
    <h1 class="hero-title">Entretien terminé, ${result.name || 'Candidat'} !</h1>
    <p class="hero-sub">Rapport pour <strong>${result.role || 'Professionnel'}</strong> — niveau <strong>${result.level || 'Débutant'}</strong>.</p>
  `;

  const best     = scores.length ? Math.max(...scores) : 0;
  const answered = answers.filter(a => a !== '[Passé]' && a !== '[Aucune réponse]').length;

  document.getElementById('statsRow').innerHTML = `
    <div class="stat-box"><div class="stat-num">${avg}</div><div class="stat-lbl">Score moyen</div></div>
    <div class="stat-box"><div class="stat-num">${best}</div><div class="stat-lbl">Meilleur score</div></div>
    <div class="stat-box"><div class="stat-num">${answered}</div><div class="stat-lbl">Réponses données</div></div>
    <div class="stat-box"><div class="stat-num">${Math.round((avg/10)*100)}%</div><div class="stat-lbl">Taux de réussite</div></div>
  `;

  document.getElementById('sections').innerHTML = `
    <div class="section">
      <div class="section-title">Détail des réponses</div>
      <div class="qa-list">
        ${scores.map((s, i) => `
          <div class="qa-item">
            <div class="qa-header">
              <div class="qa-q">Q${i+1}. ${questions[i] || 'Question ' + (i+1)}</div>
              <div class="qa-score ${getScoreClass(s)}">${s}<span style="font-size:0.7rem;color:var(--muted)">/10</span></div>
            </div>
            <div class="qa-answer">${answers[i] && answers[i].length > 10
              ? answers[i].substring(0, 160) + (answers[i].length > 160 ? '…' : '')
              : '<em>Aucune réponse</em>'}</div>
          </div>
        `).join('')}
      </div>
    </div>
    <div class="section">
      <div class="section-title">Performance par question</div>
      <div class="score-bars">
        ${scores.map((s, i) => `
          <div class="bar-item">
            <div class="bar-label">Question ${i+1}</div>
            <div class="bar-track"><div class="bar-fill" data-width="${s*10}" style="width:0%"></div></div>
            <div class="bar-val">${s}/10</div>
          </div>
        `).join('')}
      </div>
    </div>
    <div class="section">
      <div class="section-title">Conseils personnalisés</div>
      <div class="advice-list">
        ${generateAdvice(avg, scores).map((a, i) => `
          <div class="advice-item">
            <div class="advice-num">${i+1}</div>
            <div class="advice-text">${a}</div>
          </div>
        `).join('')}
      </div>
    </div>
  `;

  document.getElementById('actions').innerHTML = `
    <a href="dashboard.html" class="btn-secondary">Voir le dashboard</a>
    <a href="index.html" class="btn-primary">Recommencer →</a>
    <button class="btn-secondary" onclick="window.print()">Exporter PDF</button>
  `;

  setTimeout(() => {
    document.querySelectorAll('.bar-fill').forEach(b => b.style.width = b.dataset.width + '%');
  }, 600);
}

function generateAdvice(avg, scores) {
  const advice = [];
  if (avg < 6) advice.push("<strong>Pratiquez la méthode STAR</strong> (Situation, Tâche, Action, Résultat) pour structurer vos réponses de manière claire et impactante.");
  if (Math.min(...scores) < 5) advice.push("<strong>Identifiez vos lacunes</strong> sur les questions où vous avez le moins bien performé et consacrez du temps à les approfondir.");
  if (avg >= 7) advice.push("<strong>Affinez votre storytelling</strong> : vous maîtrisez les fondamentaux, travaillez maintenant sur l'impact de vos réponses.");
  advice.push("<strong>Répétez à voix haute</strong> : s'entraîner en parlant améliore significativement votre fluidité en entretien réel.");
  advice.push(`<strong>Prochaine étape :</strong> relancez une simulation avec le niveau ${avg >= 7 ? 'supérieur' : 'actuel'} pour continuer à progresser.`);
  return advice.slice(0, 3);
}

render();