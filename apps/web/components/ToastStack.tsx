"use client";

import { AnimatePresence, motion } from "framer-motion";
import type { ToastMessage } from "@/lib/types";

export function ToastStack({ toasts }: { toasts: ToastMessage[] }) {
  return (
    <div className="fixed right-4 top-4 z-50 flex w-[min(360px,calc(100vw-2rem))] flex-col gap-2">
      <AnimatePresence>
        {toasts.map((toast) => (
          <motion.div
            key={toast.id}
            className="glass-panel rounded-lg p-3"
            initial={{ opacity: 0, x: 30, scale: 0.98 }}
            animate={{ opacity: 1, x: 0, scale: 1 }}
            exit={{ opacity: 0, x: 30, scale: 0.98 }}
          >
            <p className="text-sm font-semibold text-white">{toast.title}</p>
            <p className="mt-1 text-xs text-slate-300">{toast.body}</p>
          </motion.div>
        ))}
      </AnimatePresence>
    </div>
  );
}
