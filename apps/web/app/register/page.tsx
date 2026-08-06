"use client";

import { FormEvent, useState } from "react";
import { FaGamepad, FaIdBadge, FaPaperPlane } from "react-icons/fa6";
import { postJson } from "@/lib/api";
import type { PublicPlayer } from "@/lib/types";
import { QrJoin } from "@/components/QrJoin";
import { Shell } from "@/components/Shell";
import { StatusPill } from "@/components/StatusPill";

const branches = ["Computer", "AI", "Electrical", "Electronics", "Mechanical"];
const games = ["Snake", "Tetris"];

export default function RegisterPage() {
  const [form, setForm] = useState({
    name: "",
    branch: branches[0],
    rollNumber: "",
    game: games[0],
    prediction: ""
  });
  const [player, setPlayer] = useState<PublicPlayer | null>(null);
  const [submitting, setSubmitting] = useState(false);
  const [error, setError] = useState("");

  async function onSubmit(event: FormEvent<HTMLFormElement>) {
    event.preventDefault();
    setSubmitting(true);
    setError("");
    try {
      const response = await postJson<{ player: PublicPlayer }>("/api/register", {
        ...form,
        prediction: form.prediction ? Number(form.prediction) : undefined
      });
      setPlayer(response.player);
      setForm((current) => ({ ...current, name: "", rollNumber: "", prediction: "" }));
    } catch (err) {
      setError(err instanceof Error ? err.message : "Registration failed");
    } finally {
      setSubmitting(false);
    }
  }

  return (
    <Shell active="/register">
      <section className="grid gap-6 py-10 lg:grid-cols-[0.95fr_1.05fr] lg:items-start">
        <div>
          <StatusPill tone="orange">
            <FaIdBadge aria-hidden />
            Student Registration
          </StatusPill>
          <h1 className="mt-5 text-5xl font-black leading-none text-white sm:text-6xl">Ready Room</h1>
          <p className="mt-4 max-w-xl text-slate-300">
            Lock in the player profile before the handheld console starts streaming.
          </p>
          <div className="mt-6">
            <QrJoin />
          </div>
        </div>

        <form onSubmit={onSubmit} className="glass-panel rounded-lg p-5">
          <div className="grid gap-4 sm:grid-cols-2">
            <label className="sm:col-span-2">
              <span className="text-sm text-slate-300">Name</span>
              <input
                required
                minLength={2}
                value={form.name}
                onChange={(event) => setForm({ ...form, name: event.target.value })}
                className="mt-2 w-full rounded-md border border-white/10 bg-white/8 px-3 py-3 text-white"
                placeholder="Rahul Verma"
              />
            </label>
            <label>
              <span className="text-sm text-slate-300">Branch</span>
              <select
                value={form.branch}
                onChange={(event) => setForm({ ...form, branch: event.target.value })}
                className="mt-2 w-full rounded-md border border-white/10 bg-white/8 px-3 py-3 text-white"
              >
                {branches.map((branch) => (
                  <option key={branch}>{branch}</option>
                ))}
              </select>
            </label>
            <label>
              <span className="text-sm text-slate-300">Roll Number</span>
              <input
                value={form.rollNumber}
                onChange={(event) => setForm({ ...form, rollNumber: event.target.value })}
                className="mt-2 w-full rounded-md border border-white/10 bg-white/8 px-3 py-3 text-white"
                placeholder="NST24C011"
              />
            </label>
            <label>
              <span className="text-sm text-slate-300">Game</span>
              <select
                value={form.game}
                onChange={(event) => setForm({ ...form, game: event.target.value })}
                className="mt-2 w-full rounded-md border border-white/10 bg-white/8 px-3 py-3 text-white"
              >
                {games.map((game) => (
                  <option key={game}>{game}</option>
                ))}
              </select>
            </label>
            <label>
              <span className="text-sm text-slate-300">Score Prediction</span>
              <input
                inputMode="numeric"
                value={form.prediction}
                onChange={(event) => setForm({ ...form, prediction: event.target.value })}
                className="mt-2 w-full rounded-md border border-white/10 bg-white/8 px-3 py-3 text-white"
                placeholder="88"
              />
            </label>
          </div>

          {error && <p className="mt-4 rounded-md border border-rose-300/30 bg-rose-400/12 p-3 text-sm text-rose-100">{error}</p>}

          <button
            type="submit"
            disabled={submitting}
            className="mt-5 inline-flex w-full items-center justify-center gap-2 rounded-md bg-orange-500 px-5 py-3 text-sm font-black text-white disabled:opacity-60"
          >
            <FaPaperPlane aria-hidden />
            {submitting ? "Registering" : "Ready To Play"}
          </button>
        </form>
      </section>

      {player && (
        <section className="neon-border glass-panel mb-8 rounded-lg p-5">
          <div className="flex flex-col gap-4 sm:flex-row sm:items-center sm:justify-between">
            <div>
              <p className="text-sm text-cyan-100">Player ID Generated</p>
              <h2 className="mt-1 text-4xl font-black text-white">{player.playerCode}</h2>
              <p className="mt-2 text-slate-300">{player.name} / {player.branch} / {player.game}</p>
            </div>
            <div className="flex h-20 w-20 items-center justify-center rounded-lg border border-orange-300/30 bg-orange-400/15 text-3xl text-orange-100">
              <FaGamepad aria-hidden />
            </div>
          </div>
        </section>
      )}
    </Shell>
  );
}
