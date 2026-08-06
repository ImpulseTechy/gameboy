export type PublicPlayer = {
  id: string;
  playerCode: string;
  name: string;
  branch: string;
  rollNumber: string | null;
  game: string;
  score: number;
  avatar: string | null;
  prediction: number | null;
  difference: number | null;
  rank: number;
  level: number;
  battery: number;
  wifi: number;
  fps: number;
  heap: number;
  streak: number;
  team: string | null;
  badges: string[];
  judgeAverage: number | null;
  trend: number;
  createdAt: string;
  updatedAt: string;
  history: { score: number; timestamp: string }[];
  judgePredictions: {
    judgeId: string;
    judgeName: string;
    prediction: number;
  }[];
};

export type GameMode = {
  id: string;
  label: string;
  kicker: string;
  scoring: string;
};

export type MysteryEvent = {
  id: string;
  title: string;
  description: string;
  multiplier: number;
  points: number;
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
  gameModes: GameMode[];
  currentMode: string;
  currentChallenge: { id: string; targetScore: number } | null;
  lastMystery: MysteryEvent | null;
  mysteryEvents: MysteryEvent[];
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

export type ToastMessage = {
  id: string;
  title: string;
  body: string;
  tone: "info" | "success" | "warning" | "danger";
};
