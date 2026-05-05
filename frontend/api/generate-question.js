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
    const apiKey = process.env.GROQ_API_KEY || process.env.GROK_API_KEY;

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
      'Monitoring and observability'
    ];
    
    const selectedTopic = topics[index % topics.length];
    const previousContext = askedQuestions.length > 0 
      ? `\n\nIMPORTANT: DO NOT ask these topics again - they were already asked:\n${askedQuestions.map((q, i) => `${i+1}. ${q.substring(0, 100)}...`).join('\n')}`
      : '';
    
    const promptText = `Generate ONE unique, specific interview question for a ${level} level ${role} in ${domain}.\n\nTopic focus: ${selectedTopic}\n\nRequirements:\n- Question must be DIFFERENT from any topic/concept already covered${previousContext}\n- Make it practical and scenario-based\n- Avoid generic questions\n- Return ONLY the question text, no explanations`;
    
    const response = await fetch('https://api.groq.com/openai/v1/chat/completions', {
      method: 'POST',
      headers: {
        'Authorization': `Bearer ${apiKey}`,
        'Content-Type': 'application/json'
      },
      body: JSON.stringify({
        model: 'llama-3.3-70b-versatile',
        messages: [{ role: 'user', content: promptText }],
        max_tokens: 200,
        temperature: 1.0
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
