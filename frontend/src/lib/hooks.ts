"use client";

import { useState, useEffect, useCallback, useRef } from "react";
import { kvStoreApi, StoreStatus, KVStoreApiError } from "./api";

interface UseStoreStatusOptions {
  /** Polling interval in milliseconds (default: 2000) */
  pollingInterval?: number;
  /** Whether polling is enabled (default: true) */
  enabled?: boolean;
}

interface UseStoreStatusResult {
  /** Current store status */
  status: StoreStatus | null;
  /** Loading state (true during initial fetch) */
  isLoading: boolean;
  /** Error object if fetch failed */
  error: KVStoreApiError | null;
  /** Manually trigger a refresh */
  refresh: () => Promise<void>;
  /** Whether currently fetching (including polling) */
  isFetching: boolean;
}

/**
 * Hook for polling KV Store status
 * 
 * Fetches store status at regular intervals for Live Monitor.
 * Automatically handles cleanup on unmount.
 */
export function useStoreStatus(
  options: UseStoreStatusOptions = {}
): UseStoreStatusResult {
  const { pollingInterval = 2000, enabled = true } = options;
  
  const [status, setStatus] = useState<StoreStatus | null>(null);
  const [isLoading, setIsLoading] = useState(true);
  const [isFetching, setIsFetching] = useState(false);
  const [error, setError] = useState<KVStoreApiError | null>(null);
  
  // Track previous version to detect changes
  const previousVersionRef = useRef<number | null>(null);

  const fetchStatus = useCallback(async () => {
    setIsFetching(true);
    
    try {
      const newStatus = await kvStoreApi.getStatus();
      
      // Track version changes for animations
      if (previousVersionRef.current !== null && 
          newStatus.version !== previousVersionRef.current) {
        // Version changed - new data available
        console.log(`[KV Store] Version changed: ${previousVersionRef.current} -> ${newStatus.version}`);
      }
      
      previousVersionRef.current = newStatus.version;
      setStatus(newStatus);
      setError(null);
    } catch (err) {
      setError(err instanceof KVStoreApiError ? err : new KVStoreApiError(
        "Unknown error",
        0,
        String(err)
      ));
    } finally {
      setIsLoading(false);
      setIsFetching(false);
    }
  }, []);

  // Initial fetch and polling setup
  useEffect(() => {
    if (!enabled) return;
    
    // Initial fetch
    fetchStatus();
    
    // Set up polling interval
    const intervalId = setInterval(fetchStatus, pollingInterval);
    
    // Cleanup on unmount or when dependencies change
    return () => {
      clearInterval(intervalId);
    };
  }, [enabled, pollingInterval, fetchStatus]);

  return {
    status,
    isLoading,
    error,
    refresh: fetchStatus,
    isFetching,
  };
}

interface UseSearchKeyResult {
  /** Search for a key and return its value */
  search: (key: string) => Promise<string | null>;
  /** Current search result */
  result: { key: string; value: string } | null;
  /** Whether search is in progress */
  isSearching: boolean;
  /** Search error */
  error: KVStoreApiError | null;
  /** Clear search result */
  clear: () => void;
}

/**
 * Hook for searching by key
 */
export function useSearchKey(): UseSearchKeyResult {
  const [result, setResult] = useState<{ key: string; value: string } | null>(null);
  const [isSearching, setIsSearching] = useState(false);
  const [error, setError] = useState<KVStoreApiError | null>(null);

  const search = useCallback(async (key: string): Promise<string | null> => {
    if (!key.trim()) {
      setResult(null);
      setError(null);
      return null;
    }

    setIsSearching(true);
    setError(null);

    try {
      const response = await kvStoreApi.getValue(key);
      setResult({ key: response.key, value: response.value });
      return response.value;
    } catch (err) {
      const apiError = err instanceof KVStoreApiError ? err : new KVStoreApiError(
        "Search failed",
        0,
        String(err)
      );
      setError(apiError);
      setResult(null);
      return null;
    } finally {
      setIsSearching(false);
    }
  }, []);

  const clear = useCallback(() => {
    setResult(null);
    setError(null);
  }, []);

  return {
    search,
    result,
    isSearching,
    error,
    clear,
  };
}

interface UseSetValueResult {
  /** Set a key-value pair */
  setValue: (key: string, value: string) => Promise<boolean>;
  /** Whether operation is in progress */
  isSubmitting: boolean;
  /** Operation error */
  error: KVStoreApiError | null;
  /** Success message */
  successMessage: string | null;
  /** Clear messages */
  clearMessages: () => void;
}

/**
 * Hook for setting key-value pairs
 */
export function useSetValue(): UseSetValueResult {
  const [isSubmitting, setIsSubmitting] = useState(false);
  const [error, setError] = useState<KVStoreApiError | null>(null);
  const [successMessage, setSuccessMessage] = useState<string | null>(null);

  const setValue = useCallback(async (key: string, value: string): Promise<boolean> => {
    setIsSubmitting(true);
    setError(null);
    setSuccessMessage(null);

    try {
      const response = await kvStoreApi.setValue(key, value);
      setSuccessMessage(response.message);
      return true;
    } catch (err) {
      const apiError = err instanceof KVStoreApiError ? err : new KVStoreApiError(
        "Failed to set value",
        0,
        String(err)
      );
      setError(apiError);
      return false;
    } finally {
      setIsSubmitting(false);
    }
  }, []);

  const clearMessages = useCallback(() => {
    setError(null);
    setSuccessMessage(null);
  }, []);

  return {
    setValue,
    isSubmitting,
    error,
    successMessage,
    clearMessages,
  };
}


