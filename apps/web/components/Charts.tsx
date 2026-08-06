"use client";

import {
  BarElement,
  CategoryScale,
  Chart as ChartJS,
  Filler,
  LinearScale,
  LineElement,
  PointElement,
  Tooltip
} from "chart.js";
import { Bar, Line } from "react-chartjs-2";
import type { EventState, PublicPlayer } from "@/lib/types";

ChartJS.register(CategoryScale, LinearScale, PointElement, LineElement, BarElement, Tooltip, Filler);

const chartOptions = {
  responsive: true,
  maintainAspectRatio: false,
  scales: {
    x: {
      grid: { color: "rgba(255,255,255,0.05)" },
      ticks: { color: "rgba(226,232,240,0.72)" }
    },
    y: {
      grid: { color: "rgba(255,255,255,0.06)" },
      ticks: { color: "rgba(226,232,240,0.72)" }
    }
  },
  plugins: {
    tooltip: {
      backgroundColor: "rgba(15,23,42,0.92)",
      borderColor: "rgba(255,255,255,0.16)",
      borderWidth: 1
    }
  }
};

export function BranchBattleChart({
  branches
}: {
  branches: EventState["branchBattle"];
}) {
  return (
    <div className="glass-panel h-72 rounded-lg p-4">
      <div className="mb-3 flex items-center justify-between">
        <h2 className="text-sm font-semibold text-white">Battle Of Branches</h2>
        <span className="text-xs text-slate-400">Average score</span>
      </div>
      <Bar
        data={{
          labels: branches.map((item) => item.branch),
          datasets: [
            {
              data: branches.map((item) => item.average),
              backgroundColor: [
                "rgba(34,211,238,0.72)",
                "rgba(249,115,22,0.72)",
                "rgba(168,85,247,0.72)",
                "rgba(34,197,94,0.72)",
                "rgba(251,113,133,0.72)"
              ],
              borderRadius: 6
            }
          ]
        }}
        options={chartOptions}
      />
    </div>
  );
}

export function ScoreHistoryChart({ player }: { player: PublicPlayer | null }) {
  const history = player?.history ?? [];
  return (
    <div className="glass-panel h-72 rounded-lg p-4">
      <div className="mb-3 flex items-center justify-between">
        <h2 className="text-sm font-semibold text-white">Score Signal</h2>
        <span className="text-xs text-slate-400">{player?.name ?? "No player"}</span>
      </div>
      <Line
        data={{
          labels: history.map((_, index) => `T${index + 1}`),
          datasets: [
            {
              data: history.map((item) => item.score),
              borderColor: "rgba(34,211,238,0.95)",
              backgroundColor: "rgba(34,211,238,0.14)",
              pointRadius: 3,
              tension: 0.35,
              fill: true
            }
          ]
        }}
        options={chartOptions}
      />
    </div>
  );
}
