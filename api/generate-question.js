export default async function handler(req, res) {
  // CORS headers
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
      console.error('GROQ_API_KEY not configured');
      return res.status(500).json({ success: false, error: 'API key not configured' });
    }

    if (!domain || !role || !level) {
      return res.status(400).json({ success: false, error: 'Missing: domain, role, level' });
    }

    const prompt = `Generate ONE interview question for a ${level} level ${role} in ${domain}. Be professional and clear. Return ONLY the question, no explanation.`;

    const groqResponse = await fetch('https://api.groq.com/openai/v1/chat/completions', {
      method: 'POST',
      headers: {
        'Authorization': `Bearer ${apiKey}`,
        'Content-Type': 'application/json'
      },
      body: JSON.stringify({
        model: 'llama-3.3-70b-versatile',
        messages: [{ role: 'user', content: prompt }],
        max_tokens: 200,
        temperature: 0.7
      })
    });

    if (!groqResponse.ok) {
      const error = await groqResponse.json();
      throw new Error(`Groq API: ${error.error?.message || 'Unknown error'}`);
    }

    const data = await groqResponse.json();
    const question = data.choices?.[0]?.message?.content?.trim();

    if (!question) {
      throw new Error('No content in Groq response');
    }

    return res.status(200).json({
      success: true,
      question,
      domain,
      role,
      level
    });

  } catch (error) {
    console.error('generate-question error:', error.message);
    return res.status(500).json({
      success: false,
      error: error.message || 'Failed to generate question'
    });
  }
}
