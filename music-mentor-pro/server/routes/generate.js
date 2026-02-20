import { Router } from 'express';
import { generateLyrics } from '../services/geminiService.js';

const router = Router();

router.post('/', async (req, res) => {
  const { level, vibe, style, mode, isExplicit } = req.body;

  if (!level || !vibe || !style || !mode || typeof isExplicit !== 'boolean') {
    return res.status(400).json({ error: 'Niepoprawne dane wejściowe.' });
  }

  try {
    const text = await generateLyrics({ level, vibe, style, mode, isExplicit });
    return res.json({ text });
  } catch (error) {
    return res.status(500).json({ error: error.message || 'Błąd serwera.' });
  }
});

export default router;
