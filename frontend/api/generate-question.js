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

    // Build prompt with strict diversity instruction and previous questions context
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
    
    const promptText = `You MUST generate a COMPLETELY DIFFERENT interview question.\n\nGenerate ONE technical interview question for a ${level} level ${role} in ${domain}.\nFocus area: ${selectedTopic}\n\n${previousQuestionsList ? `QUESTIONS ALREADY ASKED (DO NOT REPEAT THESE TOPICS OR CONCEPTS):\n${previousQuestionsList}\n\n` : ''}STRICT REQUIREMENTS:\n- Generate a question on COMPLETELY DIFFERENT topic than above\n- Do NOT ask about architecture patterns already covered\n- Do NOT repeat similar concepts\n- Make it specific and practical\n- Return ONLY the question text (no preamble, no explanation)\n- If you would ask same topic as above, pick something completely unrelated instead`;
    
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
