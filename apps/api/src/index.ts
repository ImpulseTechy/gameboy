import "dotenv/config";
import cors from "cors";
import express from "express";
import { createServer } from "http";
import path from "path";
import { Server } from "socket.io";
import { createApiRouter } from "./routes";
import { buildEventState, recordActivity } from "./leaderboard";

const port = Number(process.env.PORT ?? 4000);
const clientUrl = process.env.CLIENT_URL ?? "http://localhost:3000";

const app = express();
const server = createServer(app);
const io = new Server(server, {
  cors: {
    origin: [clientUrl, "http://127.0.0.1:3000", "http://localhost:3000"],
    methods: ["GET", "POST", "PATCH", "DELETE"]
  }
});

app.use(
  cors({
    origin: [clientUrl, "http://127.0.0.1:3000", "http://localhost:3000"],
    credentials: true
  })
);
app.use(express.json({ limit: "1mb" }));
app.use(express.urlencoded({ extended: true }));
app.use("/uploads", express.static(path.resolve(process.cwd(), "uploads")));
app.use("/api", createApiRouter(io));

io.on("connection", async (socket) => {
  socket.emit("leaderboard-updated", await buildEventState());
  socket.emit("player-connected", { socketId: socket.id });
});

app.use(
  (
    error: Error & { statusCode?: number; issues?: unknown },
    _req: express.Request,
    res: express.Response,
    _next: express.NextFunction
  ) => {
    const statusCode = error.statusCode ?? (error.issues ? 400 : 500);
    res.status(statusCode).json({
      error: statusCode === 500 ? "Internal server error" : error.message,
      details: error.issues
    });
  }
);

server.listen(port, async () => {
  try {
    await recordActivity(
      "API online",
      `Realtime server listening on port ${port}`,
      "success"
    );
  } catch {
    // The database may not be migrated yet during first boot.
  }
  console.log(`NST Arcade Live API running on http://localhost:${port}`);
});
