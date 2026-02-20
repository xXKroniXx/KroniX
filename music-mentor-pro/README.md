# 🎤 Music Mentor PRO

Kompletna aplikacja full-stack do generowania rapowych wersów z pomocą Google Gemini.

## Stack

- **Frontend:** React (Vite), TailwindCSS, Framer Motion, Fetch API
- **Backend:** Node.js, Express, CORS, dotenv, Gemini API

## Struktura

```txt
music-mentor-pro/
├── client/
│   ├── components/
│   ├── styles/
│   ├── App.jsx
│   ├── main.jsx
│   └── ...
└── server/
    ├── routes/
    ├── services/
    ├── utils/
    └── index.js
```

## Instalacja

### 1) Backend

```bash
cd music-mentor-pro/server
npm install
cp .env.example .env
```

Uzupełnij `.env`:

```env
GEMINI_API_KEY=xxxx
PORT=3001
```

Start backendu:

```bash
npm run dev
```

### 2) Frontend

W drugim terminalu:

```bash
cd music-mentor-pro/client
npm install
npm run dev
```

Aplikacja frontend uruchomi się domyślnie na `http://localhost:5173`.

## Opcjonalna konfiguracja frontendu

Jeśli backend działa pod innym adresem, ustaw:

```env
VITE_API_URL=http://localhost:3001/api/generate
```

## Funkcje

- 5 poziomów zaawansowania z wpływem na styl tekstu
- Vibe, styl i tryb (HIT/UNDERGROUND)
- Tryb 18+ z animowaną zmianą UI
- Loader + animowany result card
- Kopiowanie wyniku do schowka
- Generuj ponownie
- Historia ostatnich 5 tekstów (localStorage)
- Prompt budowany dynamicznie po stronie backendu
- API key przechowywany wyłącznie po stronie backendu
