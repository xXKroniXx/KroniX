/** @type {import('tailwindcss').Config} */
export default {
  content: ['./index.html', './**/*.{js,jsx}'],
  theme: {
    extend: {
      colors: {
        accent: '#7c4dff',
        primary: '#00e676',
        explicitAccent: '#bc13fe'
      },
      boxShadow: {
        neon: '0 0 15px rgba(188, 19, 254, 0.45)'
      }
    }
  },
  plugins: []
};
