import type { IconType } from "react-icons";
import { AnimatedNumber } from "./AnimatedNumber";

export function MetricCard({
  label,
  value,
  icon: Icon,
  accent,
  suffix
}: {
  label: string;
  value: string | number;
  icon: IconType;
  accent: "orange" | "cyan" | "purple" | "green" | "rose";
  suffix?: string;
}) {
  const color = {
    orange: "text-orange-200 bg-orange-400/12 border-orange-300/25",
    cyan: "text-cyan-200 bg-cyan-400/12 border-cyan-300/25",
    purple: "text-purple-200 bg-purple-400/12 border-purple-300/25",
    green: "text-emerald-200 bg-emerald-400/12 border-emerald-300/25",
    rose: "text-rose-200 bg-rose-400/12 border-rose-300/25"
  }[accent];

  return (
    <div className="glass-panel rounded-lg p-4">
      <div className="flex items-center justify-between gap-3">
        <div className={`flex h-10 w-10 items-center justify-center rounded-lg border ${color}`}>
          <Icon aria-hidden />
        </div>
        <span className="text-xs text-slate-300">{label}</span>
      </div>
      <div className="mt-4 truncate text-3xl font-black text-white">
        {typeof value === "number" ? (
          <AnimatedNumber value={value} suffix={suffix} />
        ) : (
          value
        )}
      </div>
    </div>
  );
}
