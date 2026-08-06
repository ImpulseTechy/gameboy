import type { Player, Prisma } from "@prisma/client";
import { branches, gameModes, games } from "./constants";
import { prisma } from "./db";

type PlayerWithRelations = Prisma.PlayerGetPayload<{
  include: {
    judgePredictions: { include: { judge: true } };
    history: { orderBy: { timestamp: "desc" }; take: 20 };
  };
}>;

export type PublicPlayer = Omit<
  Player,
  "badges" | "createdAt" | "updatedAt" | "isActive"
> & {
  badges: string[];
  judgeAverage: number | null;
  rank: number;
  trend: number;
  updatedAt: string;
  createdAt: string;
  history: { score: number; timestamp: string }[];
  judgePredictions: {
    judgeId: string;
    judgeName: string;
    prediction: number;
  }[];
};

export type EventState = {
  generatedAt: string;
  leaderboard: PublicPlayer[];
  stats: {
    participants: number;
    highestScore: number;
    currentGame: string;
    champion: string;
    wifiStatus: string;
    esp32Connected: boolean;
    averageScore: number;
    topStreak: number;
  };
  currentPlayer: PublicPlayer | null;
  nextPlayer: PublicPlayer | null;
  gameModes: typeof gameModes;
  currentMode: string;
  currentChallenge: { id: string; targetScore: number } | null;
  lastMystery: {
    id: string;
    title: string;
    description: string;
    multiplier: number;
    points: number;
  } | null;
  mysteryEvents: {
    id: string;
    title: string;
    description: string;
    multiplier: number;
    points: number;
  }[];
  branchBattle: {
    branch: string;
    average: number;
    total: number;
    participants: number;
  }[];
  teamBattle: {
    team: string;
    average: number;
    total: number;
    participants: number;
  }[];
  telemetry: {
    name: string;
    game: string;
    score: number;
    level: number;
    battery: number;
    wifi: number;
    fps: number;
    heap: number;
    createdAt: string;
  } | null;
  activity: {
    id: string;
    title: string;
    description: string;
    tone: string;
    createdAt: string;
  }[];
  settings: {
    theme: string;
    collegeLogo: string | null;
  };
};

export function parseBadges(value: string | null | undefined): string[] {
  if (!value) return [];
  try {
    const parsed = JSON.parse(value);
    return Array.isArray(parsed) ? parsed.filter(Boolean).map(String) : [];
  } catch {
    return [];
  }
}

export function stringifyBadges(badges: string[]): string {
  return JSON.stringify([...new Set(badges.filter(Boolean))]);
}

export function buildPlayerCode(): string {
  const suffix = Math.random().toString(36).slice(2, 6).toUpperCase();
  const stamp = Date.now().toString(36).slice(-4).toUpperCase();
  return `NST-${stamp}-${suffix}`;
}

export function assignTeam(branch: string): string {
  const names = ["Team Byte", "Team Spark", "Team Torque", "Team Vector"];
  const index = Math.abs(
    branch.split("").reduce((total, char) => total + char.charCodeAt(0), 0)
  );
  return names[index % names.length];
}

export function applyBadges(input: {
  score: number;
  battery?: number | null;
  wifi?: number | null;
  difference?: number | null;
  beatProfessor?: boolean;
  luckyExact?: boolean;
  previous?: string[];
}): string[] {
  const badges = new Set(input.previous ?? []);
  if (input.score >= 100) badges.add("Century Run");
  if (input.score >= 150) badges.add("Hall of Fame");
  if ((input.battery ?? 0) >= 85) badges.add("Charged Up");
  if ((input.wifi ?? -100) >= -60) badges.add("Signal Boss");
  if ((input.difference ?? 999) <= 5) badges.add("Talent Radar");
  if (input.beatProfessor) badges.add("Professor Slayer");
  if (input.luckyExact) badges.add("Perfect Prediction");
  return [...badges];
}

export async function recordActivity(
  title: string,
  description: string,
  tone: "info" | "success" | "warning" | "danger" = "info"
) {
  await prisma.activity.create({
    data: { title, description, tone }
  });

  const oldActivity = await prisma.activity.findMany({
    orderBy: { createdAt: "desc" },
    skip: 80,
    select: { id: true }
  });

  if (oldActivity.length > 0) {
    await prisma.activity.deleteMany({
      where: { id: { in: oldActivity.map((item) => item.id) } }
    });
  }
}

function normalizePlayer(player: PlayerWithRelations, rank: number): PublicPlayer {
  const sortedHistory = [...player.history].sort(
    (a, b) => a.timestamp.getTime() - b.timestamp.getTime()
  );
  const previousScore =
    sortedHistory.length > 1
      ? sortedHistory[sortedHistory.length - 2]?.score ?? player.score
      : player.score;
  const judgeValues = player.judgePredictions.map((item) => item.prediction);
  const judgeAverage =
    judgeValues.length > 0
      ? Math.round(
          judgeValues.reduce((total, value) => total + value, 0) /
            judgeValues.length
        )
      : null;

  return {
    id: player.id,
    playerCode: player.playerCode,
    name: player.name,
    branch: player.branch,
    rollNumber: player.rollNumber,
    game: player.game,
    score: player.score,
    avatar: player.avatar,
    prediction: player.prediction,
    difference:
      player.difference ??
      (judgeAverage === null ? null : Math.abs(player.score - judgeAverage)),
    rank,
    level: player.level,
    battery: player.battery,
    wifi: player.wifi,
    fps: player.fps,
    heap: player.heap,
    streak: player.streak,
    team: player.team,
    badges: parseBadges(player.badges),
    judgeAverage,
    trend: player.score - previousScore,
    createdAt: player.createdAt.toISOString(),
    updatedAt: player.updatedAt.toISOString(),
    history: sortedHistory.map((item) => ({
      score: item.score,
      timestamp: item.timestamp.toISOString()
    })),
    judgePredictions: player.judgePredictions.map((item) => ({
      judgeId: item.judgeId,
      judgeName: item.judge.name,
      prediction: item.prediction
    }))
  };
}

function summarizeTeamGroups(players: PublicPlayer[]) {
  const groups = new Map<string, { total: number; participants: number }>();

  for (const player of players) {
    const name = player.team || "Unassigned";
    const current = groups.get(name) ?? { total: 0, participants: 0 };
    current.total += player.score;
    current.participants += 1;
    groups.set(name, current);
  }

  return [...groups.entries()]
    .map(([name, value]) => ({
      team: name,
      average:
        value.participants === 0
          ? 0
          : Math.round(value.total / value.participants),
      total: value.total,
      participants: value.participants
    }))
    .sort((a, b) => b.average - a.average);
}

export async function getRankedLeaderboard(): Promise<PublicPlayer[]> {
  const players = await prisma.player.findMany({
    where: { isActive: true },
    include: {
      judgePredictions: { include: { judge: true } },
      history: { orderBy: { timestamp: "desc" }, take: 20 }
    },
    orderBy: [{ score: "desc" }, { createdAt: "asc" }]
  });

  const ranked = players.map((player, index) =>
    normalizePlayer(player, index + 1)
  );

  await Promise.all(
    ranked
      .filter((player) => player.rank !== players.find((p) => p.id === player.id)?.rank)
      .map((player) =>
        prisma.player.update({
          where: { id: player.id },
          data: { rank: player.rank }
        })
      )
  );

  return ranked;
}

async function getSetting(key: string, fallback: string) {
  const setting = await prisma.setting.findUnique({ where: { key } });
  return setting?.value ?? fallback;
}

export async function setSetting(key: string, value: string) {
  return prisma.setting.upsert({
    where: { key },
    create: { key, value },
    update: { value }
  });
}

export async function buildEventState(): Promise<EventState> {
  const [
    leaderboard,
    liveGame,
    latestGame,
    professor,
    mysteryEvents,
    latestTelemetry,
    activity,
    currentMode,
    theme,
    collegeLogo,
    lastMysteryRaw
  ] = await Promise.all([
    getRankedLeaderboard(),
    prisma.game.findFirst({
      where: { status: "live" },
      orderBy: { startTime: "desc" }
    }),
    prisma.game.findFirst({ orderBy: { createdAt: "desc" } }),
    prisma.professorChallenge.findFirst({
      where: { active: true },
      orderBy: { createdAt: "desc" }
    }),
    prisma.mysteryEvent.findMany({ orderBy: { title: "asc" } }),
    prisma.telemetry.findFirst({ orderBy: { createdAt: "desc" } }),
    prisma.activity.findMany({
      orderBy: { createdAt: "desc" },
      take: 14
    }),
    getSetting("currentMode", "classic"),
    getSetting("theme", "dark"),
    getSetting("collegeLogo", ""),
    getSetting("lastMystery", "")
  ]);

  const highestScore = leaderboard[0]?.score ?? 0;
  const champion = leaderboard[0]?.name ?? "Waiting for player";
  const averageScore =
    leaderboard.length === 0
      ? 0
      : Math.round(
          leaderboard.reduce((total, player) => total + player.score, 0) /
            leaderboard.length
        );
  const currentPlayer = [...leaderboard].sort(
    (a, b) => new Date(b.updatedAt).getTime() - new Date(a.updatedAt).getTime()
  )[0];
  const nextPlayer =
    leaderboard.find((player) => player.id !== currentPlayer?.id) ?? null;

  let lastMystery = null;
  if (lastMysteryRaw) {
    try {
      lastMystery = JSON.parse(lastMysteryRaw);
    } catch {
      lastMystery = null;
    }
  }

  const currentGame =
    liveGame?.gameName ?? latestGame?.gameName ?? currentPlayer?.game ?? games[0];
  const esp32Connected =
    latestTelemetry !== null &&
    Date.now() - latestTelemetry.createdAt.getTime() < 45_000;
  const wifiStatus =
    latestTelemetry === null
      ? "No packets"
      : latestTelemetry.wifi >= -60
        ? "Excellent"
        : latestTelemetry.wifi >= -72
          ? "Stable"
          : "Weak";

  return {
    generatedAt: new Date().toISOString(),
    leaderboard,
    stats: {
      participants: leaderboard.length,
      highestScore,
      currentGame,
      champion,
      wifiStatus,
      esp32Connected,
      averageScore,
      topStreak: Math.max(0, ...leaderboard.map((player) => player.streak))
    },
    currentPlayer: currentPlayer ?? null,
    nextPlayer,
    gameModes,
    currentMode,
    currentChallenge: professor
      ? { id: professor.id, targetScore: professor.targetScore }
      : null,
    lastMystery,
    mysteryEvents,
    branchBattle: branches.map((branch) => {
      const players = leaderboard.filter((player) => player.branch === branch);
      const total = players.reduce((sum, player) => sum + player.score, 0);
      return {
        branch,
        average: players.length === 0 ? 0 : Math.round(total / players.length),
        total,
        participants: players.length
      };
    }),
    teamBattle: summarizeTeamGroups(leaderboard).map((item) => ({
      team: item.team,
      average: item.average,
      total: item.total,
      participants: item.participants
    })),
    telemetry: latestTelemetry
      ? {
          name: latestTelemetry.name,
          game: latestTelemetry.game,
          score: latestTelemetry.score,
          level: latestTelemetry.level,
          battery: latestTelemetry.battery,
          wifi: latestTelemetry.wifi,
          fps: latestTelemetry.fps,
          heap: latestTelemetry.heap,
          createdAt: latestTelemetry.createdAt.toISOString()
        }
      : null,
    activity: activity.map((item) => ({
      id: item.id,
      title: item.title,
      description: item.description,
      tone: item.tone,
      createdAt: item.createdAt.toISOString()
    })),
    settings: {
      theme,
      collegeLogo: collegeLogo || null
    }
  };
}
