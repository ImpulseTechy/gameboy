"use client";

import { motion } from "framer-motion";
import type { ReactNode } from "react";

const particles = Array.from({ length: 42 }, (_, index) => ({
  id: index,
  left: `${(index * 37) % 100}%`,
  delay: `${(index * 0.37) % 6}s`,
  duration: `${6 + (index % 6)}s`,
  size: `${2 + (index % 4)}px`,
  x: `${(index % 2 === 0 ? 1 : -1) * (20 + (index % 7) * 12)}px`
}));

export function AnimatedBackdrop({ children }: { children: ReactNode }) {
  return (
    <div className="stage-background relative min-h-screen overflow-hidden">
      <div className="grid-overlay pointer-events-none absolute inset-0 opacity-70" />
      <div className="scanline pointer-events-none absolute inset-x-0 top-0 h-1/3 opacity-30" />
      <div className="pointer-events-none absolute inset-0">
        {particles.map((particle) => (
          <span
            key={particle.id}
            className="absolute bottom-[-20px] rounded-full bg-cyan-200 shadow-[0_0_14px_rgba(34,211,238,0.9)]"
            style={{
              left: particle.left,
              width: particle.size,
              height: particle.size,
              animation: `float-particle ${particle.duration} linear infinite`,
              animationDelay: particle.delay,
              "--x": particle.x
            } as React.CSSProperties}
          />
        ))}
      </div>
      <motion.div
        className="relative z-10 min-h-screen"
        initial={{ opacity: 0 }}
        animate={{ opacity: 1 }}
        transition={{ duration: 0.45 }}
      >
        {children}
      </motion.div>
    </div>
  );
}
