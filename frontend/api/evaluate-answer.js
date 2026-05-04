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
    const apiKey = process.env.GROQ_API_KEY;

    if (!apiKey) {
      return res.status(500).json({ success: false, error: 'API key not configured' });
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
        messages: [{ role: 'user', content: `Evaluate this interview answer on 1-10 scale. Question: ${question} Answer: ${answer} Reply with JSON: {"score": X, "feedback": "...", "strengths": [], "improvements": []}` }],
        max_tokens: 300
      })
    });

    if (!response.ok) throw new Error('Groq API error');

    const data = await response.json();
    let content = data.choices?.[0]?.message?.content?.trim();
    const jsonMatch = content?.match(/\{[\s\S]*\}/);
    const evaluation = JSON.parse(jsonMatch?.[0] || content);

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
