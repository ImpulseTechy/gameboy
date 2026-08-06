import { PrismaClient } from "@prisma/client";
import {
  defaultJudges,
  defaultMysteryEvents,
  samplePlayers
} from "../src/constants";
import {
  assignTeam,
  buildPlayerCode,
  stringifyBadges
} from "../src/leaderboard";

const prisma = new PrismaClient();

async function main() {
  for (const name of defaultJudges) {
    await prisma.judge.upsert({
      where: { name },
      create: { name },
      update: {}
    });
  }

  for (const event of defaultMysteryEvents) {
    await prisma.mysteryEvent.upsert({
      where: { title: event.title },
      create: event,
      update: event
    });
  }

  await prisma.setting.upsert({
    where: { key: "currentMode" },
    create: { key: "currentMode", value: "classic" },
    update: {}
  });
  await prisma.setting.upsert({
    where: { key: "theme" },
    create: { key: "theme", value: "dark" },
    update: {}
  });

  const count = await prisma.player.count();
  if (count === 0) {
    for (const sample of samplePlayers) {
      const player = await prisma.player.create({
        data: {
          playerCode: buildPlayerCode(),
          name: sample.name,
          branch: sample.branch,
          rollNumber: sample.rollNumber,
          game: sample.game,
          score: sample.score,
          prediction: sample.prediction,
          level: sample.level,
          battery: sample.battery,
          wifi: sample.wifi,
          fps: sample.fps,
          heap: sample.heap,
          team: assignTeam(sample.branch),
          badges: stringifyBadges(sample.badges)
        }
      });
      await prisma.leaderboardHistory.create({
        data: { playerId: player.id, score: sample.score }
      });
      await prisma.telemetry.create({
        data: {
          playerId: player.id,
          name: player.name,
          game: player.game,
          score: player.score,
          level: player.level,
          battery: player.battery,
          wifi: player.wifi,
          fps: player.fps,
          heap: player.heap
        }
      });
    }
  }

  await prisma.professorChallenge.upsert({
    where: { id: "professor-default" },
    create: {
      id: "professor-default",
      targetScore: 67,
      active: true
    },
    update: {
      targetScore: 67,
      active: true
    }
  });

  await prisma.activity.create({
    data: {
      title: "Demo data loaded",
      description: "NST Arcade Live is ready for orientation",
      tone: "success"
    }
  });
}

main()
  .then(async () => {
    await prisma.$disconnect();
  })
  .catch(async (error) => {
    console.error(error);
    await prisma.$disconnect();
    process.exit(1);
  });
