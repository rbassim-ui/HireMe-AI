async function handler(req, res) {
  res.setHeader('Access-Control-Allow-Origin', '*');
  res.setHeader('Access-Control-Allow-Methods', 'POST, OPTIONS');
  res.setHeader('Access-Control-Allow-Headers', 'Content-Type');

  if (req.method === 'OPTIONS') {
    return res.status(200).end();
  }

  if (req.method !== 'POST') {
    return res.status(405).json({ success: false, error: 'Method not allowed' });
  }

  try {
    const { question, answer } = req.body;
    const apiKey = process.env.GROQ_API_KEY || process.env.GROK_API_KEY;

    if (!apiKey) {
      return res.status(500).json({ success: false, error: 'API key not configured (expected GROQ_API_KEY or GROK_API_KEY)' });
    }

    if (!question || !answer) {
      return res.status(400).json({ success: false, error: 'Missing fields' });
    }

    const response = await fetch('https://api.groq.com/openai/v1/chat/completions', {
      method: 'POST',
      headers: {
        'Authorization': `Bearer ${apiKey}`,
        'Content-Type': 'application/json'
      },
      body: JSON.stringify({
        model: 'llama-3.3-70b-versatile',
        messages: [{ role: 'user', content: `Evaluate this interview answer on a 1-10 scale.
Question: ${question}
Answer: ${answer}

Return ONLY valid JSON (no markdown, no extra text) with this exact schema:
{"score": number, "feedback": string, "strengths": string[], "improvements": string[]}` }],
        max_tokens: 300
      })
    });

    if (!response.ok) throw new Error('Groq API error');

    const data = await response.json();
    let content = data.choices?.[0]?.message?.content?.trim() || '';

    // Remove common markdown wrappers before parsing.
    if (content.startsWith('```')) {
      content = content.replace(/^```[a-zA-Z]*\n?/, '').replace(/```$/, '').trim();
    }

    let evaluation = null;
    try {
      const jsonMatch = content.match(/\{[\s\S]*\}/);
      evaluation = JSON.parse((jsonMatch && jsonMatch[0]) || content);
    } catch (_) {
      const scoreMatch = content.match(/(?:score|rating|note)\D{0,12}(10|[0-9](?:\.[0-9])?)/i);
      const fallbackScore = scoreMatch ? Number(scoreMatch[1]) : 6;

      evaluation = {
        score: fallbackScore,
        feedback: content.slice(0, 600) || 'Good response overall.',
        strengths: ['Clear structure'],
        improvements: ['Add more specific technical details']
      };
    }

    return res.json({
      success: true,
      score: Math.max(1, Math.min(10, evaluation.score || 6)),
      feedback: evaluation.feedback || 'Good response',
      strengths: evaluation.strengths || ['Clear'],
      improvements: evaluation.improvements || ['Expand']
    });
  } catch (error) {
    return res.status(500).json({ success: false, error: error.message });
  }
}

module.exports = handler;
