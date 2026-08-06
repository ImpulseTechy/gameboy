import bcrypt from "bcryptjs";
import type { NextFunction, Request, Response } from "express";
import { Router } from "express";
import jwt from "jsonwebtoken";
import multer from "multer";
import type { Server } from "socket.io";
import { z } from "zod";
import ExcelJS from "exceljs";
import { defaultMysteryEvents, gameModes } from "./constants";
import { prisma } from "./db";
import {
  applyBadges,
  assignTeam,
  buildEventState,
  buildPlayerCode,
  parseBadges,
  recordActivity,
  setSetting,
  stringifyBadges
} from "./leaderboard";

type AsyncRoute = (
  req: Request,
  res: Response,
  next: NextFunction
) => Promise<void>;

const upload = multer({
  dest: "uploads/",
  limits: { fileSize: 4 * 1024 * 1024 }
});

const registerSchema = z.object({
  name: z.string().min(2).max(80),
  branch: z.string().min(2).max(60),
  rollNumber: z.string().max(40).optional().or(z.literal("")),
  game: z.string().min(2).max(40),
  prediction: z.coerce.number().int().min(0).max(9999).optional()
});

const scoreSchema = z.object({
  playerId: z.string().optional(),
  playerCode: z.string().optional(),
  name: z.string().min(2).max(80).optional(),
  branch: z.string().min(2).max(60).optional(),
  rollNumber: z.string().max(40).optional(),
  game: z.string().min(2).max(40).default("Snake"),
  score: z.coerce.number().int().min(0).max(99999),
  level: z.coerce.number().int().min(1).max(100).optional(),
  battery: z.coerce.number().int().min(0).max(100).optional(),
  wifi: z.coerce.number().int().min(-120).max(0).optional(),
  fps: z.coerce.number().int().min(0).max(240).optional(),
  heap: z.coerce.number().int().min(0).max(1000000).optional()
});

function asyncHandler(route: AsyncRoute) {
  return (req: Request, res: Response, next: NextFunction) => {
    route(req, res, next).catch(next);
  };
}

function jwtSecret() {
  return process.env.JWT_SECRET || "nst-arcade-local-secret";
}

function isAdminToken(req: Request) {
  const header = req.headers.authorization;
  const token =
    header?.startsWith("Bearer ") === true
      ? header.slice("Bearer ".length)
      : typeof req.query.token === "string"
        ? req.query.token
        : "";

  if (!token) return false;

  try {
    jwt.verify(token, jwtSecret());
    return true;
  } catch {
    return false;
  }
}

function requireAdmin(req: Request, res: Response, next: NextFunction) {
  if (!isAdminToken(req)) {
    res.status(401).json({ error: "Admin token required" });
    return;
  }
  next();
}

async function emitState(io: Server, event?: string, payload?: unknown) {
  const state = await buildEventState();
  io.emit("leaderboard-updated", state);
  if (event) io.emit(event, payload ?? state);
  return state;
}

async function findOrCreatePlayer(input: z.infer<typeof scoreSchema>) {
  const existing = input.playerId
    ? await prisma.player.findUnique({ where: { id: input.playerId } })
    : input.playerCode
      ? await prisma.player.findUnique({ where: { playerCode: input.playerCode } })
      : input.name
        ? await prisma.player.findFirst({
            where: { name: input.name, game: input.game, isActive: true },
            orderBy: { createdAt: "desc" }
          })
        : null;

  if (existing) return existing;

  if (!input.name) {
    throw Object.assign(new Error("playerId, playerCode, or name is required"), {
      statusCode: 400
    });
  }

  return prisma.player.create({
    data: {
      playerCode: buildPlayerCode(),
      name: input.name,
      branch: input.branch ?? "Computer",
      rollNumber: input.rollNumber || null,
      game: input.game,
      score: 0,
      team: assignTeam(input.branch ?? "Computer"),
      badges: stringifyBadges(["Rookie"])
    }
  });
}

async function applyScoreUpdate(input: z.infer<typeof scoreSchema>) {
  const player = await findOrCreatePlayer(input);
  const previousHigh = await prisma.player.findFirst({
    where: { isActive: true },
    orderBy: { score: "desc" }
  });

  const [modeSetting, professor, judgePredictions] = await Promise.all([
    prisma.setting.findUnique({ where: { key: "currentMode" } }),
    prisma.professorChallenge.findFirst({
      where: { active: true },
      orderBy: { createdAt: "desc" }
    }),
    prisma.judgePrediction.findMany({ where: { playerId: player.id } })
  ]);

  const mode = modeSetting?.value ?? "classic";
  const judgeAverage =
    judgePredictions.length === 0
      ? null
      : Math.round(
          judgePredictions.reduce((sum, item) => sum + item.prediction, 0) /
            judgePredictions.length
        );
  const luckyExact =
    mode === "lucky-score" &&
    player.prediction !== null &&
    player.prediction === input.score;
  const finalScore = luckyExact ? input.score + 50 : input.score;
  const beatProfessor =
    mode === "beat-professor" &&
    professor !== null &&
    finalScore > professor.targetScore;
  const difference =
    mode === "nst-got-talent" && judgeAverage !== null
      ? Math.abs(finalScore - judgeAverage)
      : player.prediction === null
        ? null
        : Math.abs(finalScore - player.prediction);
  const badges = applyBadges({
    score: finalScore,
    battery: input.battery ?? player.battery,
    wifi: input.wifi ?? player.wifi,
    difference,
    beatProfessor,
    luckyExact,
    previous: parseBadges(player.badges)
  });

  const updated = await prisma.player.update({
    where: { id: player.id },
    data: {
      score: finalScore,
      game: input.game,
      level: input.level ?? player.level,
      battery: input.battery ?? player.battery,
      wifi: input.wifi ?? player.wifi,
      fps: input.fps ?? player.fps,
      heap: input.heap ?? player.heap,
      difference,
      streak: finalScore > player.score ? player.streak + 1 : player.streak,
      badges: stringifyBadges(badges)
    }
  });

  await Promise.all([
    prisma.leaderboardHistory.create({
      data: { playerId: player.id, score: finalScore }
    }),
    prisma.telemetry.create({
      data: {
        playerId: player.id,
        name: updated.name,
        game: updated.game,
        score: updated.score,
        level: updated.level,
        battery: updated.battery,
        wifi: updated.wifi,
        fps: updated.fps,
        heap: updated.heap
      }
    }),
    recordActivity(
      "Score received",
      `${updated.name} uploaded ${finalScore} in ${updated.game}`,
      finalScore > (previousHigh?.score ?? 0) ? "success" : "info"
    )
  ]);

  return {
    player: updated,
    newHighest: finalScore > (previousHigh?.score ?? 0),
    beatProfessor,
    luckyExact
  };
}

function escapeCsv(value: unknown) {
  const raw = String(value ?? "");
  return `"${raw.replace(/"/g, '""')}"`;
}

async function exportPlayers(format: string, res: Response) {
  const players = await prisma.player.findMany({
    where: { isActive: true },
    orderBy: [{ rank: "asc" }, { score: "desc" }]
  });
  const rows = players.map((player) => ({
    rank: player.rank ?? "",
    playerCode: player.playerCode,
    name: player.name,
    branch: player.branch,
    rollNumber: player.rollNumber ?? "",
    game: player.game,
    score: player.score,
    prediction: player.prediction ?? "",
    difference: player.difference ?? "",
    badges: parseBadges(player.badges).join("; "),
    createdAt: player.createdAt.toISOString()
  }));

  if (format === "json") {
    res.setHeader("Content-Disposition", "attachment; filename=nst-arcade.json");
    res.json(rows);
    return;
  }

  if (format === "csv") {
    const headers = Object.keys(rows[0] ?? {
      rank: "",
      playerCode: "",
      name: "",
      branch: "",
      rollNumber: "",
      game: "",
      score: "",
      prediction: "",
      difference: "",
      badges: "",
      createdAt: ""
    });
    const csv = [
      headers.map(escapeCsv).join(","),
      ...rows.map((row) =>
        headers.map((header) => escapeCsv(row[header as keyof typeof row])).join(",")
      )
    ].join("\n");
    res.setHeader("Content-Disposition", "attachment; filename=nst-arcade.csv");
    res.type("text/csv").send(csv);
    return;
  }

  if (format === "xlsx") {
    const workbook = new ExcelJS.Workbook();
    const sheet = workbook.addWorksheet("Leaderboard");
    sheet.columns = Object.keys(rows[0] ?? { name: "" }).map((key) => ({
      header: key,
      key,
      width: Math.max(14, key.length + 2)
    }));
    sheet.addRows(rows);
    sheet.getRow(1).font = { bold: true, color: { argb: "FFFFFFFF" } };
    sheet.getRow(1).fill = {
      type: "pattern",
      pattern: "solid",
      fgColor: { argb: "FF111827" }
    };
    res.setHeader(
      "Content-Type",
      "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet"
    );
    res.setHeader("Content-Disposition", "attachment; filename=nst-arcade.xlsx");
    await workbook.xlsx.write(res);
    res.end();
    return;
  }

  res.status(400).json({ error: "Unsupported export format" });
}

export function createApiRouter(io: Server) {
  const router = Router();

  router.get("/health", (_req, res) => {
    res.json({ ok: true, service: "NST Arcade Live API" });
  });

  router.post(
    "/auth/login",
    asyncHandler(async (req, res) => {
      const body = z
        .object({ username: z.string(), password: z.string() })
        .parse(req.body);
      const username = process.env.ADMIN_USERNAME || "admin";
      const password = process.env.ADMIN_PASSWORD || "nst-arcade";
      const passwordHash = process.env.ADMIN_PASSWORD_HASH;
      const validUser = body.username === username;
      const validPassword = passwordHash
        ? await bcrypt.compare(body.password, passwordHash)
        : body.password === password;

      if (!validUser || !validPassword) {
        res.status(401).json({ error: "Invalid credentials" });
        return;
      }

      const token = jwt.sign({ role: "admin", username }, jwtSecret(), {
        expiresIn: "10h"
      });
      res.json({ token, username });
    })
  );

  router.get(
    "/state",
    asyncHandler(async (_req, res) => {
      res.json(await buildEventState());
    })
  );

  router.get(
    "/leaderboard",
    asyncHandler(async (_req, res) => {
      const state = await buildEventState();
      res.json(state.leaderboard);
    })
  );

  router.get(
    "/history",
    asyncHandler(async (_req, res) => {
      const history = await prisma.leaderboardHistory.findMany({
        include: { player: true },
        orderBy: { timestamp: "desc" },
        take: 200
      });
      res.json(
        history.map((item) => ({
          id: item.id,
          playerId: item.playerId,
          player: item.player.name,
          game: item.player.game,
          score: item.score,
          timestamp: item.timestamp
        }))
      );
    })
  );

  router.post(
    "/register",
    asyncHandler(async (req, res) => {
      const body = registerSchema.parse(req.body);
      const player = await prisma.player.create({
        data: {
          playerCode: buildPlayerCode(),
          name: body.name.trim(),
          branch: body.branch,
          rollNumber: body.rollNumber || null,
          game: body.game,
          prediction: body.prediction,
          team: assignTeam(body.branch),
          badges: stringifyBadges(["Rookie"])
        }
      });

      await recordActivity(
        "Player registered",
        `${player.name} is ready for ${player.game}`,
        "success"
      );
      const state = await emitState(io, "player-connected", { player });
      res.status(201).json({ player, state });
    })
  );

  router.post(
    "/score",
    asyncHandler(async (req, res) => {
      const result = await applyScoreUpdate(scoreSchema.parse(req.body));
      const state = await emitState(io, "score-added", result);
      if (result.newHighest) {
        io.emit("winner-announced", {
          player: result.player,
          title: "New highest score"
        });
      }
      if (result.beatProfessor) {
        io.emit("winner-announced", {
          player: result.player,
          title: "You defeated the Professor"
        });
      }
      res.json({ ...result, state });
    })
  );

  router.post(
    "/telemetry",
    asyncHandler(async (req, res) => {
      const result = await applyScoreUpdate(scoreSchema.parse(req.body));
      const state = await emitState(io, "telemetry-updated", result);
      res.json({ ...result, state });
    })
  );

  router.post(
    "/predict",
    asyncHandler(async (req, res) => {
      const body = z
        .object({
          playerId: z.string(),
          judgeId: z.string().optional(),
          judgeName: z.string().optional(),
          prediction: z.coerce.number().int().min(0).max(99999),
          ownPrediction: z.boolean().optional()
        })
        .parse(req.body);

      if (body.ownPrediction || (!body.judgeId && !body.judgeName)) {
        const player = await prisma.player.update({
          where: { id: body.playerId },
          data: { prediction: body.prediction }
        });
        await recordActivity(
          "Prediction locked",
          `${player.name} predicted ${body.prediction}`,
          "info"
        );
        res.json({ player, state: await emitState(io, "leaderboard-updated") });
        return;
      }

      const judge = body.judgeId
        ? await prisma.judge.findUniqueOrThrow({ where: { id: body.judgeId } })
        : await prisma.judge.upsert({
            where: { name: body.judgeName ?? "Guest Judge" },
            create: { name: body.judgeName ?? "Guest Judge" },
            update: {}
          });

      const prediction = await prisma.judgePrediction.upsert({
        where: {
          playerId_judgeId: {
            playerId: body.playerId,
            judgeId: judge.id
          }
        },
        create: {
          playerId: body.playerId,
          judgeId: judge.id,
          prediction: body.prediction
        },
        update: { prediction: body.prediction }
      });

      const allPredictions = await prisma.judgePrediction.findMany({
        where: { playerId: body.playerId }
      });
      const player = await prisma.player.findUniqueOrThrow({
        where: { id: body.playerId }
      });
      const average = Math.round(
        allPredictions.reduce((sum, item) => sum + item.prediction, 0) /
          allPredictions.length
      );
      await prisma.player.update({
        where: { id: player.id },
        data: { difference: Math.abs(player.score - average) }
      });
      await recordActivity(
        "Judge prediction",
        `${judge.name} predicted ${body.prediction} for ${player.name}`,
        "info"
      );
      res.json({ prediction, state: await emitState(io, "leaderboard-updated") });
    })
  );

  router.delete(
    "/player/:id",
    requireAdmin,
    asyncHandler(async (req, res) => {
      await prisma.player.delete({ where: { id: req.params.id } });
      await recordActivity("Player deleted", "Admin removed a player", "warning");
      res.json({ state: await emitState(io, "leaderboard-updated") });
    })
  );

  router.patch(
    "/player/:id",
    requireAdmin,
    asyncHandler(async (req, res) => {
      const body = registerSchema.partial().extend({
        score: z.coerce.number().int().min(0).max(99999).optional()
      }).parse(req.body);
      const current = await prisma.player.findUniqueOrThrow({
        where: { id: req.params.id }
      });
      const player = await prisma.player.update({
        where: { id: req.params.id },
        data: {
          name: body.name ?? current.name,
          branch: body.branch ?? current.branch,
          rollNumber: body.rollNumber ?? current.rollNumber,
          game: body.game ?? current.game,
          score: body.score ?? current.score,
          prediction: body.prediction ?? current.prediction,
          team: body.branch ? assignTeam(body.branch) : current.team
        }
      });
      if (typeof body.score === "number") {
        await prisma.leaderboardHistory.create({
          data: { playerId: player.id, score: body.score }
        });
      }
      await recordActivity(
        "Admin update",
        `${player.name} was updated from the control room`,
        "info"
      );
      res.json({ player, state: await emitState(io, "leaderboard-updated") });
    })
  );

  router.post(
    "/reset",
    requireAdmin,
    asyncHandler(async (_req, res) => {
      await prisma.$transaction([
        prisma.judgePrediction.deleteMany(),
        prisma.leaderboardHistory.deleteMany(),
        prisma.telemetry.deleteMany(),
        prisma.player.deleteMany(),
        prisma.game.deleteMany(),
        prisma.professorChallenge.deleteMany(),
        prisma.activity.deleteMany()
      ]);
      await recordActivity("Event reset", "Leaderboard and telemetry cleared", "danger");
      res.json({ state: await emitState(io, "leaderboard-updated") });
    })
  );

  router.post(
    "/mystery",
    requireAdmin,
    asyncHandler(async (req, res) => {
      const body = z.object({ eventId: z.string().optional() }).parse(req.body);
      const events = await prisma.mysteryEvent.findMany();
      const chosen = body.eventId
        ? await prisma.mysteryEvent.findUniqueOrThrow({
            where: { id: body.eventId }
          })
        : events[Math.floor(Math.random() * events.length)];
      await setSetting("lastMystery", JSON.stringify(chosen));
      await recordActivity(
        "Mystery wheel",
        `${chosen.title}: ${chosen.description}`,
        chosen.points > 0 || chosen.multiplier > 1 ? "success" : "warning"
      );
      const state = await emitState(io, "mystery-event", chosen);
      res.json({ event: chosen, state });
    })
  );

  router.post(
    "/professor",
    requireAdmin,
    asyncHandler(async (req, res) => {
      const body = z
        .object({ targetScore: z.coerce.number().int().min(1).max(99999) })
        .parse(req.body);
      await prisma.professorChallenge.updateMany({ data: { active: false } });
      const challenge = await prisma.professorChallenge.create({
        data: { targetScore: body.targetScore }
      });
      await recordActivity(
        "Professor challenge",
        `Target score set to ${challenge.targetScore}`,
        "warning"
      );
      res.json({
        challenge,
        state: await emitState(io, "professor-updated", challenge)
      });
    })
  );

  router.get(
    "/judges",
    asyncHandler(async (_req, res) => {
      res.json(await prisma.judge.findMany({ orderBy: { name: "asc" } }));
    })
  );

  router.post(
    "/judges",
    requireAdmin,
    asyncHandler(async (req, res) => {
      const body = z.object({ name: z.string().min(2).max(80) }).parse(req.body);
      const judge = await prisma.judge.upsert({
        where: { name: body.name },
        create: { name: body.name },
        update: {}
      });
      await recordActivity("Judge added", `${judge.name} joined the panel`, "info");
      res.json({ judge, state: await emitState(io, "leaderboard-updated") });
    })
  );

  router.delete(
    "/judges/:id",
    requireAdmin,
    asyncHandler(async (req, res) => {
      await prisma.judge.delete({ where: { id: req.params.id } });
      res.json({ state: await emitState(io, "leaderboard-updated") });
    })
  );

  router.post(
    "/admin/player",
    requireAdmin,
    asyncHandler(async (req, res) => {
      const body = registerSchema.parse(req.body);
      const player = await prisma.player.create({
        data: {
          playerCode: buildPlayerCode(),
          name: body.name.trim(),
          branch: body.branch,
          rollNumber: body.rollNumber || null,
          game: body.game,
          prediction: body.prediction,
          team: assignTeam(body.branch),
          badges: stringifyBadges(["Rookie", "Admin Added"])
        }
      });
      await recordActivity("Admin add", `${player.name} entered the arena`, "success");
      res.status(201).json({ player, state: await emitState(io, "player-connected") });
    })
  );

  router.post(
    "/admin/mode",
    requireAdmin,
    asyncHandler(async (req, res) => {
      const body = z.object({ mode: z.string() }).parse(req.body);
      const validMode = gameModes.some((mode) => mode.id === body.mode);
      if (!validMode) {
        res.status(400).json({ error: "Unknown game mode" });
        return;
      }
      await setSetting("currentMode", body.mode);
      await recordActivity(
        "Mode switched",
        `Current mode is ${gameModes.find((mode) => mode.id === body.mode)?.label}`,
        "info"
      );
      res.json({ state: await emitState(io, "game-started") });
    })
  );

  router.post(
    "/admin/theme",
    requireAdmin,
    asyncHandler(async (req, res) => {
      const body = z.object({ theme: z.enum(["dark", "light"]) }).parse(req.body);
      await setSetting("theme", body.theme);
      res.json({ state: await emitState(io, "theme-updated") });
    })
  );

  router.post(
    "/admin/logo",
    requireAdmin,
    upload.single("logo"),
    asyncHandler(async (req, res) => {
      if (!req.file) {
        res.status(400).json({ error: "Logo file is required" });
        return;
      }
      const path = `/uploads/${req.file.filename}`;
      await setSetting("collegeLogo", path);
      await recordActivity("Logo uploaded", "College logo updated", "info");
      res.json({ path, state: await emitState(io, "theme-updated") });
    })
  );

  router.post(
    "/game/start",
    requireAdmin,
    asyncHandler(async (req, res) => {
      const body = z.object({ gameName: z.string().min(2).max(40) }).parse(req.body);
      await prisma.game.updateMany({
        where: { status: "live" },
        data: { status: "ended", endTime: new Date() }
      });
      const game = await prisma.game.create({
        data: { gameName: body.gameName, status: "live", startTime: new Date() }
      });
      await recordActivity("Game started", `${game.gameName} is live`, "success");
      res.json({ game, state: await emitState(io, "game-started", game) });
    })
  );

  router.post(
    "/game/end",
    requireAdmin,
    asyncHandler(async (_req, res) => {
      const games = await prisma.game.updateMany({
        where: { status: "live" },
        data: { status: "ended", endTime: new Date() }
      });
      await recordActivity("Game ended", "Live game session closed", "warning");
      res.json({ games, state: await emitState(io, "game-ended") });
    })
  );

  router.get(
    "/export/:format",
    requireAdmin,
    asyncHandler(async (req, res) => {
      await exportPlayers(req.params.format, res);
    })
  );

  router.post(
    "/maintenance/seed",
    requireAdmin,
    asyncHandler(async (_req, res) => {
      for (const event of defaultMysteryEvents) {
        await prisma.mysteryEvent.upsert({
          where: { title: event.title },
          create: event,
          update: event
        });
      }
      res.json({ state: await emitState(io, "leaderboard-updated") });
    })
  );

  return router;
}
