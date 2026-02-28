/**
 * API Client for Shared Memory KV Store
 * 
 * Uses native fetch for optimal performance.
 * Base URL is configurable via environment variable.
 */

// API base URL - defaults to localhost:8000 for development
const API_BASE_URL = process.env.NEXT_PUBLIC_API_URL || "http://localhost:8000";

// Types matching FastAPI response models
export interface KVEntry {
  key: string;
  value: string;
  timestamp: number;
}

export interface StoreStatus {
  version: number;
  entry_count: number;
  max_entries: number;
  entries: KVEntry[];
}

export interface GetResponse {
  key: string;
  value: string;
}

export interface SetResponse {
  success: boolean;
  message: string;
}

export interface ApiError {
  detail: string;
}

// Custom error class for API errors
export class KVStoreApiError extends Error {
  constructor(
    message: string,
    public statusCode: number,
    public detail?: string
  ) {
    super(message);
    this.name = "KVStoreApiError";
  }
}

/**
 * Generic fetch wrapper with error handling
 */
async function apiFetch<T>(
  endpoint: string,
  options?: RequestInit
): Promise<T> {
  const url = `${API_BASE_URL}${endpoint}`;
  
  try {
    const response = await fetch(url, {
      ...options,
      headers: {
        "Content-Type": "application/json",
        ...options?.headers,
      },
    });

    if (!response.ok) {
      const errorData: ApiError = await response.json().catch(() => ({
        detail: `HTTP ${response.status}: ${response.statusText}`,
      }));
      
      throw new KVStoreApiError(
        errorData.detail || "Unknown error",
        response.status,
        errorData.detail
      );
    }

    return await response.json();
  } catch (error) {
    if (error instanceof KVStoreApiError) {
      throw error;
    }
    
    // Network error or other fetch failure
    throw new KVStoreApiError(
      error instanceof Error ? error.message : "Network error",
      0,
      "Failed to connect to API server"
    );
  }
}

/**
 * KV Store API client
 */
export const kvStoreApi = {
  /**
   * Get store status with all entries
   * Used for Live Monitor polling
   */
  async getStatus(): Promise<StoreStatus> {
    return apiFetch<StoreStatus>("/status");
  },

  /**
   * Get value by key
   */
  async getValue(key: string): Promise<GetResponse> {
    return apiFetch<GetResponse>(`/get/${encodeURIComponent(key)}`);
  },

  /**
   * Set key-value pair
   */
  async setValue(key: string, value: string): Promise<SetResponse> {
    return apiFetch<SetResponse>("/set", {
      method: "POST",
      body: JSON.stringify({ key, value }),
    });
  },
};

export default kvStoreApi;


