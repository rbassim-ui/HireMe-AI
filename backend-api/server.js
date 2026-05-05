const express = require('express');
const cors = require('cors');
const path = require('path');
const fs = require('fs');
const Database = require('better-sqlite3');
const fetch = require('node-fetch');

const app = express();
app.use(cors());
app.use(express.json());

const DATA_DIR = path.join(__dirname, 'output');
if (!fs.existsSync(DATA_DIR)) fs.mkdirSync(DATA_DIR, { recursive: true });
const DB_PATH = path.join(DATA_DIR, 'hireme.db');

const db = new Database(DB_PATH);
db.pragma('foreign_keys = ON');

function createTables() {
  const sql = `
  CREATE TABLE IF NOT EXISTS users (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    name TEXT NOT NULL,
    created_at TEXT DEFAULT CURRENT_TIMESTAMP
  );

  CREATE TABLE IF NOT EXISTS sessions (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    user_id INTEGER,
    domain TEXT,
    role TEXT,
    level TEXT,
    total_score REAL DEFAULT 0,
    badge TEXT DEFAULT 'Pending',
    created_at TEXT DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY(user_id) REFERENCES users(id)
  );

  CREATE TABLE IF NOT EXISTS answers (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    session_id INTEGER,
    question TEXT,
    answer TEXT,
    score INTEGER DEFAULT 0,
    feedback TEXT,
    created_at TEXT DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY(session_id) REFERENCES sessions(id)
  );

  CREATE TABLE IF NOT EXISTS scores (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    session_id INTEGER,
    average_score REAL,
    total_questions INTEGER,
    created_at TEXT DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY(session_id) REFERENCES sessions(id)
  );
  `;
  db.exec(sql);
}

createTables();

function getGroqKey() {
  return process.env.GROQ_API_KEY || process.env.GROK_API_KEY || '';
}

function extractJsonObject(text) {
  if (!text) return null;
  let content = String(text).trim();
  if (content.startsWith('```')) {
    content = content.replace(/^```[a-zA-Z]*\n?/, '').replace(/```$/, '').trim();
  }
  const match = content.match(/\{[\s\S]*\}/);
  const candidate = (match && match[0]) || content;
  try {
    return JSON.parse(candidate);
  } catch (_) {
    return null;
  }
}

// POST /api/generate-question
app.post('/api/generate-question', async (req, res) => {
  try {
    const { domain, role, level } = req.body || {};
    const apiKey = getGroqKey();

    if (!apiKey) {
      return res.status(500).json({ success: false, error: 'API key not configured (expected GROQ_API_KEY or GROK_API_KEY)' });
    }

    if (!domain || !role || !level) {
      return res.status(400).json({ success: false, error: 'Missing fields' });
    }

    // Build prompt with diversity instruction and previous questions context
    const askedQuestions = req.body.askedQuestions || [];
    const index = req.body.index || 0;
    const totalQuestions = req.body.totalQuestions || 5;
    
    const topics = [
      'API design and REST principles',
      'Database design and optimization',
      'System architecture and scalability',
      'Code quality and best practices',
      'Testing strategies and debugging',
      'Performance optimization',
      'Security and authentication',
      'DevOps and deployment',
      'Problem-solving and algorithms',
      'Communication and leadership',
      'Version control and git workflows',
      'Error handling and logging',
      'Caching strategies',
      'Concurrency and async programming',
      'Monitoring and observability',
      'Design patterns',
      'Data structures',
      'API versioning',
      'Load balancing',
      'Disaster recovery'
    ];
    
    let selectedTopic = topics[index % topics.length];
    if (askedQuestions.length > 0) {
      const prevText = askedQuestions.join(' ').toLowerCase();
      let attempts = 0;
      while (prevText.includes(selectedTopic.toLowerCase()) && attempts < 5) {
        selectedTopic = topics[(index + attempts + 1) % topics.length];
        attempts++;
      }
    }
    
    const previousQuestionsList = askedQuestions.length > 0
      ? askedQuestions.map((q, i) => `Q${i+1}: ${q.substring(0, 80)}`).join('\n')
      : '';
    
    const promptText = `CRITICAL: Generate a UNIQUE interview question that is COMPLETELY DIFFERENT from previous ones.\n\nRole: ${level} level ${role} in ${domain}\nFocus Topic: ${selectedTopic}\n\n${previousQuestionsList ? `PREVIOUS QUESTIONS (ABSOLUTELY DO NOT REPEAT THESE):\n${previousQuestionsList}\n\nYou MUST NOT generate any question about:\n- GET/POST/HTTP methods (already asked)\n- Monolithic vs Microservices (already asked)\n- MVC patterns (already asked)\n- REST vs GraphQL (already asked)\n` : ''}INSTRUCTIONS:\n- Generate ONE question about ${selectedTopic} that is DIFFERENT from all above\n- Use completely different keywords and phrasing\n- Ask a new angle or scenario\n- Make it specific and practical\n- Return ONLY the question text (no explanations, no commentary)\n- If you recognize a topic above, pick a completely unrelated topic from: database indexing, caching strategies, transaction handling, concurrency control, load balancing, monitoring, logging, error handling, rate limiting\n`;
    
    const groqResponse = await fetch('https://api.groq.com/openai/v1/chat/completions', {
      method: 'POST',
      headers: {
        Authorization: `Bearer ${apiKey}`,
        'Content-Type': 'application/json'
      },
      body: JSON.stringify({
        model: 'llama-3.3-70b-versatile',
        messages: [
          {
            role: 'user',
            content: promptText
          }
        ],
        max_tokens: 200,
        temperature: 1.0
      })
    });

    if (!groqResponse.ok) {
      const errText = await groqResponse.text();
      throw new Error(`Groq API error: ${groqResponse.status} ${errText}`);
    }

    const data = await groqResponse.json();
    const question = (data.choices && data.choices[0] && data.choices[0].message && data.choices[0].message.content || '').trim();

    if (!question) {
      return res.status(500).json({ success: false, error: 'Empty question from Groq' });
    }

    return res.json({ success: true, question, domain, role, level });
  } catch (err) {
    console.error('POST /api/generate-question error', err);
    return res.status(500).json({ success: false, error: err.message });
  }
});

// POST /api/evaluate-answer
app.post('/api/evaluate-answer', async (req, res) => {
  try {
    const { question, answer } = req.body || {};
    const apiKey = getGroqKey();

    if (!apiKey) {
      return res.status(500).json({ success: false, error: 'API key not configured (expected GROQ_API_KEY or GROK_API_KEY)' });
    }

    if (!question || !answer) {
      return res.status(400).json({ success: false, error: 'Missing fields' });
    }

    const groqResponse = await fetch('https://api.groq.com/openai/v1/chat/completions', {
      method: 'POST',
      headers: {
        Authorization: `Bearer ${apiKey}`,
        'Content-Type': 'application/json'
      },
      body: JSON.stringify({
        model: 'llama-3.3-70b-versatile',
        messages: [
          {
            role: 'user',
            content: `Evaluate this interview answer on a 1-10 scale.\nQuestion: ${question}\nAnswer: ${answer}\n\nReturn ONLY valid JSON with this exact schema:\n{"score": number, "feedback": string, "strengths": string[], "improvements": string[]}`
          }
        ],
        max_tokens: 300
      })
    });

    if (!groqResponse.ok) {
      const errText = await groqResponse.text();
      throw new Error(`Groq API error: ${groqResponse.status} ${errText}`);
    }

    const data = await groqResponse.json();
    const content = (data.choices && data.choices[0] && data.choices[0].message && data.choices[0].message.content) || '';

    let evaluation = extractJsonObject(content);
    if (!evaluation) {
      const scoreMatch = String(content).match(/(?:score|rating|note)\D{0,12}(10|[0-9](?:\.[0-9])?)/i);
      evaluation = {
        score: scoreMatch ? Number(scoreMatch[1]) : 6,
        feedback: String(content).slice(0, 600) || 'Good response overall.',
        strengths: ['Clear structure'],
        improvements: ['Add more specific technical details']
      };
    }

    return res.json({
      success: true,
      score: Math.max(1, Math.min(10, Number(evaluation.score) || 6)),
      feedback: evaluation.feedback || 'Good response',
      strengths: Array.isArray(evaluation.strengths) ? evaluation.strengths : ['Clear'],
      improvements: Array.isArray(evaluation.improvements) ? evaluation.improvements : ['Expand']
    });
  } catch (err) {
    console.error('POST /api/evaluate-answer error', err);
    return res.status(500).json({ success: false, error: err.message });
  }
});

// GET /api/session/:id
app.get('/api/session/:id', (req, res) => {
  const id = Number(req.params.id) || 0;
  if (id <= 0) return res.status(400).json({});

  const sql = `SELECT s.user_id, s.domain, s.role, s.level, s.total_score, s.badge, u.name
    FROM sessions s JOIN users u ON s.user_id = u.id WHERE s.id = ?`;
  const row = db.prepare(sql).get(id);
  if (!row) return res.json({});
  res.json({
    session_id: id,
    user_id: row.user_id,
    name: row.name || '',
    domain: row.domain || '',
    role: row.role || '',
    level: row.level || '',
    total_score: Number(row.total_score || 0),
    badge: row.badge || ''
  });
});

// POST /api/session
app.post('/api/session', (req, res) => {
  const { name, domain, role, level, startedAt, user_id } = req.body || {};
  if (!name || !domain || !role) return res.status(400).json({ success: false, message: 'Missing required fields' });

  let uid = Number(user_id) || 0;
  try {
    if (uid <= 0) {
      const insertUser = db.prepare('INSERT INTO users (name, created_at) VALUES (?, CURRENT_TIMESTAMP)');
      const info = insertUser.run(name);
      uid = info.lastInsertRowid || info.lastInsertId || info.changes && info.changes > 0 ? info.lastInsertRowid : uid;
    }

    const insertSession = db.prepare('INSERT INTO sessions (user_id, domain, role, level) VALUES (?, ?, ?, ?)');
    const info2 = insertSession.run(uid, domain, role, level || '');
    const sessionId = info2.lastInsertRowid || info2.lastInsertId || 0;

    res.json({ success: true, session_id: sessionId, user_id: uid, message: 'Session created' });
  } catch (err) {
    console.error('POST /api/session error', err);
    res.status(500).json({ success: false, message: 'Unable to create session' });
  }
});

// GET /api/sessions
app.get('/api/sessions', (req, res) => {
  const sql = `SELECT s.id, u.name, s.domain, s.role, s.level, s.total_score, s.badge, s.created_at
    FROM sessions s JOIN users u ON s.user_id = u.id
    ORDER BY s.created_at DESC LIMIT 20`;
  const rows = db.prepare(sql).all();
  const items = rows.map(r => ({
    id: r.id,
    name: r.name || '',
    domain: r.domain || '',
    role: r.role || '',
    level: r.level || '',
    score: Number(r.total_score || 0),
    badge: r.badge || '',
    date: r.created_at || ''
  }));
  res.json(items);
});

// GET /api/stats
app.get('/api/stats', (req, res) => {
  try {
    const globalSql = `SELECT COUNT(*) as total_sessions, COALESCE(AVG(total_score),0) as avg_score, COALESCE(MAX(total_score),0) as best_score
      FROM sessions WHERE total_score > 0`;
    const g = db.prepare(globalSql).get();

    const bestDomainSql = `SELECT domain, AVG(total_score) as avg_score, COUNT(*) as sessions
      FROM sessions WHERE total_score > 0 AND domain IS NOT NULL AND domain != ''
      GROUP BY domain ORDER BY avg_score DESC, sessions DESC LIMIT 1`;
    const best = db.prepare(bestDomainSql).get() || { domain: '', avg_score: 0, sessions: 0 };

    const domainsSql = `SELECT domain, AVG(total_score) as avg_score, COUNT(*) as sessions, MAX(total_score) as best_score
      FROM sessions WHERE total_score > 0 AND domain IS NOT NULL AND domain != ''
      GROUP BY domain ORDER BY avg_score DESC, sessions DESC LIMIT 5`;
    const domains = db.prepare(domainsSql).all().map(r => ({ domain: r.domain, avg_score: Number(r.avg_score || 0), sessions: r.sessions, best_score: Number(r.best_score || 0) }));

    res.json({
      total_sessions: Number(g.total_sessions || 0),
      avg_score: Number((g.avg_score || 0).toFixed(1)),
      best_score: Number((g.best_score || 0).toFixed(1)),
      best_domain: best.domain || '',
      best_domain_avg: Number((best.avg_score || 0).toFixed(1)),
      best_domain_sessions: best.sessions || 0,
      domains
    });
  } catch (err) {
    console.error('GET /api/stats error', err);
    res.status(500).json({});
  }
});

const PORT = process.env.PORT || 3000;
app.listen(PORT, () => console.log(`HireMe API listening on port ${PORT}`));
