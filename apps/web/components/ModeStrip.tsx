import clsx from "clsx";
import type { GameMode } from "@/lib/types";

export function ModeStrip({
  modes,
  currentMode
}: {
  modes: GameMode[];
  currentMode: string;
}) {
  return (
    <div className="grid gap-3 sm:grid-cols-2 lg:grid-cols-4">
      {modes.map((mode) => (
        <div
          key={mode.id}
          className={clsx(
            "rounded-lg border p-3",
            mode.id === currentMode
              ? "border-orange-300/50 bg-orange-400/15"
              : "border-white/10 bg-white/5"
          )}
        >
          <p className="text-sm font-semibold text-white">{mode.label}</p>
          <p className="mt-1 text-xs text-slate-300">{mode.kicker}</p>
        </div>
      ))}
    </div>
  );
}
