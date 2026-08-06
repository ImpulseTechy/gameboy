CREATE TABLE "Players" (
    "id" TEXT NOT NULL PRIMARY KEY,
    "playerCode" TEXT NOT NULL,
    "name" TEXT NOT NULL,
    "branch" TEXT NOT NULL,
    "rollNumber" TEXT,
    "game" TEXT NOT NULL,
    "score" INTEGER NOT NULL DEFAULT 0,
    "avatar" TEXT,
    "prediction" INTEGER,
    "difference" INTEGER,
    "rank" INTEGER,
    "level" INTEGER NOT NULL DEFAULT 1,
    "battery" INTEGER NOT NULL DEFAULT 100,
    "wifi" INTEGER NOT NULL DEFAULT 0,
    "fps" INTEGER NOT NULL DEFAULT 0,
    "heap" INTEGER NOT NULL DEFAULT 0,
    "streak" INTEGER NOT NULL DEFAULT 0,
    "badges" TEXT NOT NULL DEFAULT '[]',
    "team" TEXT,
    "isActive" BOOLEAN NOT NULL DEFAULT true,
    "createdAt" DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    "updatedAt" DATETIME NOT NULL
);

CREATE TABLE "Games" (
    "id" TEXT NOT NULL PRIMARY KEY,
    "gameName" TEXT NOT NULL,
    "status" TEXT NOT NULL DEFAULT 'queued',
    "startTime" DATETIME,
    "endTime" DATETIME,
    "createdAt" DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE "Judges" (
    "id" TEXT NOT NULL PRIMARY KEY,
    "name" TEXT NOT NULL
);

CREATE TABLE "JudgePredictions" (
    "id" TEXT NOT NULL PRIMARY KEY,
    "playerId" TEXT NOT NULL,
    "judgeId" TEXT NOT NULL,
    "prediction" INTEGER NOT NULL,
    CONSTRAINT "JudgePredictions_playerId_fkey" FOREIGN KEY ("playerId") REFERENCES "Players" ("id") ON DELETE CASCADE ON UPDATE CASCADE,
    CONSTRAINT "JudgePredictions_judgeId_fkey" FOREIGN KEY ("judgeId") REFERENCES "Judges" ("id") ON DELETE CASCADE ON UPDATE CASCADE
);

CREATE TABLE "ProfessorChallenge" (
    "id" TEXT NOT NULL PRIMARY KEY,
    "targetScore" INTEGER NOT NULL,
    "active" BOOLEAN NOT NULL DEFAULT true,
    "createdAt" DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE "MysteryEvents" (
    "id" TEXT NOT NULL PRIMARY KEY,
    "title" TEXT NOT NULL,
    "description" TEXT NOT NULL,
    "multiplier" REAL NOT NULL DEFAULT 1,
    "points" INTEGER NOT NULL DEFAULT 0,
    "createdAt" DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE "LeaderboardHistory" (
    "id" TEXT NOT NULL PRIMARY KEY,
    "playerId" TEXT NOT NULL,
    "score" INTEGER NOT NULL,
    "timestamp" DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    CONSTRAINT "LeaderboardHistory_playerId_fkey" FOREIGN KEY ("playerId") REFERENCES "Players" ("id") ON DELETE CASCADE ON UPDATE CASCADE
);

CREATE TABLE "Telemetry" (
    "id" TEXT NOT NULL PRIMARY KEY,
    "playerId" TEXT,
    "name" TEXT NOT NULL,
    "game" TEXT NOT NULL,
    "score" INTEGER NOT NULL,
    "level" INTEGER NOT NULL,
    "battery" INTEGER NOT NULL,
    "wifi" INTEGER NOT NULL,
    "fps" INTEGER NOT NULL,
    "heap" INTEGER NOT NULL,
    "createdAt" DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    CONSTRAINT "Telemetry_playerId_fkey" FOREIGN KEY ("playerId") REFERENCES "Players" ("id") ON DELETE SET NULL ON UPDATE CASCADE
);

CREATE TABLE "Activity" (
    "id" TEXT NOT NULL PRIMARY KEY,
    "title" TEXT NOT NULL,
    "description" TEXT NOT NULL,
    "tone" TEXT NOT NULL DEFAULT 'info',
    "createdAt" DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE "Settings" (
    "key" TEXT NOT NULL PRIMARY KEY,
    "value" TEXT NOT NULL,
    "updatedAt" DATETIME NOT NULL
);

CREATE UNIQUE INDEX "Players_playerCode_key" ON "Players"("playerCode");
CREATE UNIQUE INDEX "Judges_name_key" ON "Judges"("name");
CREATE UNIQUE INDEX "JudgePredictions_playerId_judgeId_key" ON "JudgePredictions"("playerId", "judgeId");
CREATE UNIQUE INDEX "MysteryEvents_title_key" ON "MysteryEvents"("title");
