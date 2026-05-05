# HireMe Backend API — Node Express Edition

Express.js backend for HireMe interview platform. Provides:
- Session management (SQLite)
- Groq AI question generation & answer evaluation
- REST endpoints compatible with C backend

## Deployment on Railway

### 1. Connect GitHub
- [Railway Dashboard](https://railway.app/dashboard) → New Project
- Select "Deploy from GitHub" → Choose `HireMe-AI` repo

### 2. Configure Service
- **Root Directory**: `backend-api`
- **Build Command**: `npm install`
- **Start Command**: `npm start`
- **Port**: `3000` (auto-detected)

### 3. Environment Variables
In Railway service settings, add:
```
GROQ_API_KEY=sk-proj-your-actual-key-here
```

### 4. Deploy
- Railway auto-detects Node.js and builds
- Service gets a public URL like: `https://your-service.up.railway.app`

## Local Testing

```bash
npm install
npm start
```

Server listens on `http://localhost:3000`

Test endpoints:
```bash
# Generate question
curl -X POST http://localhost:3000/api/generate-question \
  -H "Content-Type: application/json" \
  -d '{"domain":"Tech","role":"Backend Developer","level":"Debutant"}'

# Evaluate answer
curl -X POST http://localhost:3000/api/evaluate-answer \
  -H "Content-Type: application/json" \
  -d '{"question":"What is REST?","answer":"REST is stateless and uses HTTP.","level":"Debutant","domain":"Tech"}'
```

## API Endpoints

| Method | Route | Description |
|--------|-------|-------------|
| POST | `/api/generate-question` | Generate AI interview question |
| POST | `/api/evaluate-answer` | Evaluate answer with Groq |
| POST | `/api/session` | Create interview session |
| GET | `/api/session/:id` | Get session details |
| GET | `/api/sessions` | List all sessions |
| GET | `/api/stats` | Global statistics |

## Environment

- **Node.js**: 20.x (required for better-sqlite3)
- **Database**: SQLite (auto-created at `output/hireme.db`)

## Troubleshooting

### "API key not configured"
- Check Railway environment variables
- Ensure `GROQ_API_KEY` is set (not empty)
- Restart service after adding/changing var

### "Cannot find module 'better-sqlite3'"
- Ensure Node 20.x is selected (not Node 24+)
- Railway uses prebuilt binaries for Node 20

### Port already in use
- Railway assigns PORT via env var automatically
- Check logs in Railway dashboard
