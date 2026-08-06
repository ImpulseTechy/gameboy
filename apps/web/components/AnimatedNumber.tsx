"use client";

import { animate, motion, useMotionValue, useTransform } from "framer-motion";
import { useEffect } from "react";

export function AnimatedNumber({
  value,
  className,
  suffix = ""
}: {
  value: number;
  className?: string;
  suffix?: string;
}) {
  const motionValue = useMotionValue(value);
  const rounded = useTransform(motionValue, (latest) =>
    `${Math.round(latest).toLocaleString()}${suffix}`
  );

  useEffect(() => {
    const controls = animate(motionValue, value, {
      duration: 0.55,
      ease: "easeOut"
    });
    return controls.stop;
  }, [motionValue, value]);

  return <motion.span className={className}>{rounded}</motion.span>;
}
