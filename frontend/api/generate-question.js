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
    const { domain, role, level } = req.body;
    const apiKey = process.env.GROQ_API_KEY;

    if (!apiKey) {
      return res.status(500).json({ success: false, error: 'API key not configured' });
    }

    if (!domain || !role || !level) {
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
        messages: [{ role: 'user', content: `Generate ONE interview question for ${level} level ${role} in ${domain}.` }],
        max_tokens: 200
      })
    });

    if (!response.ok) throw new Error('Groq API error');

    const data = await response.json();
    const question = data.choices?.[0]?.message?.content?.trim();

    return res.json({ success: true, question, domain, role, level });
  } catch (error) {
    return res.status(500).json({ success: false, error: error.message });
  }
}

module.exports = handler;
