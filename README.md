# NST Arcade Live

Where Engineering Meets Gaming.

NST Arcade Live is a production-ready orientation event system for ESP32 handheld game consoles. Students play Snake or Tetris on hardware, the ESP32 posts telemetry to the API, and the projector dashboard updates in realtime with tournament-grade screens, game modes, judges, mystery events, branch battles, and admin controls.

## Phases Delivered

1. Project setup: npm workspace, Next.js 15 app, Express API, TypeScript, TailwindCSS, Docker.
2. Backend: REST API, Socket.io, Prisma ORM, SQLite, JWT admin auth, Multer uploads, exports.
3. Frontend: landing arena, registration, now playing, leaderboard, big screen, admin panel.
4. Socket integration: score, telemetry, player, game, mystery, winner, leaderboard events.
5. Animations: particles, neon backdrop, animated numbers, podium motion, confetti, mystery wheel.
6. ESP32 integration: Arduino WiFi client with reconnects, retries, timeout, JSON posts.
7. Testing hooks: typecheck/build scripts and seed data for local verification.
8. Deployment: Dockerfiles and `docker-compose.yml`.

## Stack

- Frontend: Next.js 15, React 19, TypeScript, TailwindCSS, Framer Motion, Socket.io Client, React Icons, Chart.js, QR code generation, PWA service worker.
- Backend: Node.js, Express, Socket.io, SQLite, Prisma ORM, JWT, Multer, ExcelJS.
- Hardware: ESP32 Arduino client posting JSON telemetry.

## Project Structure

```text
.
├── apps
│   ├── api
│   │   ├── prisma
│   │   │   ├── migrations
│   │   │   ├── schema.prisma
│   │   │   └── seed.ts
│   │   └── src
│   │       ├── constants.ts
│   │       ├── db.ts
│   │       ├── index.ts
│   │       ├── leaderboard.ts
│   │       └── routes.ts
│   └── web
│       ├── app
│       ├── components
│       ├── hooks
│       ├── lib
│       └── public
├── esp32
│   └── nst_arcade_live_client.ino
└── docker-compose.yml
```

## Local Setup

```bash
npm install
cp apps/api/.env.example apps/api/.env
npm run db:generate
npm run db:deploy
npm run db:seed
npm run dev
```

Open:

- Web app: [http://localhost:3000](http://localhost:3000)
- API health: [http://localhost:4000/api/health](http://localhost:4000/api/health)
- Admin: [http://localhost:3000/admin](http://localhost:3000/admin)

Default admin login:

```text
username: admin
password: nst-arcade
```

## Environment Variables

```text
PORT=4000
CLIENT_URL=http://localhost:3000
DATABASE_URL=file:./dev.db
JWT_SECRET=change-me-before-the-event
ADMIN_USERNAME=admin
ADMIN_PASSWORD=nst-arcade
NEXT_PUBLIC_API_URL=http://localhost:4000
```

For production, set `JWT_SECRET` and replace the default admin password or use `ADMIN_PASSWORD_HASH`.

## Database

Prisma models map to these SQLite tables:

- `Players`
- `Games`
- `Judges`
- `JudgePredictions`
- `ProfessorChallenge`
- `MysteryEvents`
- `LeaderboardHistory`
- `Telemetry`
- `Activity`
- `Settings`

Useful commands:

```bash
npm run db:generate
npm run db:deploy
npm run db:seed
```

## REST API

Public event endpoints:

- `POST /api/register`
- `POST /api/score`
- `POST /api/predict`
- `POST /api/telemetry`
- `GET /api/leaderboard`
- `GET /api/history`
- `GET /api/state`

Admin endpoints require `Authorization: Bearer <token>`:

- `POST /api/auth/login`
- `DELETE /api/player/:id`
- `PATCH /api/player/:id`
- `POST /api/reset`
- `POST /api/mystery`
- `POST /api/professor`
- `POST /api/judges`
- `DELETE /api/judges/:id`
- `POST /api/admin/player`
- `POST /api/admin/mode`
- `POST /api/admin/theme`
- `POST /api/admin/logo`
- `POST /api/game/start`
- `POST /api/game/end`
- `GET /api/export/csv`
- `GET /api/export/xlsx`
- `GET /api/export/json`

Telemetry example:

```json
{
  "name": "Rahul",
  "branch": "Computer",
  "game": "Snake",
  "score": 88,
  "level": 5,
  "battery": 91,
  "wifi": -58,
  "fps": 30,
  "heap": 182000
}
```

## Socket Events

The API emits:

- `score-added`
- `leaderboard-updated`
- `player-connected`
- `game-started`
- `game-ended`
- `telemetry-updated`
- `winner-announced`
- `mystery-event`
- `professor-updated`
- `theme-updated`

## Frontend Screens

- `/`: esports landing arena with metrics, QR join, activity, modes, and branch chart.
- `/register`: mobile-friendly student registration and player ID generation.
- `/playing`: huge current player display, score animation, level, high score, battery, WiFi, FPS, heap, and demo telemetry pulse.
- `/leaderboard`: full-screen podium, animated table, badges, score trends, confetti for new highs.
- `/big-screen`: projector view with current player, next player, challenge, sponsor/logo area, fullscreen button.
- `/admin`: secure dashboard for players, scores, resets, exports, judges, professor score, mystery wheel, theme, mode, logo, and game state.

## Component Notes

- `useLiveEvent` centralizes `/api/state` loading, Socket.io subscriptions, notifications, and score pulse sounds.
- `LeaderboardPodium` and `LeaderboardTable` render top-three and full rankings from the same payload.
- `TelemetryGrid` maps ESP32 packet fields directly into projector-safe cards.
- `MysteryWheel` gives admin a visual spin control backed by `/api/mystery`.
- `BranchBattleChart` and `ScoreHistoryChart` use Chart.js for audience-readable trends.

## ESP32 Setup

1. Open `esp32/nst_arcade_live_client.ino` in Arduino IDE.
2. Install Arduino libraries: `ArduinoJson`.
3. Set `WIFI_SSID`, `WIFI_PASSWORD`, and `SERVER_URL`.
4. Use your laptop IP for `SERVER_URL`, for example `http://192.168.1.10:4000/api/telemetry`.
5. Upload to ESP32.

The included firmware reconnects WiFi automatically, posts JSON once per second, retries failed packets, uses HTTP timeouts, and prints server status through Serial.

## Docker

```bash
docker compose up --build
```

The API runs on port `4000`, the web app runs on port `3000`, and SQLite data is stored under `./data`.

## Production Checklist

- Change `JWT_SECRET`.
- Replace default admin password.
- Put the API and web app behind HTTPS.
- Set `NEXT_PUBLIC_API_URL` to the public API URL.
- Confirm projector resolution using `/big-screen`.
- Run one ESP32 telemetry upload before the event opens.
