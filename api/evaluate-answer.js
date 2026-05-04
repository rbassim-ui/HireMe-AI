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
    const { question, answer, level, domain } = req.body;
    const apiKey = process.env.GROQ_API_KEY;

    if (!apiKey) {
      console.error('GROQ_API_KEY not configured');
      return res.status(500).json({ success: false, error: 'API key not configured' });
    }

    if (!question || !answer) {
      return res.status(400).json({ success: false, error: 'Missing: question, answer' });
    }

    const prompt = `Evaluate this interview answer on a scale of 1-10.
Question: ${question}
Answer: ${answer}

Reply ONLY with valid JSON (no markdown):
{"score": 7, "feedback": "Good answer because...", "strengths": ["point1"], "improvements": ["point1"]}`;

    const groqResponse = await fetch('https://api.groq.com/openai/v1/chat/completions', {
      method: 'POST',
      headers: {
        'Authorization': `Bearer ${apiKey}`,
        'Content-Type': 'application/json'
      },
      body: JSON.stringify({
        model: 'llama-3.3-70b-versatile',
        messages: [{ role: 'user', content: prompt }],
        max_tokens: 300,
        temperature: 0.5
      })
    });

    if (!groqResponse.ok) {
      const error = await groqResponse.json();
      throw new Error(`Groq API: ${error.error?.message || 'Unknown error'}`);
    }

    const data = await groqResponse.json();
    let responseText = data.choices?.[0]?.message?.content?.trim();

    if (!responseText) {
      throw new Error('No content in Groq response');
    }

    // Extract JSON if wrapped in markdown
    const jsonMatch = responseText.match(/\{[\s\S]*\}/);
    if (jsonMatch) {
      responseText = jsonMatch[0];
    }

    const evaluation = JSON.parse(responseText);
    
    // Validate and normalize score
    let score = Number(evaluation.score) || 6;
    score = Math.max(1, Math.min(10, score));

    return res.status(200).json({
      success: true,
      score,
      feedback: evaluation.feedback || 'Good response',
      strengths: Array.isArray(evaluation.strengths) ? evaluation.strengths : ['Clear answer'],
      improvements: Array.isArray(evaluation.improvements) ? evaluation.improvements : ['More detail']
    });

  } catch (error) {
    console.error('evaluate-answer error:', error.message);
    return res.status(500).json({
      success: false,
      error: error.message || 'Failed to evaluate answer'
    });
  }
}
