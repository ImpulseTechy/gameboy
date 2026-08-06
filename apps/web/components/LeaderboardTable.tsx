"use client";

import { motion } from "framer-motion";
import { FaArrowTrendUp, FaBolt } from "react-icons/fa6";
import type { PublicPlayer } from "@/lib/types";
import { AnimatedNumber } from "./AnimatedNumber";

export function LeaderboardTable({
  players,
  limit
}: {
  players: PublicPlayer[];
  limit?: number;
}) {
  const list = typeof limit === "number" ? players.slice(0, limit) : players;

  return (
    <div className="glass-panel overflow-hidden rounded-lg">
      <div className="grid grid-cols-[64px_1fr_96px_110px] gap-3 border-b border-white/10 px-4 py-3 text-xs text-slate-400 sm:grid-cols-[72px_80px_1fr_120px_120px_140px]">
        <span>Rank</span>
        <span className="hidden sm:block">Avatar</span>
        <span>Player</span>
        <span className="hidden sm:block">Game</span>
        <span>Score</span>
        <span>Badges</span>
      </div>
      <div className="divide-y divide-white/8">
        {list.length === 0 && <div className="p-5 text-sm text-slate-400">No players yet.</div>}
        {list.map((player) => (
          <motion.div
            layout
            key={player.id}
            className="grid grid-cols-[64px_1fr_96px_110px] items-center gap-3 px-4 py-3 sm:grid-cols-[72px_80px_1fr_120px_120px_140px]"
            initial={{ opacity: 0, x: -14 }}
            animate={{ opacity: 1, x: 0 }}
          >
            <div className="text-xl font-black text-white">#{player.rank}</div>
            <div className="hidden sm:flex">
              <div className="flex h-11 w-11 items-center justify-center rounded-lg border border-cyan-300/30 bg-cyan-400/12 text-sm font-black text-cyan-100">
                {player.name.slice(0, 2).toUpperCase()}
              </div>
            </div>
            <div className="min-w-0">
              <p className="truncate font-semibold text-white">{player.name}</p>
              <p className="truncate text-xs text-slate-400">{player.branch} / {player.playerCode}</p>
            </div>
            <div className="hidden text-sm text-slate-200 sm:block">{player.game}</div>
            <div className="text-xl font-black text-white">
              <AnimatedNumber value={player.score} />
            </div>
            <div className="flex min-w-0 flex-wrap gap-1">
              {player.trend > 0 && (
                <span className="inline-flex items-center gap-1 rounded-md border border-emerald-300/25 bg-emerald-400/12 px-2 py-1 text-xs text-emerald-100">
                  <FaArrowTrendUp aria-hidden /> +{player.trend}
                </span>
              )}
              {player.badges.slice(0, 1).map((badge) => (
                <span key={badge} className="inline-flex max-w-full items-center gap-1 rounded-md border border-orange-300/25 bg-orange-400/12 px-2 py-1 text-xs text-orange-100">
                  <FaBolt aria-hidden /> <span className="truncate">{badge}</span>
                </span>
              ))}
            </div>
          </motion.div>
        ))}
      </div>
    </div>
  );
}
