// ══════════════════════════════════════════════
//  HireMe AI — interview.js
// ══════════════════════════════════════════════

const session = JSON.parse(localStorage.getItem('hireme_session') || '{}');
const TOTAL_Q = 5;
// Use relative paths for Vercel serverless functions (works on any domain)
const API_URL = '';
const DEMO_MODE = true; // Use local questions bank if API unavailable

let currentQ = 0;
let scores   = [];
let answers  = [];
let timerInterval = null;
let timeLeft = 60;

// ── Questions bank (fallback) ──
const questionsBank = {
  tech: {
    'Développeur Frontend': {
      Débutant:      ["Quelle est la différence entre `display: flex` et `display: grid` en CSS ?","Comment fonctionne le DOM et comment JavaScript interagit-il avec lui ?","Expliquez la différence entre `==` et `===` en JavaScript.","Qu'est-ce que le responsive design et comment l'implémenter ?","Qu'est-ce qu'une promesse (Promise) en JavaScript ?"],
      Intermédiaire: ["Expliquez le concept de Virtual DOM dans React et son avantage en termes de performance.","Comment gérez-vous l'état global dans une application React sans Redux ?","Quelle est la différence entre SSR et CSR ?","Expliquez le principe de 'code splitting' et son impact sur les performances.","Comment optimisez-vous les images et assets dans une application web moderne ?"],
      Senior:        ["Décrivez votre approche pour architecturer une application frontend à grande échelle.","Comment implémentez-vous une stratégie de micro-frontends dans une organisation multi-équipes ?","Expliquez les compromis entre différentes stratégies de mise en cache côté client.","Comment gérez-vous la dette technique dans un projet frontend legacy ?","Décrivez une situation où vous avez amélioré significativement les Core Web Vitals d'une app."],
      Lead:          ["Comment construisez-vous une culture d'ingénierie frontend dans une équipe en croissance ?","Quelles sont vos stratégies pour aligner les décisions techniques avec les objectifs business ?","Comment définissez-vous les standards de qualité de code pour une équipe distribuée ?","Décrivez votre approche pour évaluer et adopter de nouvelles technologies frontend.","Comment gérez-vous les désaccords techniques majeurs au sein de votre équipe ?"]
    },
    'Développeur Backend': {
      Débutant:      ["Expliquez la différence entre une API REST et une API SOAP.","Qu'est-ce qu'un ORM et quels sont ses avantages ?","Comment fonctionne l'authentification par token JWT ?","Quelle est la différence entre SQL et NoSQL ?","Expliquez le concept de middleware dans une application backend."],
      Intermédiaire: ["Comment concevez-vous un système de gestion des sessions évolutif ?","Expliquez les patterns CQRS et Event Sourcing.","Comment gérez-vous les transactions distribuées dans un système microservices ?","Quelle est votre approche pour la gestion des erreurs et le logging en production ?","Comment implémentez-vous un système de rate limiting robuste ?"],
      Senior:        ["Comment architecturez-vous un système capable de gérer 10 millions de requêtes par jour ?","Décrivez votre stratégie pour migrer un monolithe vers des microservices sans downtime.","Comment concevez-vous un système de cache distribué haute disponibilité ?","Expliquez vos décisions d'architecture pour un système de paiement sécurisé.","Comment gérez-vous la cohérence des données dans un système distribué ?"],
      Lead:          ["Comment définissez-vous les standards d'API pour une organisation avec plusieurs équipes ?","Quelle est votre approche pour la gestion de la capacité et la planification de la scalabilité ?","Comment créez-vous une culture DevOps dans une équipe backend traditionnelle ?","Décrivez votre processus de revue d'architecture pour les nouvelles fonctionnalités critiques.","Comment équilibrez-vous la vitesse de livraison avec la qualité et la sécurité du code ?"]
    },
    'Fullstack': {
      Débutant:      ["Comment gérez-vous la communication entre frontend et backend dans un projet fullstack ?","Quelle est votre approche pour choisir une base de données selon le projet ?","Expliquez le cycle de vie d'une requête HTTP de bout en bout.","Comment gérez-vous les variables d'environnement dans un projet fullstack ?","Quelle est la différence entre une session et un token JWT ?"],
      Intermédiaire: ["Comment structurez-vous un projet fullstack pour une équipe de 5 développeurs ?","Expliquez votre stratégie de déploiement continu pour une app fullstack.","Comment gérez-vous l'authentification et les autorisations dans une app moderne ?","Quelle est votre approche pour le testing dans un projet fullstack ?","Comment optimisez-vous les performances d'une app fullstack en production ?"],
      Senior:        ["Décrivez l'architecture d'une application fullstack scalable à 1 million d'utilisateurs.","Comment gérez-vous la synchronisation des données en temps réel entre frontend et backend ?","Quelle est votre stratégie pour la migration de base de données sans downtime ?","Comment concevez-vous un système de monitoring et d'alerting pour une app fullstack ?","Décrivez un incident de production que vous avez résolu et ce que vous en avez appris."],
      Lead:          ["Comment définissez-vous les standards d'architecture pour une équipe fullstack ?","Quelle est votre vision pour l'outillage et l'automatisation dans une équipe fullstack ?","Comment recrutez-vous et évaluez-vous des développeurs fullstack ?","Décrivez comment vous gérez la dette technique dans un projet fullstack legacy.","Comment créez-vous une roadmap technique alignée avec les objectifs produit ?"]
    },
    'DevOps / SRE': {
      Débutant:      ["Expliquez la différence entre CI et CD.","Qu'est-ce que Docker et pourquoi l'utiliser ?","Comment fonctionne un reverse proxy ?","Qu'est-ce que le concept d'Infrastructure as Code ?","Expliquez la différence entre un conteneur et une machine virtuelle."],
      Intermédiaire: ["Comment concevez-vous un pipeline CI/CD robuste ?","Expliquez votre approche pour la gestion des secrets en production.","Comment gérez-vous le scaling automatique dans Kubernetes ?","Quelle est votre stratégie de monitoring et d'alerting ?","Comment gérez-vous les rollbacks en cas d'incident ?"],
      Senior:        ["Décrivez l'architecture d'une infrastructure multi-cloud haute disponibilité.","Comment définissez-vous et atteignez-vous vos SLOs ?","Quelle est votre approche pour le disaster recovery planning ?","Comment gérez-vous la sécurité dans un environnement cloud-native ?","Décrivez comment vous avez réduit le MTTR (Mean Time To Recovery) dans votre organisation."],
      Lead:          ["Comment construisez-vous une culture DevOps dans une organisation traditionnelle ?","Quelle est votre vision pour la platform engineering dans une grande organisation ?","Comment gérez-vous les on-call rotations et la charge mentale de votre équipe SRE ?","Décrivez votre approche pour le capacity planning à long terme.","Comment mesurez-vous et améliorez-vous la vélocité de livraison de l'organisation ?"]
    }
  },
  data: {
    'Data Analyst': {
      Débutant:      ["Quelle est la différence entre la moyenne, la médiane et le mode ?","Comment détectez-vous et traitez-vous les valeurs aberrantes dans un dataset ?","Expliquez la différence entre données structurées et non structurées.","Qu'est-ce qu'une jointure SQL et quels sont les différents types ?","Comment présentez-vous des données complexes à un public non-technique ?"],
      Intermédiaire: ["Expliquez les tests A/B et comment vous en mesurez la significativité statistique.","Comment abordez-vous l'analyse de cohortes pour mesurer la rétention utilisateur ?","Quelle est votre approche pour construire un dashboard analytique actionnable ?","Comment gérez-vous les données manquantes dans votre analyse ?","Expliquez la régression linéaire et ses hypothèses clés."],
      Senior:        ["Comment concevez-vous un data warehouse pour une entreprise en croissance rapide ?","Décrivez votre approche pour construire un système de recommandation basé sur les données.","Comment mesurez-vous l'impact business de vos analyses et insights ?","Expliquez votre stratégie pour garantir la qualité des données à grande échelle.","Comment combinez-vous données quantitatives et qualitatives pour prendre de meilleures décisions ?"],
      Lead:          ["Comment construisez-vous une culture data-driven dans une organisation résistante ?","Quelle est votre vision pour une infrastructure analytique moderne et scalable ?","Comment définissez-vous les KPIs qui ont réellement de l'importance pour le business ?","Décrivez votre approche pour la gouvernance des données à l'échelle de l'entreprise.","Comment recrutez-vous et développez-vous des talents analytiques dans un marché compétitif ?"]
    },
    'Data Scientist': {
      Débutant:      ["Expliquez la différence entre apprentissage supervisé et non supervisé.","Qu'est-ce que l'overfitting et comment l'éviter ?","Quelle est la différence entre précision et rappel (recall) ?","Expliquez le concept de validation croisée (cross-validation).","Qu'est-ce qu'une forêt aléatoire (random forest) ?"],
      Intermédiaire: ["Comment choisissez-vous le bon algorithme de machine learning pour un problème donné ?","Expliquez le gradient boosting et ses avantages par rapport aux forêts aléatoires.","Comment gérez-vous le déséquilibre des classes dans un problème de classification ?","Quelle est votre approche pour le feature engineering ?","Comment déployez-vous un modèle de ML en production ?"],
      Senior:        ["Décrivez l'architecture d'un système de ML en production à grande échelle.","Comment gérez-vous la dérive des données (data drift) en production ?","Quelle est votre approche pour l'explicabilité des modèles (XAI) ?","Comment concevez-vous un système d'expérimentation ML robuste ?","Décrivez un modèle que vous avez construit qui a eu un impact business significatif."],
      Lead:          ["Comment créez-vous une roadmap ML alignée avec la stratégie business ?","Quelle est votre vision pour la MLOps dans une organisation data-mature ?","Comment évaluez-vous le ROI des projets de data science ?","Décrivez comment vous gérez les attentes des parties prenantes sur les projets ML.","Comment construisez-vous et gérez-vous une équipe de data scientists performante ?"]
    }
  },
  design: {
    'UX Designer': {
      Débutant:      ["Expliquez la différence entre UX et UI design.","Qu'est-ce que le design centré sur l'utilisateur ?","Comment menez-vous des entretiens utilisateurs ?","Qu'est-ce qu'un wireframe et pourquoi est-il important ?","Expliquez le concept de persona en UX design."],
      Intermédiaire: ["Comment mesurez-vous l'expérience utilisateur de manière quantitative ?","Décrivez votre processus de recherche utilisateur pour un nouveau produit.","Comment gérez-vous les compromis entre design idéal et contraintes techniques ?","Quelle est votre approche pour les tests d'utilisabilité ?","Comment intégrez-vous le design thinking dans votre processus de travail ?"],
      Senior:        ["Comment créez-vous un système de design scalable pour une grande organisation ?","Décrivez votre approche pour l'accessibilité dans le design produit.","Comment mesurez-vous l'impact business de vos décisions de design ?","Quelle est votre stratégie pour aligner design, produit et engineering ?","Décrivez un projet où vous avez transformé une expérience utilisateur complexe."],
      Lead:          ["Comment construisez-vous une culture design dans une organisation tech ?","Quelle est votre vision pour l'évolution du design avec l'IA générative ?","Comment gérez-vous et développez-vous une équipe de designers ?","Décrivez votre approche pour le design strategy à long terme.","Comment mesurez-vous et communiquez-vous la valeur du design aux dirigeants ?"]
    }
  },
  finance: {
    'Analyste financier': {
      Débutant:      ["Expliquez la différence entre un bilan et un compte de résultat.","Qu'est-ce que le cash flow et pourquoi est-il important ?","Comment calculez-vous le ROI d'un investissement ?","Expliquez le concept de valeur actuelle nette (VAN).","Quelle est la différence entre coût fixe et coût variable ?"],
      Intermédiaire: ["Comment construisez-vous un modèle financier de prévision ?","Expliquez votre approche pour l'analyse de la valorisation d'une entreprise.","Comment gérez-vous les risques financiers dans un portefeuille ?","Quelle est votre méthode pour l'analyse comparative sectorielle ?","Comment présentez-vous des analyses financières complexes à des non-financiers ?"],
      Senior:        ["Décrivez votre approche pour une due diligence financière complète.","Comment modélisez-vous les scénarios de stress test pour une entreprise ?","Quelle est votre stratégie pour optimiser la structure du capital d'une entreprise ?","Comment évaluez-vous les synergies dans une opération de M&A ?","Décrivez un modèle financier complexe que vous avez construit et son impact."],
      Lead:          ["Comment définissez-vous la stratégie financière à long terme d'une organisation ?","Quelle est votre approche pour la gestion de la relation avec les investisseurs ?","Comment construisez-vous et gérez-vous une équipe financière performante ?","Décrivez votre vision pour la transformation digitale de la fonction finance.","Comment alignez-vous la stratégie financière avec les objectifs business globaux ?"]
    }
  },
  marketing: {
    'Growth Hacker': {
      Débutant:      ["Qu'est-ce que le growth hacking et comment le différencier du marketing traditionnel ?","Expliquez le concept de funnel AARRR.","Comment mesurez-vous le taux de conversion d'une landing page ?","Qu'est-ce qu'un test A/B et comment l'implémentez-vous ?","Quelle est la différence entre CAC et LTV ?"],
      Intermédiaire: ["Décrivez une expérience de growth que vous avez conduite et ses résultats.","Comment identifiez-vous les canaux d'acquisition les plus rentables ?","Quelle est votre approche pour réduire le churn ?","Comment construisez-vous des boucles de croissance virales ?","Quelle est votre stratégie pour l'optimisation du funnel de conversion ?"],
      Senior:        ["Comment concevez-vous une stratégie de croissance durable pour une startup B2B ?","Décrivez votre approche pour passer de 0 à 1 million d'utilisateurs.","Comment gérez-vous la tension entre croissance rapide et qualité du produit ?","Quelle est votre stratégie pour l'internationalisation d'un produit ?","Comment mesurez-vous et améliorez-vous le product-market fit ?"],
      Lead:          ["Comment créez-vous une culture growth dans une organisation traditionnelle ?","Quelle est votre vision pour la croissance à long terme dans un marché saturé ?","Comment gérez-vous et développez-vous une équipe growth multidisciplinaire ?","Décrivez votre approche pour aligner growth, produit et business development.","Comment définissez-vous les métriques north star pour votre organisation ?"]
    }
  },
  product: {
    'Product Manager': {
      Débutant:      ["Quelle est la différence entre un Product Manager et un Project Manager ?","Comment priorisez-vous les fonctionnalités d'un produit ?","Qu'est-ce qu'une user story et comment la rédigez-vous ?","Comment mesurez-vous le succès d'une fonctionnalité après son lancement ?","Expliquez le concept de MVP (Minimum Viable Product)."],
      Intermédiaire: ["Comment gérez-vous les désaccords entre les équipes engineering et business ?","Décrivez votre processus de discovery produit.","Comment construisez-vous une roadmap produit convaincante ?","Quelle est votre approche pour la gestion des stakeholders ?","Comment utilisez-vous les données pour prendre des décisions produit ?"],
      Senior:        ["Comment définissez-vous la stratégie produit à long terme d'une entreprise ?","Décrivez comment vous avez pivoté un produit basé sur les retours du marché.","Comment gérez-vous un portefeuille de produits avec des équipes multiples ?","Quelle est votre approche pour lancer un produit sur un nouveau marché ?","Comment mesurez-vous et communiquez-vous l'impact business de votre produit ?"],
      Lead:          ["Comment créez-vous une organisation produit world-class ?","Quelle est votre vision pour l'intégration de l'IA dans votre stratégie produit ?","Comment recrutez-vous et développez-vous des PMs exceptionnels ?","Décrivez votre approche pour aligner la vision produit avec la stratégie corporate.","Comment gérez-vous la culture produit dans une organisation en hypercroissance ?"]
    }
  }
};

const genericQuestions = {
  Débutant:      ["Parlez-moi de vous et de ce qui vous a amené vers ce domaine.","Quels sont vos principaux points forts dans votre domaine ?","Décrivez un projet dont vous êtes particulièrement fier.","Comment continuez-vous à apprendre et à vous améliorer dans votre domaine ?","Où vous voyez-vous dans 2 ans ?"],
  Intermédiaire: ["Décrivez une situation difficile que vous avez dû gérer dans un projet.","Comment gérez-vous la pression et les délais serrés ?","Parlez d'un moment où vous avez dû apprendre rapidement une nouvelle technologie.","Comment collaborez-vous avec des équipes cross-fonctionnelles ?","Quelle est votre plus grande réalisation professionnelle ?"],
  Senior:        ["Comment influencez-vous la stratégie technique de votre organisation ?","Décrivez comment vous avez guidé une équipe à travers un changement technologique majeur.","Comment mesurez-vous votre impact et celui de votre équipe ?","Parlez d'une décision difficile que vous avez prise avec des informations incomplètes.","Comment construisez-vous et maintenez-vous la confiance au sein de votre équipe ?"],
  Lead:          ["Quelle est votre philosophie de leadership et comment l'appliquez-vous ?","Comment créez-vous un environnement psychologiquement sûr pour votre équipe ?","Décrivez comment vous avez transformé une équipe sous-performante.","Comment gérez-vous les conflits entre membres seniors de votre équipe ?","Quelle est votre vision pour votre domaine dans les 5 prochaines années ?"]
};

function getQuestions() {
  const { domain, role, level } = session;
  try {
    return questionsBank[domain][role][level] || genericQuestions[level] || genericQuestions['Débutant'];
  } catch {
    return genericQuestions[level] || genericQuestions['Débutant'];
  }
}

// Récupérer une question via API Groq (avec fallback)
async function fetchQuestion(index) {
  try {
    // Get previously asked questions to avoid repetition
    const askedQuestions = JSON.parse(sessionStorage.getItem('hireme_asked_questions') || '[]');
    
    // Add retry logic for question diversity
    let question = null;
    let retries = 0;
    const maxRetries = 2;
    
    while (!question && retries <= maxRetries) {
      const response = await fetch('/api/generate-question', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({
          domain: session.domain || 'Tech',
          role: session.role || 'Général',
          level: session.level || 'Débutant',
          index: index + retries,
          askedQuestions: askedQuestions,
          totalQuestions: TOTAL_Q
        })
      });

      const data = await response.json().catch(() => ({}));
      if (response.ok && data.success && data.question) {
        // Check if question is too similar to previous ones
        const questionLower = data.question.toLowerCase();
        const isDuplicate = askedQuestions.some(q => {
          const qLower = q.toLowerCase();
          const commonWords = questionLower.split(/\s+/).filter(word => 
            word.length > 4 && qLower.includes(word)
          ).length;
          return commonWords > 3;
        });
        
        if (!isDuplicate) {
          question = data.question;
        } else if (retries < maxRetries) {
          retries++;
          continue;
        } else {
          question = data.question;
        }
      }
      
      if (retries < maxRetries && !question) {
        retries++;
      } else {
        break;
      }
    }
    
    if (!question) {
      throw new Error('Impossible de générer une question diversifiée');
    }
    
    // Track this question to avoid future repetition
    askedQuestions.push(question);
    sessionStorage.setItem('hireme_asked_questions', JSON.stringify(askedQuestions));
    
    return question;
  } catch (err) {
    console.warn('API indisponible, utilisation des questions locales:', err.message);
    // Use local questions bank as fallback
    return getLocalQuestion(index);
  }
}

function getLocalQuestion(index) {
  // Retourner une question de la banque locale
  const domain = session.domain || 'Tech';
  const role = session.role || 'Développeur Frontend';
  const level = session.level || 'Débutant';
  const questions = questionsBank[domain.toLowerCase()]?.[role]?.[level] || questionsBank.tech['Développeur Frontend']['Débutant'];
  return questions[index % questions.length] || 'Décrivez votre approche pour résoudre ce problème.';
}

// Évaluer une réponse via API Groq (avec fallback)
async function evaluateAnswer(question, answer) {
  const trimmedAnswer = (answer || '').trim();
  try {
    const response = await fetch('/api/evaluate-answer', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({
        question: question,
        answer: trimmedAnswer,
        level: session.level || 'Débutant',
        domain: session.domain || 'Tech'
      })
    });

    const data = await response.json().catch(() => ({}));
    if (!response.ok || !data.success) {
      throw new Error(data.error || 'Evaluation IA indisponible');
    }

    const score = Math.max(0, Math.min(10, data.score || 0));
    const feedback = data.feedback || 'Evaluation IA recue.';
    return { score, feedback };
  } catch (err) {
    console.warn('API evaluation indisponible, utilisation scoring local:', err.message);
    // Fallback: Score based on answer length and content
    const score = Math.min(10, Math.max(1, Math.floor(trimmedAnswer.length / 20)));
    const feedback = trimmedAnswer.length > 50 ? 'Réponse détaillée et constructive.' : 'Réponse brève - développez davantage.';
    return { score, feedback };
  }
}

let questions = [];

function init() {
  document.getElementById('displayName').textContent  = session.name  || 'Candidat';
  document.getElementById('displayRole').textContent  = session.role  || '—';
  document.getElementById('displayLevel').textContent = session.level || '—';

  // Initialize question tracking for this session
  sessionStorage.setItem('hireme_asked_questions', JSON.stringify([]));

  const dots = document.getElementById('progressDots');
  for (let i = 0; i < TOTAL_Q; i++) {
    const d = document.createElement('div');
    d.className = 'dot' + (i === 0 ? ' current' : '');
    dots.appendChild(d);
  }

  // Questions are generated by Groq in real time.
  questions = new Array(TOTAL_Q).fill('');
  loadQuestion(0);
}

async function loadQuestion(index) {
  currentQ = index;
  
  let q = questions[index];
  if (!q) {
    try {
      q = await fetchQuestion(index);
      questions[index] = q;
    } catch (err) {
      clearInterval(timerInterval);
      document.getElementById('questionText').textContent = 'Generation IA indisponible. Verifiez que le backend est lance avec GROQ_API_KEY.';
      document.getElementById('submitBtn').disabled = true;
      document.getElementById('answerInput').disabled = true;
      alert(err.message || 'Generation IA indisponible.');
      return;
    }
  }

  document.getElementById('submitBtn').disabled = false;
  document.getElementById('answerInput').disabled = false;

  document.getElementById('qNumber').textContent       = `Question ${String(index+1).padStart(2,'0')}`;
  document.getElementById('progressCount').textContent = `Question ${index+1} / ${TOTAL_Q}`;
  document.getElementById('progressFill').style.width  = `${(index / TOTAL_Q) * 100}%`;

  document.querySelectorAll('.dot').forEach((d, i) => {
    d.className = 'dot';
    if (i < index)       d.classList.add('done');
    else if (i === index) d.classList.add('current');
  });

  document.getElementById('qMeta').innerHTML = `
    <div class="q-tag">${session.role   || 'Général'}</div>
    <div class="q-tag">${session.level  || 'Débutant'}</div>
    <div class="q-tag">${session.domain || 'Tech'}</div>
  `;

  const el = document.getElementById('questionText');
  el.textContent = q;
  el.classList.remove('typing');

  document.getElementById('answerInput').value = '';
  document.getElementById('charCount').textContent = '0 / 2000 caractères';
  document.getElementById('charCount').classList.remove('warn');

  startTimer();
}

function startTimer() {
  clearInterval(timerInterval);
  timeLeft = 60;
  const ring = document.getElementById('timerRing');
  const text = document.getElementById('timerText');
  const circ = 175.9;

  ring.classList.remove('danger');
  text.classList.remove('danger');

  timerInterval = setInterval(() => {
    timeLeft--;
    text.textContent = timeLeft;
    ring.style.strokeDashoffset = circ * (1 - timeLeft / 60);
    if (timeLeft <= 15) { ring.classList.add('danger'); text.classList.add('danger'); }
    if (timeLeft <= 0)  { clearInterval(timerInterval); submitAnswer(true); }
  }, 1000);
}

function updateCharCount() {
  const len = document.getElementById('answerInput').value.length;
  const el  = document.getElementById('charCount');
  el.textContent = `${len} / 2000 caractères`;
  el.classList.toggle('warn', len > 1800);
}

function submitAnswer(auto = false) {
  clearInterval(timerInterval);
  const answer = document.getElementById('answerInput').value.trim();
  const question = questions[currentQ];
  
  // Évaluer la réponse (async)
  evaluateAnswer(question, answer).then(async ({ score, feedback }) => {
    answers.push(answer || '[Aucune réponse]');
    scores.push(score);
    if (session.session_id) {
      try {
        await fetch(`${API_URL}/api/answer`, {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify({
            session_id: session.session_id,
            question,
            answer: answer || '[Aucune réponse]',
            score,
            feedback
          })
        });
      } catch (err) {
        console.warn('Answer save failed:', err);
      }
    }
    showFeedback(score, feedback);
  }).catch(() => {
    const score = 0;
    const feedback = "Evaluation IA indisponible (Groq). Verifiez GROQ_API_KEY, le backend et la connexion reseau, puis reessayez.";
    answers.push(answer || '[Aucune réponse]');
    scores.push(score);
    if (session.session_id) {
      fetch(`${API_URL}/api/answer`, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({
          session_id: session.session_id,
          question,
          answer: answer || '[Aucune réponse]',
          score,
          feedback
        })
      }).catch(err => console.warn('Answer save failed:', err));
    }
    showFeedback(score, feedback);
  });
}

function skipQuestion() {
  clearInterval(timerInterval);
  const question = questions[currentQ];
  const feedback = "Vous avez passé cette question. Essayez de répondre à toutes les questions pour un meilleur score final.";
  answers.push('[Passé]');
  scores.push(0);
  if (session.session_id) {
    fetch(`${API_URL}/api/answer`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({
        session_id: session.session_id,
        question,
        answer: '[Passé]',
        score: 0,
        feedback
      })
    }).catch(err => console.warn('Answer save failed:', err));
  }
  showFeedback(0, feedback);
}

function simulateScore(answer) {
  if (isLowEffortAnswer(answer)) return 1;
  if (!answer || answer.length < 20)  return Math.floor(Math.random() * 3) + 1;
  if (answer.length < 80)             return Math.floor(Math.random() * 3) + 4;
  if (answer.length < 200)            return Math.floor(Math.random() * 2) + 6;
  return Math.floor(Math.random() * 2) + 8;
}

function isLowEffortAnswer(answer) {
  const text = (answer || '').toLowerCase().trim();
  if (!text) return true;

  // Very short or non-informative content.
  if (text.length <= 3) return true;

  const lowEffortPatterns = [
    'je ne sais pas',
    'je sais pas',
    'jsais pas',
    'aucune idee',
    'pas d idee',
    'pas sur',
    'idk',
    "i don't know",
    'no idea',
    'je ne comprends pas',
    'je ne comprends rien'
  ];

  return lowEffortPatterns.some((pattern) => text.includes(pattern));
}

function generateFeedback(score) {
  if (score >= 9) return "Excellente réponse ! Vous avez démontré une maîtrise approfondie du sujet avec des exemples concrets et une structure claire.";
  if (score >= 7) return "Bonne réponse. Vous couvrez les points essentiels. Ajoutez des exemples spécifiques de votre expérience pour aller plus loin.";
  if (score >= 5) return "Réponse acceptable. Vous abordez le sujet mais manquez de profondeur. Développez votre raisonnement et illustrez avec des cas concrets.";
  if (score >= 3) return "Réponse insuffisante. Les points clés ne sont pas couverts. Préparez des exemples STAR (Situation, Tâche, Action, Résultat).";
  return "Cette question mérite une meilleure préparation. Revoyez les fondamentaux et pratiquez avec des exemples de votre expérience.";
}

function showFeedback(score, feedback) {
  const overlay = document.getElementById('feedbackOverlay');
  document.getElementById('scoreVal').textContent   = score;
  document.getElementById('feedbackText').textContent = feedback;

  const isLast = currentQ >= TOTAL_Q - 1;
  document.getElementById('nextBtn').textContent = isLast ? 'Voir le rapport final →' : 'Question suivante →';

  const titles = { 9:'Excellent ! 🎯', 7:'Très bien !', 5:'Correct', 3:'À améliorer', 0:'Question passée' };
  const t = Object.keys(titles).sort((a,b)=>b-a).find(k => score >= k);
  document.getElementById('feedbackTitle').textContent = titles[t] || 'Réponse reçue';

  overlay.classList.add('show');
  setTimeout(() => {
    document.getElementById('scoreFill').style.strokeDashoffset = 270 - (270 * score / 10);
  }, 100);
}

function nextQuestion() {
  document.getElementById('feedbackOverlay').classList.remove('show');
  document.getElementById('scoreFill').style.strokeDashoffset = 270;
  const next = currentQ + 1;
  if (next >= TOTAL_Q) finishInterview();
  else setTimeout(() => loadQuestion(next), 300);
}

function finishInterview() {
  const avg = (scores.reduce((a,b) => a+b, 0) / scores.length).toFixed(1);
  const result = { ...session, scores, answers, questions, average: avg, date: new Date().toISOString() };
  localStorage.setItem('hireme_result', JSON.stringify(result));

  const history = JSON.parse(localStorage.getItem('hireme_history') || '[]');
  history.unshift(result);
  localStorage.setItem('hireme_history', JSON.stringify(history.slice(0, 20)));

  const badge = parseFloat(avg) >= 8 ? 'Excellent Candidate'
    : parseFloat(avg) >= 6 ? 'Good Potential'
    : parseFloat(avg) >= 5 ? 'Rising Profile'
    : 'Needs Practice';

  if (session.session_id) {
    fetch(`${API_URL}/api/session/score`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({
        session_id: session.session_id,
        total_score: parseFloat(avg),
        badge
      })
    }).catch(err => console.warn('Score sync failed:', err)).finally(() => {
      window.location.href = 'result.html';
    });
    return;
  }

  window.location.href = 'result.html';
}

init();