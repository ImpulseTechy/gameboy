"use client";

import { QRCodeCanvas } from "qrcode.react";

export function QrJoin({ path = "/register" }: { path?: string }) {
  const origin =
    typeof window === "undefined" ? "http://localhost:3000" : window.location.origin;
  const value = `${origin}${path}`;

  return (
    <div className="glass-panel rounded-lg p-4">
      <div className="flex items-center gap-4">
        <div className="rounded-lg bg-white p-2">
          <QRCodeCanvas value={value} size={112} marginSize={1} />
        </div>
        <div className="min-w-0">
          <p className="text-sm font-semibold text-white">Scan To Join</p>
          <p className="mt-1 text-xs text-slate-300">Registration opens directly on student phones.</p>
          <p className="mt-3 truncate text-xs text-cyan-200">{value}</p>
        </div>
      </div>
    </div>
  );
}
