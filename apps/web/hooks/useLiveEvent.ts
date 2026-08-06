"use client";

import { useCallback, useEffect, useMemo, useRef, useState } from "react";
import { io, type Socket } from "socket.io-client";
import { API_URL, getEventState } from "@/lib/api";
import type { EventState, ToastMessage } from "@/lib/types";

const emptyState: EventState = {
  generatedAt: new Date().toISOString(),
  leaderboard: [],
  stats: {
    participants: 0,
    highestScore: 0,
    currentGame: "Snake",
    champion: "Waiting for player",
    wifiStatus: "No packets",
    esp32Connected: false,
    averageScore: 0,
    topStreak: 0
  },
  currentPlayer: null,
  nextPlayer: null,
  gameModes: [],
  currentMode: "classic",
  currentChallenge: null,
  lastMystery: null,
  mysteryEvents: [],
  branchBattle: [],
  teamBattle: [],
  telemetry: null,
  activity: [],
  settings: {
    theme: "dark",
    collegeLogo: null
  }
};

function isState(payload: unknown): payload is EventState {
  return Boolean(
    payload &&
      typeof payload === "object" &&
      Array.isArray((payload as EventState).leaderboard) &&
      (payload as EventState).stats
  );
}

function playPulse() {
  const AudioContextCtor =
    window.AudioContext || (window as unknown as { webkitAudioContext?: typeof AudioContext }).webkitAudioContext;
  if (!AudioContextCtor) return;
  const context = new AudioContextCtor();
  const oscillator = context.createOscillator();
  const gain = context.createGain();
  oscillator.type = "triangle";
  oscillator.frequency.value = 660;
  gain.gain.setValueAtTime(0.0001, context.currentTime);
  gain.gain.exponentialRampToValueAtTime(0.05, context.currentTime + 0.02);
  gain.gain.exponentialRampToValueAtTime(0.0001, context.currentTime + 0.16);
  oscillator.connect(gain).connect(context.destination);
  oscillator.start();
  oscillator.stop(context.currentTime + 0.18);
}

export function useLiveEvent() {
  const [state, setState] = useState<EventState>(emptyState);
  const [loading, setLoading] = useState(true);
  const [connected, setConnected] = useState(false);
  const [toasts, setToasts] = useState<ToastMessage[]>([]);
  const socketRef = useRef<Socket | null>(null);

  const pushToast = useCallback((toast: Omit<ToastMessage, "id">) => {
    const id = crypto.randomUUID();
    setToasts((current) => [{ id, ...toast }, ...current].slice(0, 4));
    window.setTimeout(() => {
      setToasts((current) => current.filter((item) => item.id !== id));
    }, 4200);
  }, []);

  const refresh = useCallback(async () => {
    const nextState = await getEventState();
    setState(nextState);
    setLoading(false);
    return nextState;
  }, []);

  useEffect(() => {
    let active = true;
    refresh().catch((error) => {
      if (!active) return;
      setLoading(false);
      pushToast({
        title: "API unreachable",
        body: error.message,
        tone: "danger"
      });
    });

    const socket = io(API_URL, {
      transports: ["websocket", "polling"],
      reconnection: true
    });
    socketRef.current = socket;

    const updateFromPayload = (payload: unknown) => {
      if (isState(payload)) {
        setState(payload);
      } else {
        refresh().catch(() => undefined);
      }
    };

    socket.on("connect", () => setConnected(true));
    socket.on("disconnect", () => setConnected(false));
    socket.on("leaderboard-updated", updateFromPayload);
    socket.on("telemetry-updated", (payload) => {
      updateFromPayload(payload?.state ?? payload);
      playPulse();
    });
    socket.on("score-added", (payload) => {
      updateFromPayload(payload?.state ?? payload);
      playPulse();
      const player = payload?.player;
      if (player?.name) {
        pushToast({
          title: "Score uploaded",
          body: `${player.name} hit ${player.score}`,
          tone: "success"
        });
      }
    });
    socket.on("winner-announced", (payload) => {
      const player = payload?.player;
      pushToast({
        title: payload?.title ?? "Winner announced",
        body: player?.name ? `${player.name} is lighting up the board` : "The arena has a new moment",
        tone: "success"
      });
    });
    socket.on("mystery-event", (payload) => {
      pushToast({
        title: "Mystery wheel",
        body: payload?.title ?? "A new twist is live",
        tone: "warning"
      });
      refresh().catch(() => undefined);
    });
    socket.on("player-connected", () => {
      refresh().catch(() => undefined);
    });

    return () => {
      active = false;
      socket.disconnect();
    };
  }, [pushToast, refresh]);

  const mode = useMemo(
    () =>
      state.gameModes.find((item) => item.id === state.currentMode) ??
      state.gameModes[0],
    [state.currentMode, state.gameModes]
  );

  return {
    state,
    mode,
    loading,
    connected,
    toasts,
    refresh,
    pushToast
  };
}
