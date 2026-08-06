"use client";

import { useEffect, useMemo, useState } from "react";
import { FaBolt, FaGamepad, FaPaperPlane, FaStopwatch } from "react-icons/fa6";
import { ScoreHistoryChart } from "@/components/Charts";
import { MetricCard } from "@/components/MetricCard";
import { Shell } from "@/components/Shell";
import { StatusPill } from "@/components/StatusPill";
import { TelemetryGrid } from "@/components/TelemetryGrid";
import { ToastStack } from "@/components/ToastStack";
import { AnimatedNumber } from "@/components/AnimatedNumber";
import { useLiveEvent } from "@/hooks/useLiveEvent";
import { postJson } from "@/lib/api";

function useElapsed(since?: string) {
  const [now, setNow] = useState(Date.now());
  useEffect(() => {
    const timer = window.setInterval(() => setNow(Date.now()), 1000);
    return () => window.clearInterval(timer);
  }, []);
  if (!since) return "00:00";
  const seconds = Math.max(0, Math.floor((now - new Date(since).getTime()) / 1000));
  const minutes = Math.floor(seconds / 60).toString().padStart(2, "0");
  const rest = (seconds % 60).toString().padStart(2, "0");
  return `${minutes}:${rest}`;
}

export default function PlayingPage() {
  const { state, toasts, pushToast } = useLiveEvent();
  const player = state.currentPlayer ?? state.leaderboard[0] ?? null;
  const elapsed = useElapsed(player?.updatedAt);
  const [sending, setSending] = useState(false);
  const highScore = state.stats.highestScore;
  const currentScore = player?.score ?? state.telemetry?.score ?? 0;

  const demoPayload = useMemo(
    () => ({
      playerId: player?.id,
      name: player?.name ?? "Demo Player",
      branch: player?.branch ?? "Computer",
      game: player?.game ?? "Snake",
      score: currentScore + 7 + Math.floor(Math.random() * 16),
      level: (player?.level ?? 1) + 1,
      battery: Math.max(12, (player?.battery ?? 91) - 1),
      wifi: -52 - Math.floor(Math.random() * 20),
      fps: 28 + Math.floor(Math.random() * 5),
      heap: 170000 + Math.floor(Math.random() * 24000)
    }),
    [currentScore, player]
  );

  async function sendDemoPulse() {
    setSending(true);
    try {
      await postJson("/api/telemetry", demoPayload);
      pushToast({ title: "Demo packet sent", body: "Telemetry moved through the live API", tone: "success" });
    } catch (error) {
      pushToast({ title: "Packet failed", body: error instanceof Error ? error.message : "Unknown error", tone: "danger" });
    } finally {
      setSending(false);
    }
  }

  return (
    <Shell active="/playing">
      <ToastStack toasts={toasts} />
      <section className="py-8">
        <div className="mb-5 flex flex-wrap items-center gap-2">
          <StatusPill tone="green">
            <FaBolt aria-hidden />
            Now Playing
          </StatusPill>
          <StatusPill tone="cyan">
            <FaStopwatch aria-hidden />
            {elapsed}
          </StatusPill>
        </div>

        <div className="grid gap-5 lg:grid-cols-[1.1fr_0.9fr]">
          <div className="neon-border glass-panel rounded-lg p-6">
            <div className="flex items-start justify-between gap-4">
              <div className="min-w-0">
                <p className="text-sm text-slate-300">{player?.game ?? "Waiting for game"}</p>
                <h1 className="mt-2 truncate text-5xl font-black text-white sm:text-7xl">
                  {player?.name ?? "Awaiting Player"}
                </h1>
              </div>
              <div className="flex h-16 w-16 shrink-0 items-center justify-center rounded-lg border border-orange-300/30 bg-orange-400/15 text-3xl text-orange-100">
                <FaGamepad aria-hidden />
              </div>
            </div>

            <div className="mt-8 grid gap-4 sm:grid-cols-3">
              <div className="sm:col-span-2">
                <p className="text-sm text-slate-300">Current Score</p>
                <div className="mt-2 text-8xl font-black leading-none text-white">
                  <AnimatedNumber value={currentScore} />
                </div>
              </div>
              <div className="rounded-lg border border-white/10 bg-white/8 p-4">
                <p className="text-sm text-slate-300">High Score</p>
                <div className="mt-3 text-4xl font-black text-orange-100">
                  <AnimatedNumber value={highScore} />
                </div>
                <p className="mt-3 text-xs text-slate-400">Rank #{player?.rank ?? "-"}</p>
              </div>
            </div>

            <button
              type="button"
              onClick={sendDemoPulse}
              disabled={sending}
              className="mt-6 inline-flex items-center gap-2 rounded-md border border-cyan-300/30 bg-cyan-400/12 px-4 py-3 text-sm font-bold text-cyan-100 disabled:opacity-60"
            >
              <FaPaperPlane aria-hidden />
              {sending ? "Sending" : "Send Demo Pulse"}
            </button>
          </div>

          <div className="grid gap-4 sm:grid-cols-2 lg:grid-cols-1">
            <MetricCard label="Current Level" value={player?.level ?? 0} icon={FaGamepad} accent="purple" />
            <MetricCard label="Top Streak" value={state.stats.topStreak} icon={FaBolt} accent="green" />
          </div>
        </div>

        <div className="mt-5">
          <TelemetryGrid player={player} telemetry={state.telemetry} />
        </div>

        <div className="mt-5 grid gap-5 lg:grid-cols-[1fr_0.8fr]">
          <ScoreHistoryChart player={player} />
          <div className="glass-panel rounded-lg p-4">
            <h2 className="text-sm font-semibold text-white">Hardware Stream</h2>
            <div className="mt-4 space-y-3 text-sm">
              <div className="flex justify-between border-b border-white/10 pb-2"><span className="text-slate-400">Player</span><span className="text-white">{state.telemetry?.name ?? player?.name ?? "-"}</span></div>
              <div className="flex justify-between border-b border-white/10 pb-2"><span className="text-slate-400">Game</span><span className="text-white">{state.telemetry?.game ?? player?.game ?? "-"}</span></div>
              <div className="flex justify-between border-b border-white/10 pb-2"><span className="text-slate-400">Memory</span><span className="text-white">{(state.telemetry?.heap ?? player?.heap ?? 0).toLocaleString()} B</span></div>
              <div className="flex justify-between"><span className="text-slate-400">Last Packet</span><span className="text-white">{state.telemetry ? new Date(state.telemetry.createdAt).toLocaleTimeString() : "-"}</span></div>
            </div>
          </div>
        </div>
      </section>
    </Shell>
  );
}
