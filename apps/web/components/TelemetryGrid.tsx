import {
  FaBatteryThreeQuarters,
  FaGaugeHigh,
  FaMicrochip,
  FaSignal,
  FaStopwatch
} from "react-icons/fa6";
import type { EventState, PublicPlayer } from "@/lib/types";
import { AnimatedNumber } from "./AnimatedNumber";

const items = [
  { key: "level", label: "Level", icon: FaGaugeHigh, suffix: "" },
  { key: "battery", label: "Battery", icon: FaBatteryThreeQuarters, suffix: "%" },
  { key: "wifi", label: "WiFi RSSI", icon: FaSignal, suffix: " dBm" },
  { key: "fps", label: "ESP32 FPS", icon: FaStopwatch, suffix: "" },
  { key: "heap", label: "Free Heap", icon: FaMicrochip, suffix: " B" }
] as const;

export function TelemetryGrid({
  player,
  telemetry
}: {
  player: PublicPlayer | null;
  telemetry: EventState["telemetry"];
}) {
  const source = player ?? telemetry;

  return (
    <div className="grid gap-3 sm:grid-cols-2 xl:grid-cols-5">
      {items.map((item) => {
        const Icon = item.icon;
        const value = Number(source?.[item.key] ?? 0);
        return (
          <div key={item.key} className="glass-panel rounded-lg p-4">
            <div className="flex items-center justify-between text-slate-300">
              <span className="text-xs">{item.label}</span>
              <Icon aria-hidden className="text-cyan-200" />
            </div>
            <div className="mt-3 text-2xl font-black text-white">
              <AnimatedNumber value={value} suffix={item.suffix} />
            </div>
          </div>
        );
      })}
    </div>
  );
}
