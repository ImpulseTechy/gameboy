"use client";

import { FaCrown, FaGamepad, FaSignal, FaTowerBroadcast, FaUsers } from "react-icons/fa6";
import { AnimatedNumber } from "@/components/AnimatedNumber";
import { Clock } from "@/components/Clock";
import { FullscreenButton } from "@/components/FullscreenButton";
import { LeaderboardTable } from "@/components/LeaderboardTable";
import { MetricCard } from "@/components/MetricCard";
import { Shell } from "@/components/Shell";
import { StatusPill } from "@/components/StatusPill";
import { useLiveEvent } from "@/hooks/useLiveEvent";
import { API_URL } from "@/lib/api";

export default function BigScreenPage() {
  const { state, connected } = useLiveEvent();
  const player = state.currentPlayer;

  return (
    <Shell active="/big-screen" compact>
      <section className="min-h-[calc(100vh-92px)] py-5">
        <div className="mb-4 flex flex-wrap items-center justify-between gap-3">
          <div className="flex flex-wrap gap-2">
            <StatusPill tone="orange">
              <FaTowerBroadcast aria-hidden />
              Big Screen Mode
            </StatusPill>
            <StatusPill tone={connected ? "green" : "rose"}>
              <FaSignal aria-hidden />
              {connected ? "Socket Live" : "Socket Waiting"}
            </StatusPill>
          </div>
          <div className="flex items-center gap-3">
            <Clock className="rounded-md border border-white/15 bg-white/10 px-3 py-2 text-sm font-bold text-white" />
            <FullscreenButton />
          </div>
        </div>

        <div className="grid gap-5 lg:grid-cols-[1.12fr_0.88fr]">
          <div className="neon-border glass-panel rounded-lg p-6">
            <p className="text-sm text-cyan-100">Current Player</p>
            <h1 className="mt-2 truncate text-7xl font-black leading-none text-white">
              {player?.name ?? "Waiting"}
            </h1>
            <p className="mt-3 text-2xl text-slate-200">
              {player?.branch ?? "Open Arena"} / {player?.game ?? state.stats.currentGame}
            </p>
            <div className="mt-8 grid gap-4 sm:grid-cols-[1fr_220px]">
              <div>
                <p className="text-sm text-slate-300">Current Score</p>
                <div className="text-9xl font-black leading-none text-white">
                  <AnimatedNumber value={player?.score ?? 0} />
                </div>
              </div>
              <div className="rounded-lg border border-orange-300/25 bg-orange-400/12 p-4">
                <p className="text-sm text-orange-100">Current Rank</p>
                <p className="mt-2 text-7xl font-black text-white">#{player?.rank ?? "-"}</p>
              </div>
            </div>
          </div>

          <div className="grid gap-4 sm:grid-cols-2">
            <MetricCard label="Participants" value={state.stats.participants} icon={FaUsers} accent="cyan" />
            <MetricCard label="Highest Score" value={state.stats.highestScore} icon={FaCrown} accent="orange" />
            <MetricCard label="Current Game" value={state.stats.currentGame} icon={FaGamepad} accent="purple" />
            <div className="glass-panel rounded-lg p-4">
              <p className="text-xs text-slate-300">Current Challenge</p>
              <p className="mt-3 text-3xl font-black text-white">
                {state.currentChallenge ? `Beat ${state.currentChallenge.targetScore}` : state.lastMystery?.title ?? "Classic Run"}
              </p>
              <p className="mt-2 text-sm text-slate-300">{state.lastMystery?.description ?? "Highest score wins the arena."}</p>
            </div>
          </div>
        </div>

        <div className="mt-5 grid gap-5 xl:grid-cols-[1fr_340px]">
          <LeaderboardTable players={state.leaderboard} limit={8} />
          <div className="glass-panel rounded-lg p-5">
            <p className="text-sm text-slate-300">Next Player</p>
            <h2 className="mt-2 truncate text-4xl font-black text-white">{state.nextPlayer?.name ?? "Queue Open"}</h2>
            <p className="mt-2 text-cyan-100">{state.nextPlayer?.game ?? "Snake or Tetris"}</p>
            <div className="mt-6 rounded-lg border border-white/10 bg-white/8 p-4 text-center">
              {state.settings.collegeLogo ? (
                <img src={`${API_URL}${state.settings.collegeLogo}`} alt="College logo" className="mx-auto max-h-24 object-contain" />
              ) : (
                <div className="text-3xl font-black text-white">NST</div>
              )}
              <p className="mt-3 text-xs text-slate-400">Sponsor Area</p>
            </div>
          </div>
        </div>
      </section>
    </Shell>
  );
}
