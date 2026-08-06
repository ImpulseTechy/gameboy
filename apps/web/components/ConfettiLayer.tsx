"use client";

import { motion } from "framer-motion";

const confetti = Array.from({ length: 42 }, (_, index) => ({
  id: index,
  left: `${(index * 29) % 100}%`,
  color: ["#f97316", "#22d3ee", "#a855f7", "#22c55e", "#fb7185"][index % 5],
  delay: (index % 12) * 0.08
}));

export function ConfettiLayer({ active }: { active: boolean }) {
  if (!active) return null;

  return (
    <div className="pointer-events-none fixed inset-0 z-40 overflow-hidden">
      {confetti.map((piece) => (
        <motion.span
          key={piece.id}
          className="absolute top-[-24px] h-3 w-2 rounded-sm"
          style={{ left: piece.left, backgroundColor: piece.color }}
          initial={{ y: -20, rotate: 0, opacity: 0 }}
          animate={{ y: "110vh", rotate: 540, opacity: [0, 1, 1, 0] }}
          transition={{ duration: 2.2, delay: piece.delay, ease: "easeOut" }}
        />
      ))}
    </div>
  );
}
