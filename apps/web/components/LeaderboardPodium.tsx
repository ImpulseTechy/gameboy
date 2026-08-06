"use client";

import { motion } from "framer-motion";
import { FaCrown, FaMedal, FaTrophy } from "react-icons/fa6";
import type { PublicPlayer } from "@/lib/types";
import { AnimatedNumber } from "./AnimatedNumber";

const podiumStyles = [
  {
    label: "Champion",
    height: "min-h-56",
    color: "border-orange-300/40 bg-orange-400/15",
    icon: FaCrown
  },
  {
    label: "Runner Up",
    height: "min-h-44",
    color: "border-cyan-300/35 bg-cyan-400/12",
    icon: FaTrophy
  },
  {
    label: "Third Place",
    height: "min-h-40",
    color: "border-purple-300/35 bg-purple-400/12",
    icon: FaMedal
  }
];

export function LeaderboardPodium({ players }: { players: PublicPlayer[] }) {
  const order = [1, 0, 2];

  return (
    <div className="grid items-end gap-4 md:grid-cols-3">
      {order.map((playerIndex) => {
        const player = players[playerIndex];
        const style = podiumStyles[playerIndex];
        const Icon = style.icon;
        return (
          <motion.div
            key={player?.id ?? playerIndex}
            className={`glass-panel rounded-lg border p-5 ${style.height} ${style.color}`}
            initial={{ y: 28, opacity: 0 }}
            animate={{ y: 0, opacity: 1 }}
            transition={{ delay: playerIndex * 0.08 }}
          >
            <div className="flex items-center justify-between">
              <span className="text-sm text-slate-300">Rank {playerIndex + 1}</span>
              <Icon className="text-2xl text-orange-200" aria-hidden />
            </div>
            {player ? (
              <>
                <p className="mt-5 text-sm text-slate-300">{style.label}</p>
                <h2 className="mt-1 truncate text-2xl font-black text-white">{player.name}</h2>
                <p className="mt-1 text-sm text-cyan-100">{player.branch} / {player.game}</p>
                <div className="mt-6 text-5xl font-black text-white">
                  <AnimatedNumber value={player.score} />
                </div>
                <div className="mt-4 flex flex-wrap gap-2">
                  {player.badges.slice(0, 3).map((badge) => (
                    <span key={badge} className="rounded-md border border-white/10 bg-white/8 px-2 py-1 text-xs text-slate-200">
                      {badge}
                    </span>
                  ))}
                </div>
              </>
            ) : (
              <div className="mt-10 text-sm text-slate-400">Awaiting contender</div>
            )}
          </motion.div>
        );
      })}
    </div>
  );
}
