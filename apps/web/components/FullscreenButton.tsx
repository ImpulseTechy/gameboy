"use client";

import { FaExpand } from "react-icons/fa6";

export function FullscreenButton({ label = "Fullscreen" }: { label?: string }) {
  return (
    <button
      type="button"
      onClick={() => document.documentElement.requestFullscreen?.()}
      className="inline-flex items-center gap-2 rounded-md border border-white/15 bg-white/10 px-3 py-2 text-sm font-semibold text-white hover:bg-white/15"
    >
      <FaExpand aria-hidden />
      {label}
    </button>
  );
}
