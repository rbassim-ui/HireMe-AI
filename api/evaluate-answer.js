/**
 * Vercel Serverless Function: Evaluate Interview Answer
 * Calls Groq API to evaluate candidate answers
 * 
 * Request body:
 * {
 *   "question": "Explain the difference between...",
 *   "answer": "The user's answer to the question",
 *   "level": "intermediate",
 *   "domain": "TEC"
 * }
 * 
 * Response:
 * {
 *   "success": true,
 *   "score": 7.5,
 *   "feedback": "Good explanation of...",
 *   "strengths": ["Clear explanation", "Good examples"],
 *   "improvements": ["Could mention..."]
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
      max_tokens: 800,
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
    const { question, answer, level, domain } = req.body;

    if (!question || !answer) {
      res.status(400).json({
        success: false,
        error: 'Missing required fields: question and answer'
      });
      return;
    }

    const prompt = `You are an expert technical interviewer evaluating a candidate's answer.

Question: ${question}

Candidate's Answer: ${answer}

Level: ${level || 'intermediate'}
Domain: ${domain || 'Technical'}

Evaluate this answer and provide:
1. A score from 1-10 (10 being excellent)
2. Brief feedback (2-3 sentences)
3. 2-3 key strengths
4. 2-3 areas for improvement

Format your response as JSON with this exact structure (no markdown):
{
  "score": 7,
  "feedback": "Your feedback here",
  "strengths": ["strength1", "strength2"],
  "improvements": ["improvement1", "improvement2"]
}`;

    const evaluation = await makeGroqRequest(prompt);

    // Parse the JSON response
    let parsed;
    try {
      // Try to extract JSON if it's wrapped in markdown code blocks
      const jsonMatch = evaluation.match(/\{[\s\S]*\}/);
      if (jsonMatch) {
        parsed = JSON.parse(jsonMatch[0]);
      } else {
        parsed = JSON.parse(evaluation);
      }
    } catch (e) {
      // Fallback parsing if JSON parsing fails
      console.warn('Failed to parse structured response, using fallback');
      parsed = {
        score: 6,
        feedback: evaluation.substring(0, 200),
        strengths: ['Clear response'],
        improvements: ['More detail recommended']
      };
    }

    // Validate score is between 1-10
    if (typeof parsed.score === 'number') {
      parsed.score = Math.max(1, Math.min(10, parsed.score));
    } else {
      parsed.score = 6;
    }

    res.status(200).json({
      success: true,
      ...parsed
    });
  } catch (error) {
    console.error('Error in evaluate-answer:', error);
    res.status(500).json({
      success: false,
      error: error.message || 'Failed to evaluate answer'
    });
  }
}

module.exports = handler;
