import { motion } from 'framer-motion';

export default function Loader() {
  return (
    <div className="flex items-center justify-center py-6">
      <motion.div
        className="h-10 w-10 rounded-full border-4 border-accent border-t-transparent"
        animate={{ rotate: 360 }}
        transition={{ repeat: Infinity, duration: 1, ease: 'linear' }}
      />
    </div>
  );
}
