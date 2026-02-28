"use client";

import { motion } from "framer-motion";
import { Database, Cpu, Activity, AlertTriangle, RefreshCw } from "lucide-react";
import { Card, CardContent } from "@/components/ui/card";
import { Button } from "@/components/ui/button";
import { MemoryGrid } from "@/components/MemoryGrid";
import { SetValueDialog } from "@/components/SetValueDialog";
import { SearchKey } from "@/components/SearchKey";
import { ThemeToggle } from "@/components/ThemeToggle";
import { useStoreStatus } from "@/lib/hooks";

/**
 * Header component with title and status
 */
function Header({
  isConnected,
  isFetching,
  onRefresh
}: {
  isConnected: boolean;
  isFetching: boolean;
  onRefresh: () => void;
}) {
  return (
    <motion.header
      initial={{ opacity: 0, y: -20 }}
      animate={{ opacity: 1, y: 0 }}
      className="border-b border-border/30 bg-card/30 backdrop-blur-sm"
    >
      <div className="container mx-auto px-4 py-4">
        <div className="flex items-center justify-between">
          {/* Logo and title */}
          <div className="flex items-center gap-3">
            <div className="p-2 rounded-lg bg-primary/10 border border-primary/30 cyber-glow">
              <Database className="h-6 w-6 text-primary" />
            </div>
            <div>
              <h1 className="text-xl font-bold tracking-tight">
                SHM KV Store
              </h1>
              <p className="text-xs text-muted-foreground font-mono">
                Shared Memory Dashboard
              </p>
            </div>
          </div>

          {/* Status and actions */}
          <div className="flex items-center gap-4">
            {/* Connection status */}
            <div className="flex items-center gap-2 text-sm">
              <div className={`h-2 w-2 rounded-full ${isConnected
                  ? "bg-green-400 animate-pulse"
                  : "bg-red-400"
                }`} />
              <span className="text-muted-foreground">
                {isConnected ? "Connected" : "Disconnected"}
              </span>
            </div>

            {/* Theme Toggle */}
            <ThemeToggle />

            {/* Refresh button */}
            <Button
              variant="ghost"
              size="icon"
              onClick={onRefresh}
              disabled={isFetching}
              className="h-8 w-8"
            >
              <RefreshCw className={`h-4 w-4 ${isFetching ? "animate-spin" : ""}`} />
            </Button>

            {/* Set Value button */}
            <SetValueDialog onSuccess={onRefresh} />
          </div>
        </div>
      </div>
    </motion.header>
  );
}

/**
 * Stats card component
 */
function StatCard({
  icon: Icon,
  label,
  value,
  subValue,
  delay = 0
}: {
  icon: React.ElementType;
  label: string;
  value: string | number;
  subValue?: string;
  delay?: number;
}) {
  return (
    <motion.div
      initial={{ opacity: 0, y: 20 }}
      animate={{ opacity: 1, y: 0 }}
      transition={{ delay }}
    >
      <Card className="border-border/50 bg-card/50 backdrop-blur">
        <CardContent className="p-4">
          <div className="flex items-center gap-3">
            <div className="p-2 rounded-lg bg-primary/10 border border-primary/30">
              <Icon className="h-5 w-5 text-primary" />
            </div>
            <div>
              <p className="text-xs text-muted-foreground uppercase tracking-wider">
                {label}
              </p>
              <p className="text-2xl font-bold font-mono">
                {value}
                {subValue && (
                  <span className="text-sm text-muted-foreground ml-1">
                    {subValue}
                  </span>
                )}
              </p>
            </div>
          </div>
        </CardContent>
      </Card>
    </motion.div>
  );
}

/**
 * Error state component
 */
function ErrorState({
  error,
  onRetry
}: {
  error: string;
  onRetry: () => void;
}) {
  return (
    <motion.div
      initial={{ opacity: 0, scale: 0.95 }}
      animate={{ opacity: 1, scale: 1 }}
      className="flex flex-col items-center justify-center min-h-[400px] gap-4"
    >
      <div className="p-4 rounded-full bg-destructive/10 border border-destructive/30">
        <AlertTriangle className="h-12 w-12 text-destructive" />
      </div>
      <div className="text-center">
        <h2 className="text-xl font-semibold mb-2">Connection Error</h2>
        <p className="text-muted-foreground max-w-md">
          {error}
        </p>
      </div>
      <Button onClick={onRetry} className="gap-2">
        <RefreshCw className="h-4 w-4" />
        Retry Connection
      </Button>
    </motion.div>
  );
}

/**
 * Loading skeleton for stats
 */
function StatsSkeleton() {
  return (
    <div className="grid grid-cols-1 md:grid-cols-3 gap-4">
      {[0, 1, 2].map((i) => (
        <Card key={i} className="border-border/50 bg-card/50">
          <CardContent className="p-4">
            <div className="flex items-center gap-3">
              <div className="h-10 w-10 rounded-lg bg-muted/50 animate-pulse" />
              <div className="space-y-2">
                <div className="h-3 w-16 bg-muted/50 rounded animate-pulse" />
                <div className="h-6 w-24 bg-muted/50 rounded animate-pulse" />
              </div>
            </div>
          </CardContent>
        </Card>
      ))}
    </div>
  );
}

/**
 * Main dashboard page
 */
export default function Dashboard() {
  const { status, isLoading, error, refresh, isFetching } = useStoreStatus({
    pollingInterval: 2000,
    enabled: true,
  });

  const isConnected = !error && status !== null;

  return (
    <div className="min-h-screen bg-background grid-pattern relative">
      {/* Scanline overlay */}
      <div className="scanlines fixed inset-0 pointer-events-none" />

      {/* Header */}
      <Header
        isConnected={isConnected}
        isFetching={isFetching}
        onRefresh={refresh}
      />

      {/* Main content */}
      <main className="container mx-auto px-4 py-6 space-y-6">
        {/* Error state */}
        {error && !isLoading && (
          <ErrorState
            error={error.detail || error.message}
            onRetry={refresh}
          />
        )}

        {/* Loading state */}
        {isLoading && (
          <div className="space-y-6">
            <StatsSkeleton />
            <Card className="border-border/50 bg-card/50 h-[400px] animate-pulse" />
          </div>
        )}

        {/* Connected state */}
        {!isLoading && status && (
          <>
            {/* Stats row */}
            <div className="grid grid-cols-1 md:grid-cols-3 gap-4">
              <StatCard
                icon={Database}
                label="Entries"
                value={status.entry_count}
                subValue={`/ ${status.max_entries}`}
                delay={0.1}
              />
              <StatCard
                icon={Cpu}
                label="Version"
                value={status.version}
                delay={0.2}
              />
              <StatCard
                icon={Activity}
                label="Capacity"
                value={`${Math.round((status.entry_count / status.max_entries) * 100)}%`}
                delay={0.3}
              />
            </div>

            {/* Search */}
            <motion.div
              initial={{ opacity: 0, y: 20 }}
              animate={{ opacity: 1, y: 0 }}
              transition={{ delay: 0.4 }}
            >
              <SearchKey />
            </motion.div>

            {/* Memory Grid */}
            <motion.div
              initial={{ opacity: 0, y: 20 }}
              animate={{ opacity: 1, y: 0 }}
              transition={{ delay: 0.5 }}
            >
              <MemoryGrid
                entries={status.entries}
                maxEntries={status.max_entries}
                version={status.version}
                entryCount={status.entry_count}
              />
            </motion.div>
          </>
        )}
      </main>

      {/* Footer */}
      <footer className="border-t border-border/30 mt-auto">
        <div className="container mx-auto px-4 py-4">
          <p className="text-xs text-muted-foreground text-center font-mono">
            POSIX Shared Memory KV Store • Polling every 2s
          </p>
        </div>
      </footer>
    </div>
  );
}
