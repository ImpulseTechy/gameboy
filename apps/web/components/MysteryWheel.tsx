"use client";

import { motion } from "framer-motion";
import { FaWandMagicSparkles } from "react-icons/fa6";
import type { MysteryEvent } from "@/lib/types";

export function MysteryWheel({
  events,
  spinning,
  selected,
  onSpin
}: {
  events: MysteryEvent[];
  spinning: boolean;
  selected: MysteryEvent | null;
  onSpin: () => void;
}) {
  const colors = ["#f97316", "#22d3ee", "#a855f7", "#22c55e", "#fb7185", "#facc15"];
  const slice = 360 / Math.max(events.length, 1);

  return (
    <div className="glass-panel rounded-lg p-4">
      <div className="flex flex-col items-center gap-5 md:flex-row">
        <div className="relative h-64 w-64 shrink-0">
          <motion.div
            className="absolute inset-0 rounded-full border border-white/20 shadow-[0_0_50px_rgba(34,211,238,0.25)]"
            animate={{ rotate: spinning ? 1080 : selected ? 180 : 0 }}
            transition={{ duration: spinning ? 1.6 : 0.5, ease: "easeOut" }}
            style={{
              background: `conic-gradient(${events
                .map((event, index) => `${colors[index % colors.length]} ${index * slice}deg ${(index + 1) * slice}deg`)
                .join(",")})`
            }}
          />
          <div className="absolute inset-8 rounded-full border border-black/30 bg-slate-950/80" />
          <div className="absolute inset-0 flex items-center justify-center text-center">
            <div>
              <FaWandMagicSparkles className="mx-auto text-3xl text-white" aria-hidden />
              <p className="mt-2 text-sm font-black text-white">MYSTERY</p>
            </div>
          </div>
          <div className="absolute left-1/2 top-[-8px] h-0 w-0 -translate-x-1/2 border-x-[12px] border-t-[22px] border-x-transparent border-t-white" />
        </div>
        <div className="min-w-0 flex-1">
          <p className="text-sm text-slate-300">Current twist</p>
          <h2 className="mt-1 text-3xl font-black text-white">{selected?.title ?? "Spin The Wheel"}</h2>
          <p className="mt-3 text-sm text-slate-300">{selected?.description ?? "Pick a random event before the next attempt."}</p>
          <button
            type="button"
            onClick={onSpin}
            disabled={spinning}
            className="mt-5 inline-flex items-center gap-2 rounded-md border border-orange-300/30 bg-orange-500 px-4 py-2 text-sm font-semibold text-white shadow-[0_0_24px_rgba(249,115,22,0.35)] disabled:cursor-not-allowed disabled:opacity-60"
          >
            <FaWandMagicSparkles aria-hidden />
            Spin Mystery Wheel
          </button>
        </div>
      </div>
    </div>
  );
}
