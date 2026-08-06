"use client";

import { useEffect, useRef, useState } from "react";
import { FaRankingStar } from "react-icons/fa6";
import { ConfettiLayer } from "@/components/ConfettiLayer";
import { LeaderboardPodium } from "@/components/LeaderboardPodium";
import { LeaderboardTable } from "@/components/LeaderboardTable";
import { Shell } from "@/components/Shell";
import { StatusPill } from "@/components/StatusPill";
import { ToastStack } from "@/components/ToastStack";
import { useLiveEvent } from "@/hooks/useLiveEvent";

export default function LeaderboardPage() {
  const { state, toasts } = useLiveEvent();
  const [confetti, setConfetti] = useState(false);
  const lastHigh = useRef(0);

  useEffect(() => {
    if (state.stats.highestScore > lastHigh.current && lastHigh.current !== 0) {
      setConfetti(true);
      const timer = window.setTimeout(() => setConfetti(false), 2600);
      return () => window.clearTimeout(timer);
    }
    lastHigh.current = state.stats.highestScore;
  }, [state.stats.highestScore]);

  return (
    <Shell active="/leaderboard">
      <ToastStack toasts={toasts} />
      <ConfettiLayer active={confetti} />
      <section className="py-8">
        <div className="mb-5 flex flex-wrap items-end justify-between gap-3">
          <div>
            <StatusPill tone="orange">
              <FaRankingStar aria-hidden />
              Live Leaderboard
            </StatusPill>
            <h1 className="mt-4 text-5xl font-black text-white sm:text-6xl">Arena Rankings</h1>
          </div>
          <div className="glass-panel rounded-lg px-4 py-3 text-right">
            <p className="text-xs text-slate-400">Highest Score</p>
            <p className="text-3xl font-black text-orange-100">{state.stats.highestScore}</p>
          </div>
        </div>

        <LeaderboardPodium players={state.leaderboard} />
        <div className="mt-5">
          <LeaderboardTable players={state.leaderboard} />
        </div>
      </section>
    </Shell>
  );
}
