"use client";

import clsx from "clsx";
import Link from "next/link";
import type { ReactNode } from "react";
import {
  FaChartSimple,
  FaGamepad,
  FaGaugeHigh,
  FaQrcode,
  FaShieldHalved,
  FaTowerBroadcast
} from "react-icons/fa6";
import { AnimatedBackdrop } from "./AnimatedBackdrop";

const navItems = [
  { href: "/", label: "Arena", icon: FaGamepad },
  { href: "/register", label: "Register", icon: FaQrcode },
  { href: "/playing", label: "Playing", icon: FaGaugeHigh },
  { href: "/leaderboard", label: "Leaderboard", icon: FaChartSimple },
  { href: "/big-screen", label: "Big Screen", icon: FaTowerBroadcast },
  { href: "/admin", label: "Admin", icon: FaShieldHalved }
];

export function Shell({
  children,
  active,
  compact = false
}: {
  children: ReactNode;
  active: string;
  compact?: boolean;
}) {
  return (
    <AnimatedBackdrop>
      <header className="mx-auto flex w-full max-w-7xl items-center justify-between gap-4 px-4 py-4 sm:px-6">
        <Link href="/" className="flex min-w-0 items-center gap-3">
          <span className="flex h-10 w-10 shrink-0 items-center justify-center rounded-lg border border-orange-300/30 bg-orange-500/20 text-orange-200 shadow-[0_0_28px_rgba(249,115,22,0.35)]">
            <FaGamepad />
          </span>
          <span className="min-w-0">
            <span className="block truncate text-sm font-semibold text-white">NST Arcade Live</span>
            <span className="block truncate text-xs text-slate-300">Where Engineering Meets Gaming</span>
          </span>
        </Link>

        {!compact && (
          <nav className="hidden items-center gap-2 lg:flex">
            {navItems.map((item) => {
              const Icon = item.icon;
              return (
                <Link
                  key={item.href}
                  href={item.href}
                  className={clsx(
                    "flex items-center gap-2 rounded-md border px-3 py-2 text-sm transition",
                    active === item.href
                      ? "border-cyan-300/50 bg-cyan-300/15 text-cyan-100"
                      : "border-white/10 bg-white/5 text-slate-300 hover:border-white/25 hover:bg-white/10"
                  )}
                >
                  <Icon aria-hidden />
                  {item.label}
                </Link>
              );
            })}
          </nav>
        )}
      </header>
      <main className="mx-auto w-full max-w-7xl px-4 pb-10 sm:px-6">{children}</main>
    </AnimatedBackdrop>
  );
}
