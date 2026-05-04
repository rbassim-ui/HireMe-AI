/**
 * Vercel Serverless Function: Generate Interview Question
 * Calls Groq API to generate dynamic interview questions
 * 
 * Request body:
 * {
 *   "domain": "TEC",
 *   "role": "Backend Developer",
 *   "level": "intermediate"
 * }
 * 
 * Response:
 * {
 *   "success": true,
 *   "question": "Explain the difference between...",
 *   "domain": "TEC",
 *   "role": "Backend Developer",
 *   "level": "intermediate"
 * }
 */

const https = require('https');

function setHeaders(res) {
  res.setHeader('Content-Type', 'application/json');
  res.setHeader('Access-Control-Allow-Origin', '*');
  res.setHeader('Access-Control-Allow-Methods', 'GET, POST, OPTIONS');
  res.setHeader('Access-Control-Allow-Headers', 'Content-Type');
}

function makeGroqRequest(prompt) {
  return new Promise((resolve, reject) => {
    const apiKey = process.env.GROQ_API_KEY;
    if (!apiKey) {
      reject(new Error('GROQ_API_KEY environment variable not set'));
      return;
    }

    const requestBody = JSON.stringify({
      model: 'llama-3.3-70b-versatile',
      messages: [
        {
          role: 'user',
          content: prompt
        }
      ],
      max_tokens: 500,
      temperature: 0.7
    });

    const options = {
      hostname: 'api.groq.com',
      path: '/openai/v1/chat/completions',
      method: 'POST',
      headers: {
        'Authorization': `Bearer ${apiKey}`,
        'Content-Type': 'application/json',
        'Content-Length': Buffer.byteLength(requestBody)
      }
    };

    const req = https.request(options, (res) => {
      let data = '';

      res.on('data', (chunk) => {
        data += chunk;
      });

      res.on('end', () => {
        try {
          const parsed = JSON.parse(data);
          if (parsed.choices && parsed.choices[0] && parsed.choices[0].message) {
            resolve(parsed.choices[0].message.content);
          } else if (parsed.error) {
            reject(new Error(`Groq API error: ${parsed.error.message}`));
          } else {
            reject(new Error('Unexpected Groq API response format'));
          }
        } catch (e) {
          reject(new Error(`Failed to parse Groq response: ${e.message}`));
        }
      });
    });

    req.on('error', (error) => {
      reject(new Error(`Groq API request failed: ${error.message}`));
    });

    req.write(requestBody);
    req.end();
  });
}

async function handler(req, res) {
  setHeaders(res);

  // Handle CORS preflight
  if (req.method === 'OPTIONS') {
    res.status(200).end();
    return;
  }

  if (req.method !== 'POST') {
    res.status(405).json({ success: false, error: 'Method not allowed' });
    return;
  }

  try {
    const { domain, role, level } = req.body;

    if (!domain || !role || !level) {
      res.status(400).json({
        success: false,
        error: 'Missing required fields: domain, role, level'
      });
      return;
    }

    const prompt = `You are an expert technical interviewer. Generate a single, clear, professional interview question for a ${level} level ${role} position in the ${domain} domain. 

The question should:
- Be appropriate for ${level} level candidates
- Be relevant to ${domain}
- Be concise and clear
- Test practical skills or knowledge
- Require a detailed answer (not just yes/no)

Return ONLY the question text, nothing else.`;

    const question = await makeGroqRequest(prompt);

    res.status(200).json({
      success: true,
      question: question.trim(),
      domain,
      role,
      level
    });
  } catch (error) {
    console.error('Error in generate-question:', error);
    res.status(500).json({
      success: false,
      error: error.message || 'Failed to generate question'
    });
  }
}

module.exports = handler;
