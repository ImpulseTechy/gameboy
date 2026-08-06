import clsx from "clsx";
import type { EventState } from "@/lib/types";

const toneClass: Record<string, string> = {
  success: "bg-emerald-300",
  warning: "bg-orange-300",
  danger: "bg-rose-300",
  info: "bg-cyan-300"
};

export function ActivityFeed({ activity }: { activity: EventState["activity"] }) {
  return (
    <div className="glass-panel rounded-lg p-4">
      <div className="mb-4 flex items-center justify-between">
        <h2 className="text-sm font-semibold text-white">Live Activity</h2>
        <span className="text-xs text-slate-400">{activity.length} signals</span>
      </div>
      <div className="space-y-3">
        {activity.length === 0 && <p className="text-sm text-slate-400">No activity yet.</p>}
        {activity.map((item) => (
          <div key={item.id} className="flex gap-3">
            <span className={clsx("mt-1 h-2.5 w-2.5 shrink-0 rounded-full", toneClass[item.tone] ?? toneClass.info)} />
            <div className="min-w-0">
              <p className="truncate text-sm font-medium text-slate-100">{item.title}</p>
              <p className="line-clamp-2 text-xs text-slate-400">{item.description}</p>
            </div>
          </div>
        ))}
      </div>
    </div>
  );
}
