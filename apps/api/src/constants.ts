export const branches = [
  "Computer",
  "AI",
  "Electrical",
  "Electronics",
  "Mechanical"
];

export const games = ["Snake", "Tetris"];

export const gameModes = [
  {
    id: "classic",
    label: "Classic",
    kicker: "Highest score wins",
    scoring: "score-desc"
  },
  {
    id: "nst-got-talent",
    label: "NST Got Talent",
    kicker: "Judges predict the final score",
    scoring: "difference-asc"
  },
  {
    id: "lucky-score",
    label: "Lucky Score",
    kicker: "Exact prediction earns +50",
    scoring: "score-desc"
  },
  {
    id: "beat-professor",
    label: "Beat The Professor",
    kicker: "Cross the professor target",
    scoring: "score-desc"
  },
  {
    id: "mystery-wheel",
    label: "Mystery Wheel",
    kicker: "Spin before the attempt",
    scoring: "score-desc"
  },
  {
    id: "last-man-standing",
    label: "Last Man Standing",
    kicker: "Top 10 survive each round",
    scoring: "score-desc"
  },
  {
    id: "battle-branches",
    label: "Battle Of Branches",
    kicker: "Best branch average wins",
    scoring: "branch-average"
  },
  {
    id: "team-battle",
    label: "Team Battle",
    kicker: "Random teams by average",
    scoring: "team-average"
  }
];

export const defaultMysteryEvents = [
  {
    title: "Double Score",
    description: "The next score is doubled for instant drama.",
    multiplier: 2,
    points: 0
  },
  {
    title: "Half Score",
    description: "A brutal twist: only half the score counts.",
    multiplier: 0.5,
    points: 0
  },
  {
    title: "+20",
    description: "Bonus engineering luck: add 20 points.",
    multiplier: 1,
    points: 20
  },
  {
    title: "+50",
    description: "The auditorium wakes up: add 50 points.",
    multiplier: 1,
    points: 50
  },
  {
    title: "Reverse Controls",
    description: "Controls are flipped for the next attempt.",
    multiplier: 1,
    points: 0
  },
  {
    title: "Speed Mode",
    description: "Game speed increases and every move matters.",
    multiplier: 1,
    points: 0
  },
  {
    title: "Sudden Death",
    description: "One mistake ends the run.",
    multiplier: 1,
    points: 0
  },
  {
    title: "Golden Apple",
    description: "A rare item unlocks a flashy bonus round.",
    multiplier: 1,
    points: 35
  },
  {
    title: "Lucky Bonus",
    description: "Audience energy converts into surprise points.",
    multiplier: 1,
    points: 15
  },
  {
    title: "Nothing",
    description: "Pure skill. No boost, no penalty.",
    multiplier: 1,
    points: 0
  }
];

export const defaultJudges = ["Circuit Sir", "Bug Fix Ma'am", "Pixel Professor"];

export const samplePlayers = [
  {
    name: "Rahul Verma",
    branch: "Computer",
    rollNumber: "NST24C011",
    game: "Snake",
    score: 88,
    prediction: 90,
    level: 5,
    battery: 91,
    wifi: -58,
    fps: 30,
    heap: 182000,
    badges: ["Fast Starter", "Signal Boss"]
  },
  {
    name: "Aisha Khan",
    branch: "AI",
    rollNumber: "NST24A017",
    game: "Tetris",
    score: 104,
    prediction: 100,
    level: 7,
    battery: 82,
    wifi: -64,
    fps: 29,
    heap: 176400,
    badges: ["Century Run", "Stack Master"]
  },
  {
    name: "Meera Iyer",
    branch: "Electronics",
    rollNumber: "NST24E028",
    game: "Snake",
    score: 67,
    prediction: 70,
    level: 4,
    battery: 96,
    wifi: -53,
    fps: 31,
    heap: 188220,
    badges: ["Charged Up"]
  },
  {
    name: "Kabir Singh",
    branch: "Mechanical",
    rollNumber: "NST24M009",
    game: "Tetris",
    score: 121,
    prediction: 115,
    level: 8,
    battery: 73,
    wifi: -69,
    fps: 28,
    heap: 171500,
    badges: ["Hall of Fame", "Top Streak"]
  }
];
