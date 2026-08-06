"use client";

import Link from "next/link";
import {
  FaCalendarDay,
  FaGamepad,
  FaMedal,
  FaMicrochip,
  FaSignal,
  FaTowerBroadcast,
  FaUsers
} from "react-icons/fa6";
import { ActivityFeed } from "@/components/ActivityFeed";
import { BranchBattleChart } from "@/components/Charts";
import { Clock } from "@/components/Clock";
import { LeaderboardTable } from "@/components/LeaderboardTable";
import { MetricCard } from "@/components/MetricCard";
import { ModeStrip } from "@/components/ModeStrip";
import { QrJoin } from "@/components/QrJoin";
import { Shell } from "@/components/Shell";
import { StatusPill } from "@/components/StatusPill";
import { ToastStack } from "@/components/ToastStack";
import { useLiveEvent } from "@/hooks/useLiveEvent";

export default function HomePage() {
  const { state, mode, connected, toasts } = useLiveEvent();

  return (
    <Shell active="/">
      <ToastStack toasts={toasts} />
      <section className="grid min-h-[calc(100vh-92px)] gap-6 pb-8 pt-10 lg:grid-cols-[1.12fr_0.88fr] lg:items-center">
        <div>
          <div className="mb-5 flex flex-wrap gap-2">
            <StatusPill tone={connected ? "green" : "rose"}>
              <FaTowerBroadcast aria-hidden />
              {connected ? "Realtime Online" : "Realtime Offline"}
            </StatusPill>
            <StatusPill tone={state.stats.esp32Connected ? "green" : "orange"}>
              <FaMicrochip aria-hidden />
              ESP32 {state.stats.esp32Connected ? "Connected" : "Waiting"}
            </StatusPill>
            <StatusPill tone="cyan">
              <FaSignal aria-hidden />
              {state.stats.wifiStatus}
            </StatusPill>
          </div>

          <h1 className="max-w-4xl text-6xl font-black leading-none text-white sm:text-7xl lg:text-8xl">
            NST ARCADE LIVE
          </h1>
          <p className="mt-5 max-w-2xl text-xl font-medium text-cyan-100 sm:text-2xl">
            Where Engineering Meets Gaming.
          </p>
          <p className="mt-5 max-w-2xl text-base leading-7 text-slate-300">
            ESP32 handheld consoles stream scores, telemetry, and challenge moments into a giant live tournament wall for first year engineering orientation.
          </p>

          <div className="mt-8 flex flex-wrap gap-3">
            <Link
              href="/register"
              className="inline-flex items-center gap-2 rounded-md bg-orange-500 px-5 py-3 text-sm font-bold text-white shadow-[0_0_28px_rgba(249,115,22,0.4)]"
            >
              <FaGamepad aria-hidden />
              Ready To Play
            </Link>
            <Link
              href="/big-screen"
              className="inline-flex items-center gap-2 rounded-md border border-cyan-300/30 bg-cyan-400/12 px-5 py-3 text-sm font-bold text-cyan-100"
            >
              <FaTowerBroadcast aria-hidden />
              Projector Mode
            </Link>
          </div>
        </div>

        <div className="grid gap-4 sm:grid-cols-2">
          <MetricCard label="Current Participants" value={state.stats.participants} icon={FaUsers} accent="cyan" />
          <MetricCard label="Highest Score" value={state.stats.highestScore} icon={FaMedal} accent="orange" />
          <MetricCard label="Current Game" value={state.stats.currentGame} icon={FaGamepad} accent="purple" />
          <MetricCard label="Live Clock" value={<Clock /> as unknown as string} icon={FaCalendarDay} accent="green" />
          <div className="glass-panel rounded-lg p-4 sm:col-span-2">
            <p className="text-xs text-slate-300">Today&apos;s Champion</p>
            <p className="mt-3 truncate text-4xl font-black text-white">{state.stats.champion}</p>
            <p className="mt-2 text-sm text-orange-100">{mode?.label ?? "Classic"} / {mode?.kicker ?? "Highest score wins"}</p>
          </div>
        </div>
      </section>

      <section className="grid gap-5 lg:grid-cols-[0.9fr_1.1fr]">
        <div className="space-y-5">
          <QrJoin />
          <ActivityFeed activity={state.activity} />
        </div>
        <div className="space-y-5">
          <ModeStrip modes={state.gameModes} currentMode={state.currentMode} />
          <LeaderboardTable players={state.leaderboard} limit={6} />
          <BranchBattleChart branches={state.branchBattle} />
        </div>
      </section>
    </Shell>
  );
}
