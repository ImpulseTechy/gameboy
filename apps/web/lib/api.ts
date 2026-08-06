import type { EventState } from "./types";

export const API_URL =
  process.env.NEXT_PUBLIC_API_URL?.replace(/\/$/, "") || "http://localhost:4000";

type FetchOptions = RequestInit & {
  token?: string | null;
};

export async function apiFetch<T>(path: string, options: FetchOptions = {}) {
  const headers = new Headers(options.headers);
  if (!(options.body instanceof FormData)) {
    headers.set("Content-Type", "application/json");
  }
  if (options.token) {
    headers.set("Authorization", `Bearer ${options.token}`);
  }

  const response = await fetch(`${API_URL}${path}`, {
    ...options,
    headers,
    cache: "no-store"
  });

  if (!response.ok) {
    const payload = await response.json().catch(() => null);
    throw new Error(payload?.error || `Request failed with ${response.status}`);
  }

  return response.json() as Promise<T>;
}

export function getEventState() {
  return apiFetch<EventState>("/api/state");
}

export function postJson<T>(
  path: string,
  body: Record<string, unknown>,
  token?: string | null
) {
  return apiFetch<T>(path, {
    method: "POST",
    body: JSON.stringify(body),
    token
  });
}

export async function downloadExport(format: "csv" | "xlsx" | "json", token: string) {
  const response = await fetch(`${API_URL}/api/export/${format}`, {
    headers: { Authorization: `Bearer ${token}` }
  });
  if (!response.ok) throw new Error(`Export failed with ${response.status}`);
  const blob = await response.blob();
  const url = URL.createObjectURL(blob);
  const link = document.createElement("a");
  link.href = url;
  link.download = `nst-arcade.${format}`;
  document.body.appendChild(link);
  link.click();
  link.remove();
  URL.revokeObjectURL(url);
}
