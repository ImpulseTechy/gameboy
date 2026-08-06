"use client";

import { FormEvent, useEffect, useMemo, useState } from "react";
import {
  FaArrowRotateRight,
  FaDownload,
  FaFileCsv,
  FaFileExcel,
  FaFileExport,
  FaGamepad,
  FaImage,
  FaLock,
  FaPaperPlane,
  FaPlus,
  FaShuffle,
  FaTrash,
  FaUserTie
} from "react-icons/fa6";
import { BranchBattleChart } from "@/components/Charts";
import { FullscreenButton } from "@/components/FullscreenButton";
import { LeaderboardTable } from "@/components/LeaderboardTable";
import { MetricCard } from "@/components/MetricCard";
import { MysteryWheel } from "@/components/MysteryWheel";
import { Shell } from "@/components/Shell";
import { StatusPill } from "@/components/StatusPill";
import { ToastStack } from "@/components/ToastStack";
import { useLiveEvent } from "@/hooks/useLiveEvent";
import { apiFetch, downloadExport, postJson } from "@/lib/api";

const branches = ["Computer", "AI", "Electrical", "Electronics", "Mechanical"];
const games = ["Snake", "Tetris"];

export default function AdminPage() {
  const { state, toasts, pushToast, refresh } = useLiveEvent();
  const [token, setToken] = useState("");
  const [login, setLogin] = useState({ username: "admin", password: "nst-arcade" });
  const [loginError, setLoginError] = useState("");
  const [busy, setBusy] = useState(false);
  const [spinning, setSpinning] = useState(false);
  const [newPlayer, setNewPlayer] = useState({
    name: "",
    branch: branches[0],
    rollNumber: "",
    game: games[0],
    prediction: ""
  });
  const [scoreEdit, setScoreEdit] = useState({ playerId: "", score: "" });
  const [professorScore, setProfessorScore] = useState("67");
  const [judgeName, setJudgeName] = useState("");
  const [gameName, setGameName] = useState(games[0]);
  const [logo, setLogo] = useState<File | null>(null);

  useEffect(() => {
    setToken(window.localStorage.getItem("nst-admin-token") ?? "");
  }, []);

  useEffect(() => {
    if (!scoreEdit.playerId && state.leaderboard[0]) {
      setScoreEdit((current) => ({ ...current, playerId: state.leaderboard[0].id }));
    }
  }, [scoreEdit.playerId, state.leaderboard]);

  const selectedPlayer = useMemo(
    () => state.leaderboard.find((player) => player.id === scoreEdit.playerId) ?? null,
    [scoreEdit.playerId, state.leaderboard]
  );

  async function handleLogin(event: FormEvent<HTMLFormElement>) {
    event.preventDefault();
    setLoginError("");
    setBusy(true);
    try {
      const response = await postJson<{ token: string }>("/api/auth/login", login);
      setToken(response.token);
      window.localStorage.setItem("nst-admin-token", response.token);
      pushToast({ title: "Admin unlocked", body: "Control room is live", tone: "success" });
    } catch (error) {
      setLoginError(error instanceof Error ? error.message : "Login failed");
    } finally {
      setBusy(false);
    }
  }

  async function adminAction<T>(action: () => Promise<T>, success: string) {
    setBusy(true);
    try {
      const result = await action();
      await refresh();
      pushToast({ title: success, body: "Command accepted by the live server", tone: "success" });
      return result;
    } catch (error) {
      pushToast({
        title: "Admin command failed",
        body: error instanceof Error ? error.message : "Unknown error",
        tone: "danger"
      });
      return null;
    } finally {
      setBusy(false);
    }
  }

  async function addPlayer(event: FormEvent<HTMLFormElement>) {
    event.preventDefault();
    const payload = {
      ...newPlayer,
      prediction: newPlayer.prediction ? Number(newPlayer.prediction) : undefined
    };
    await adminAction(
      () => postJson("/api/admin/player", payload, token),
      "Player added"
    );
    setNewPlayer((current) => ({ ...current, name: "", rollNumber: "", prediction: "" }));
  }

  async function updateScore(event: FormEvent<HTMLFormElement>) {
    event.preventDefault();
    if (!selectedPlayer) return;
    await adminAction(
      () =>
        postJson(
          "/api/score",
          {
            playerId: selectedPlayer.id,
            game: selectedPlayer.game,
            score: Number(scoreEdit.score),
            level: selectedPlayer.level,
            battery: selectedPlayer.battery,
            wifi: selectedPlayer.wifi,
            fps: selectedPlayer.fps,
            heap: selectedPlayer.heap
          },
          token
        ),
      "Score updated"
    );
    setScoreEdit((current) => ({ ...current, score: "" }));
  }

  async function spinWheel() {
    setSpinning(true);
    await adminAction(() => postJson("/api/mystery", {}, token), "Mystery event selected");
    window.setTimeout(() => setSpinning(false), 1700);
  }

  async function uploadLogo(event: FormEvent<HTMLFormElement>) {
    event.preventDefault();
    if (!logo) return;
    const formData = new FormData();
    formData.append("logo", logo);
    await adminAction(
      () =>
        apiFetch("/api/admin/logo", {
          method: "POST",
          body: formData,
          token
        }),
      "Logo uploaded"
    );
    setLogo(null);
  }

  if (!token) {
    return (
      <Shell active="/admin">
        <section className="mx-auto max-w-md py-16">
          <form onSubmit={handleLogin} className="glass-panel rounded-lg p-5">
            <StatusPill tone="orange">
              <FaLock aria-hidden />
              Secure Login
            </StatusPill>
            <h1 className="mt-5 text-4xl font-black text-white">Admin Panel</h1>
            <label className="mt-5 block">
              <span className="text-sm text-slate-300">Username</span>
              <input
                value={login.username}
                onChange={(event) => setLogin({ ...login, username: event.target.value })}
                className="mt-2 w-full rounded-md border border-white/10 bg-white/8 px-3 py-3 text-white"
              />
            </label>
            <label className="mt-4 block">
              <span className="text-sm text-slate-300">Password</span>
              <input
                type="password"
                value={login.password}
                onChange={(event) => setLogin({ ...login, password: event.target.value })}
                className="mt-2 w-full rounded-md border border-white/10 bg-white/8 px-3 py-3 text-white"
              />
            </label>
            {loginError && <p className="mt-4 rounded-md border border-rose-300/30 bg-rose-400/12 p-3 text-sm text-rose-100">{loginError}</p>}
            <button
              type="submit"
              disabled={busy}
              className="mt-5 inline-flex w-full items-center justify-center gap-2 rounded-md bg-orange-500 px-4 py-3 text-sm font-bold text-white disabled:opacity-60"
            >
              <FaLock aria-hidden />
              {busy ? "Checking" : "Enter Control Room"}
            </button>
          </form>
        </section>
      </Shell>
    );
  }

  return (
    <Shell active="/admin">
      <ToastStack toasts={toasts} />
      <section className="py-8">
        <div className="mb-5 flex flex-wrap items-center justify-between gap-3">
          <div>
            <StatusPill tone="cyan">
              <FaUserTie aria-hidden />
              Control Room
            </StatusPill>
            <h1 className="mt-4 text-5xl font-black text-white">Admin Panel</h1>
          </div>
          <div className="flex flex-wrap gap-2">
            <FullscreenButton />
            <button
              type="button"
              onClick={() => {
                window.localStorage.removeItem("nst-admin-token");
                setToken("");
              }}
              className="inline-flex items-center gap-2 rounded-md border border-white/15 bg-white/10 px-3 py-2 text-sm font-semibold text-white"
            >
              <FaLock aria-hidden />
              Lock
            </button>
          </div>
        </div>

        <div className="grid gap-4 md:grid-cols-2 xl:grid-cols-4">
          <MetricCard label="Participants" value={state.stats.participants} icon={FaUserTie} accent="cyan" />
          <MetricCard label="Highest Score" value={state.stats.highestScore} icon={FaGamepad} accent="orange" />
          <MetricCard label="Average Score" value={state.stats.averageScore} icon={FaShuffle} accent="green" />
          <MetricCard label="Professor Score" value={state.currentChallenge?.targetScore ?? 0} icon={FaUserTie} accent="purple" />
        </div>

        <div className="mt-5 grid gap-5 xl:grid-cols-[0.82fr_1.18fr]">
          <div className="space-y-5">
            <form onSubmit={addPlayer} className="glass-panel rounded-lg p-4">
              <h2 className="text-sm font-semibold text-white">Add Player</h2>
              <div className="mt-4 grid gap-3 sm:grid-cols-2">
                <input
                  required
                  placeholder="Name"
                  value={newPlayer.name}
                  onChange={(event) => setNewPlayer({ ...newPlayer, name: event.target.value })}
                  className="rounded-md border border-white/10 bg-white/8 px-3 py-2 text-white"
                />
                <input
                  placeholder="Roll number"
                  value={newPlayer.rollNumber}
                  onChange={(event) => setNewPlayer({ ...newPlayer, rollNumber: event.target.value })}
                  className="rounded-md border border-white/10 bg-white/8 px-3 py-2 text-white"
                />
                <select
                  value={newPlayer.branch}
                  onChange={(event) => setNewPlayer({ ...newPlayer, branch: event.target.value })}
                  className="rounded-md border border-white/10 bg-white/8 px-3 py-2 text-white"
                >
                  {branches.map((branch) => <option key={branch}>{branch}</option>)}
                </select>
                <select
                  value={newPlayer.game}
                  onChange={(event) => setNewPlayer({ ...newPlayer, game: event.target.value })}
                  className="rounded-md border border-white/10 bg-white/8 px-3 py-2 text-white"
                >
                  {games.map((game) => <option key={game}>{game}</option>)}
                </select>
                <input
                  inputMode="numeric"
                  placeholder="Prediction"
                  value={newPlayer.prediction}
                  onChange={(event) => setNewPlayer({ ...newPlayer, prediction: event.target.value })}
                  className="rounded-md border border-white/10 bg-white/8 px-3 py-2 text-white sm:col-span-2"
                />
              </div>
              <button type="submit" disabled={busy} className="mt-4 inline-flex w-full items-center justify-center gap-2 rounded-md bg-orange-500 px-4 py-2.5 text-sm font-bold text-white disabled:opacity-60">
                <FaPlus aria-hidden />
                Add Player
              </button>
            </form>

            <form onSubmit={updateScore} className="glass-panel rounded-lg p-4">
              <h2 className="text-sm font-semibold text-white">Edit Score</h2>
              <div className="mt-4 grid gap-3">
                <select
                  value={scoreEdit.playerId}
                  onChange={(event) => setScoreEdit({ ...scoreEdit, playerId: event.target.value })}
                  className="rounded-md border border-white/10 bg-white/8 px-3 py-2 text-white"
                >
                  {state.leaderboard.map((player) => (
                    <option key={player.id} value={player.id}>{player.name} / {player.score}</option>
                  ))}
                </select>
                <input
                  required
                  inputMode="numeric"
                  placeholder="New score"
                  value={scoreEdit.score}
                  onChange={(event) => setScoreEdit({ ...scoreEdit, score: event.target.value })}
                  className="rounded-md border border-white/10 bg-white/8 px-3 py-2 text-white"
                />
              </div>
              <button type="submit" disabled={busy || !selectedPlayer} className="mt-4 inline-flex w-full items-center justify-center gap-2 rounded-md border border-cyan-300/30 bg-cyan-400/12 px-4 py-2.5 text-sm font-bold text-cyan-100 disabled:opacity-60">
                <FaPaperPlane aria-hidden />
                Update Score
              </button>
            </form>

            <div className="glass-panel rounded-lg p-4">
              <h2 className="text-sm font-semibold text-white">Exports</h2>
              <div className="mt-4 grid grid-cols-3 gap-2">
                <button type="button" onClick={() => downloadExport("csv", token)} className="inline-flex items-center justify-center gap-2 rounded-md border border-white/10 bg-white/8 px-3 py-2 text-sm text-white">
                  <FaFileCsv aria-hidden /> CSV
                </button>
                <button type="button" onClick={() => downloadExport("xlsx", token)} className="inline-flex items-center justify-center gap-2 rounded-md border border-white/10 bg-white/8 px-3 py-2 text-sm text-white">
                  <FaFileExcel aria-hidden /> Excel
                </button>
                <button type="button" onClick={() => downloadExport("json", token)} className="inline-flex items-center justify-center gap-2 rounded-md border border-white/10 bg-white/8 px-3 py-2 text-sm text-white">
                  <FaFileExport aria-hidden /> JSON
                </button>
              </div>
            </div>
          </div>

          <div className="space-y-5">
            <MysteryWheel
              events={state.mysteryEvents}
              spinning={spinning}
              selected={state.lastMystery}
              onSpin={spinWheel}
            />

            <div className="grid gap-5 lg:grid-cols-2">
              <div className="glass-panel rounded-lg p-4">
                <h2 className="text-sm font-semibold text-white">Switch Game Mode</h2>
                <div className="mt-4 grid gap-2">
                  {state.gameModes.map((mode) => (
                    <button
                      key={mode.id}
                      type="button"
                      onClick={() => adminAction(() => postJson("/api/admin/mode", { mode: mode.id }, token), "Mode switched")}
                      className={`rounded-md border px-3 py-2 text-left text-sm ${mode.id === state.currentMode ? "border-orange-300/40 bg-orange-400/15 text-orange-100" : "border-white/10 bg-white/8 text-slate-200"}`}
                    >
                      <span className="block font-semibold">{mode.label}</span>
                      <span className="block text-xs text-slate-400">{mode.kicker}</span>
                    </button>
                  ))}
                </div>
              </div>

              <div className="space-y-5">
                <form
                  onSubmit={(event) => {
                    event.preventDefault();
                    adminAction(() => postJson("/api/professor", { targetScore: Number(professorScore) }, token), "Professor target set");
                  }}
                  className="glass-panel rounded-lg p-4"
                >
                  <h2 className="text-sm font-semibold text-white">Professor Challenge</h2>
                  <input
                    inputMode="numeric"
                    value={professorScore}
                    onChange={(event) => setProfessorScore(event.target.value)}
                    className="mt-4 w-full rounded-md border border-white/10 bg-white/8 px-3 py-2 text-white"
                  />
                  <button type="submit" className="mt-3 inline-flex w-full items-center justify-center gap-2 rounded-md border border-orange-300/30 bg-orange-400/12 px-3 py-2 text-sm font-bold text-orange-100">
                    <FaUserTie aria-hidden />
                    Set Target
                  </button>
                </form>

                <form
                  onSubmit={(event) => {
                    event.preventDefault();
                    adminAction(() => postJson("/api/judges", { name: judgeName }, token), "Judge added");
                    setJudgeName("");
                  }}
                  className="glass-panel rounded-lg p-4"
                >
                  <h2 className="text-sm font-semibold text-white">Manage Judges</h2>
                  <input
                    required
                    value={judgeName}
                    onChange={(event) => setJudgeName(event.target.value)}
                    placeholder="Judge name"
                    className="mt-4 w-full rounded-md border border-white/10 bg-white/8 px-3 py-2 text-white"
                  />
                  <button type="submit" className="mt-3 inline-flex w-full items-center justify-center gap-2 rounded-md border border-cyan-300/30 bg-cyan-400/12 px-3 py-2 text-sm font-bold text-cyan-100">
                    <FaPlus aria-hidden />
                    Add Judge
                  </button>
                </form>
              </div>
            </div>

            <div className="grid gap-5 lg:grid-cols-3">
              <form
                onSubmit={(event) => {
                  event.preventDefault();
                  adminAction(() => postJson("/api/game/start", { gameName }, token), "Game started");
                }}
                className="glass-panel rounded-lg p-4"
              >
                <h2 className="text-sm font-semibold text-white">Current Game</h2>
                <select
                  value={gameName}
                  onChange={(event) => setGameName(event.target.value)}
                  className="mt-4 w-full rounded-md border border-white/10 bg-white/8 px-3 py-2 text-white"
                >
                  {games.map((game) => <option key={game}>{game}</option>)}
                </select>
                <button type="submit" className="mt-3 inline-flex w-full items-center justify-center gap-2 rounded-md bg-emerald-500 px-3 py-2 text-sm font-bold text-white">
                  <FaGamepad aria-hidden />
                  Start
                </button>
              </form>

              <form onSubmit={uploadLogo} className="glass-panel rounded-lg p-4">
                <h2 className="text-sm font-semibold text-white">College Logo</h2>
                <input
                  type="file"
                  accept="image/*"
                  onChange={(event) => setLogo(event.target.files?.[0] ?? null)}
                  className="mt-4 w-full rounded-md border border-white/10 bg-white/8 px-3 py-2 text-sm text-white"
                />
                <button type="submit" disabled={!logo} className="mt-3 inline-flex w-full items-center justify-center gap-2 rounded-md border border-white/15 bg-white/10 px-3 py-2 text-sm font-bold text-white disabled:opacity-60">
                  <FaImage aria-hidden />
                  Upload
                </button>
              </form>

              <div className="glass-panel rounded-lg p-4">
                <h2 className="text-sm font-semibold text-white">Reset Event</h2>
                <button
                  type="button"
                  onClick={() => {
                    if (window.confirm("Reset the event leaderboard?")) {
                      adminAction(() => postJson("/api/reset", {}, token), "Event reset");
                    }
                  }}
                  className="mt-4 inline-flex w-full items-center justify-center gap-2 rounded-md border border-rose-300/30 bg-rose-400/12 px-3 py-2 text-sm font-bold text-rose-100"
                >
                  <FaArrowRotateRight aria-hidden />
                  Reset Leaderboard
                </button>
                <button
                  type="button"
                  onClick={() => adminAction(() => postJson("/api/game/end", {}, token), "Game ended")}
                  className="mt-3 inline-flex w-full items-center justify-center gap-2 rounded-md border border-white/15 bg-white/10 px-3 py-2 text-sm font-bold text-white"
                >
                  <FaDownload aria-hidden />
                  End Game
                </button>
              </div>
            </div>

            <BranchBattleChart branches={state.branchBattle} />
            <LeaderboardTable players={state.leaderboard} limit={8} />

            <div className="glass-panel rounded-lg p-4">
              <h2 className="text-sm font-semibold text-white">Delete Player</h2>
              <div className="mt-4 grid gap-2 sm:grid-cols-2 lg:grid-cols-3">
                {state.leaderboard.slice(0, 9).map((player) => (
                  <button
                    key={player.id}
                    type="button"
                    onClick={() => adminAction(() => apiFetch(`/api/player/${player.id}`, { method: "DELETE", token }), "Player deleted")}
                    className="inline-flex min-w-0 items-center justify-between gap-2 rounded-md border border-white/10 bg-white/8 px-3 py-2 text-sm text-white"
                  >
                    <span className="truncate">{player.name}</span>
                    <FaTrash aria-hidden className="shrink-0 text-rose-200" />
                  </button>
                ))}
              </div>
            </div>
          </div>
        </div>
      </section>
    </Shell>
  );
}
