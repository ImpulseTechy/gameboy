import clsx from "clsx";
import type { ReactNode } from "react";

const toneClass = {
  green: "border-emerald-300/30 bg-emerald-400/12 text-emerald-100",
  orange: "border-orange-300/30 bg-orange-400/12 text-orange-100",
  cyan: "border-cyan-300/30 bg-cyan-400/12 text-cyan-100",
  rose: "border-rose-300/30 bg-rose-400/12 text-rose-100",
  slate: "border-white/15 bg-white/8 text-slate-100"
};

export function StatusPill({
  children,
  tone = "slate"
}: {
  children: ReactNode;
  tone?: keyof typeof toneClass;
}) {
  return (
    <span className={clsx("inline-flex items-center gap-2 rounded-md border px-2.5 py-1 text-xs font-medium", toneClass[tone])}>
      {children}
    </span>
  );
}
