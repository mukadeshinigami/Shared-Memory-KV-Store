"use client";

import { motion, AnimatePresence } from "framer-motion";
import { Database, Clock, Hash } from "lucide-react";
import { Card, CardContent, CardHeader, CardTitle } from "@/components/ui/card";
import { KVEntry } from "@/lib/api";

interface MemoryGridProps {
  entries: KVEntry[];
  maxEntries: number;
  version: number;
  entryCount: number;
}

/**
 * Memory cell component representing a single slot in shared memory
 */
function MemoryCell({ 
  entry, 
  index, 
  isOccupied 
}: { 
  entry?: KVEntry; 
  index: number; 
  isOccupied: boolean;
}) {
  const formatTimestamp = (timestamp: number) => {
    if (!timestamp) return "—";
    return new Date(timestamp * 1000).toLocaleTimeString();
  };

  return (
    <motion.div
      layout
      initial={{ opacity: 0, scale: 0.8 }}
      animate={{ 
        opacity: 1, 
        scale: 1,
        transition: { 
          type: "spring",
          stiffness: 300,
          damping: 25,
          delay: index * 0.05 
        }
      }}
      exit={{ opacity: 0, scale: 0.8 }}
      className={`
        relative p-4 rounded-lg border transition-all duration-300
        ${isOccupied 
          ? "bg-primary/10 border-primary/50 cyber-glow animate-pulse-glow" 
          : "bg-muted/30 border-border/50 opacity-50"
        }
      `}
    >
      {/* Cell index indicator */}
      <div className="absolute top-1 right-2 text-xs font-mono text-muted-foreground/50">
        [{index.toString().padStart(2, "0")}]
      </div>

      {isOccupied && entry ? (
        <AnimatePresence mode="wait">
          <motion.div
            key={entry.key}
            initial={{ opacity: 0, y: 10 }}
            animate={{ opacity: 1, y: 0 }}
            exit={{ opacity: 0, y: -10 }}
            transition={{ duration: 0.2 }}
            className="space-y-2"
          >
            {/* Key */}
            <div className="flex items-center gap-2">
              <Hash className="h-3 w-3 text-primary" />
              <span className="font-mono text-sm text-primary font-semibold truncate">
                {entry.key}
              </span>
            </div>
            
            {/* Value */}
            <div className="font-mono text-xs text-foreground/80 truncate pl-5">
              {entry.value}
            </div>
            
            {/* Timestamp */}
            <div className="flex items-center gap-1 text-xs text-muted-foreground pl-5">
              <Clock className="h-3 w-3" />
              <span>{formatTimestamp(entry.timestamp)}</span>
            </div>
          </motion.div>
        </AnimatePresence>
      ) : (
        <div className="h-16 flex items-center justify-center">
          <span className="text-xs font-mono text-muted-foreground/30">
            EMPTY
          </span>
        </div>
      )}

      {/* Data flow animation for occupied cells */}
      {isOccupied && (
        <div className="absolute inset-0 rounded-lg animate-data-flow opacity-30 pointer-events-none" />
      )}
    </motion.div>
  );
}

/**
 * Live Monitor component showing shared memory state as a grid
 */
export function MemoryGrid({ 
  entries, 
  maxEntries, 
  version, 
  entryCount 
}: MemoryGridProps) {
  // Create array of all slots (filled + empty)
  const slots = Array.from({ length: maxEntries }, (_, index) => {
    const entry = entries[index];
    return { entry, index, isOccupied: !!entry };
  });

  return (
    <Card className="border-border/50 bg-card/50 backdrop-blur">
      <CardHeader className="pb-4">
        <div className="flex items-center justify-between">
          <CardTitle className="flex items-center gap-2 text-lg">
            <Database className="h-5 w-5 text-primary" />
            <span>Live Memory Monitor</span>
          </CardTitle>
          
          {/* Status indicators */}
          <div className="flex items-center gap-4 text-sm">
            <div className="flex items-center gap-2">
              <div className="h-2 w-2 rounded-full bg-primary animate-pulse" />
              <span className="font-mono text-muted-foreground">
                v{version}
              </span>
            </div>
            <div className="font-mono text-muted-foreground">
              <span className="text-primary">{entryCount}</span>
              <span className="text-muted-foreground/50">/{maxEntries}</span>
            </div>
          </div>
        </div>
      </CardHeader>
      
      <CardContent>
        {/* Memory grid */}
        <div className="grid grid-cols-2 md:grid-cols-5 gap-3">
          {slots.map(({ entry, index, isOccupied }) => (
            <MemoryCell
              key={index}
              entry={entry}
              index={index}
              isOccupied={isOccupied}
            />
          ))}
        </div>
        
        {/* Legend */}
        <div className="flex items-center gap-6 mt-4 pt-4 border-t border-border/30 text-xs text-muted-foreground">
          <div className="flex items-center gap-2">
            <div className="h-3 w-3 rounded bg-primary/30 border border-primary/50 cyber-glow" />
            <span>Occupied</span>
          </div>
          <div className="flex items-center gap-2">
            <div className="h-3 w-3 rounded bg-muted/30 border border-border/50 opacity-50" />
            <span>Empty</span>
          </div>
        </div>
      </CardContent>
    </Card>
  );
}

export default MemoryGrid;


