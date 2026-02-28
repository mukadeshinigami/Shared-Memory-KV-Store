"use client";

import { useState, useCallback } from "react";
import { motion, AnimatePresence } from "framer-motion";
import { Search, Loader2, X, Copy, Check, AlertCircle } from "lucide-react";
import { Card, CardContent, CardHeader, CardTitle } from "@/components/ui/card";
import { Input } from "@/components/ui/input";
import { Button } from "@/components/ui/button";
import { useSearchKey } from "@/lib/hooks";

/**
 * Search component for looking up values by key
 */
export function SearchKey() {
  const [searchInput, setSearchInput] = useState("");
  const [copied, setCopied] = useState(false);
  
  const { search, result, isSearching, error, clear } = useSearchKey();

  const handleSearch = useCallback(async (e: React.FormEvent) => {
    e.preventDefault();
    if (!searchInput.trim()) return;
    await search(searchInput.trim());
  }, [searchInput, search]);

  const handleClear = useCallback(() => {
    setSearchInput("");
    clear();
  }, [clear]);

  const handleCopy = useCallback(async () => {
    if (!result?.value) return;
    
    try {
      await navigator.clipboard.writeText(result.value);
      setCopied(true);
      setTimeout(() => setCopied(false), 2000);
    } catch (err) {
      console.error("Failed to copy:", err);
    }
  }, [result?.value]);

  return (
    <Card className="border-border/50 bg-card/50 backdrop-blur">
      <CardHeader className="pb-4">
        <CardTitle className="flex items-center gap-2 text-lg">
          <Search className="h-5 w-5 text-primary" />
          <span>Search by Key</span>
        </CardTitle>
      </CardHeader>
      
      <CardContent className="space-y-4">
        {/* Search form */}
        <form onSubmit={handleSearch} className="flex gap-2">
          <div className="relative flex-1">
            <Input
              placeholder="Enter key to search..."
              value={searchInput}
              onChange={(e) => setSearchInput(e.target.value)}
              className="font-mono pr-8 bg-background/50 border-border/50 focus:border-primary/50 focus:ring-primary/20"
              disabled={isSearching}
            />
            {searchInput && (
              <button
                type="button"
                onClick={handleClear}
                className="absolute right-2 top-1/2 -translate-y-1/2 text-muted-foreground hover:text-foreground transition-colors"
              >
                <X className="h-4 w-4" />
              </button>
            )}
          </div>
          <Button 
            type="submit" 
            disabled={!searchInput.trim() || isSearching}
            className="gap-2"
          >
            {isSearching ? (
              <Loader2 className="h-4 w-4 animate-spin" />
            ) : (
              <Search className="h-4 w-4" />
            )}
            Search
          </Button>
        </form>
        
        {/* Results */}
        <AnimatePresence mode="wait">
          {/* Error state */}
          {error && (
            <motion.div
              initial={{ opacity: 0, y: -10 }}
              animate={{ opacity: 1, y: 0 }}
              exit={{ opacity: 0, y: -10 }}
              className="flex items-center gap-2 p-4 rounded-lg bg-destructive/10 border border-destructive/30"
            >
              <AlertCircle className="h-5 w-5 text-destructive flex-shrink-0" />
              <div>
                <p className="text-sm font-medium text-destructive">
                  {error.statusCode === 404 ? "Key not found" : "Search failed"}
                </p>
                <p className="text-xs text-destructive/70">
                  {error.detail || error.message}
                </p>
              </div>
            </motion.div>
          )}
          
          {/* Success state */}
          {result && (
            <motion.div
              initial={{ opacity: 0, y: -10 }}
              animate={{ opacity: 1, y: 0 }}
              exit={{ opacity: 0, y: -10 }}
              className="p-4 rounded-lg bg-primary/5 border border-primary/30 cyber-border"
            >
              <div className="flex items-start justify-between gap-4">
                <div className="space-y-2 flex-1 min-w-0">
                  <div className="flex items-center gap-2">
                    <span className="text-xs text-muted-foreground uppercase tracking-wider">
                      Key
                    </span>
                    <span className="font-mono text-sm text-primary font-semibold">
                      {result.key}
                    </span>
                  </div>
                  <div className="space-y-1">
                    <span className="text-xs text-muted-foreground uppercase tracking-wider">
                      Value
                    </span>
                    <p className="font-mono text-sm text-foreground bg-background/50 p-2 rounded border border-border/30 break-all">
                      {result.value}
                    </p>
                  </div>
                </div>
                
                {/* Copy button */}
                <Button
                  variant="ghost"
                  size="icon"
                  onClick={handleCopy}
                  className="flex-shrink-0"
                  title="Copy value"
                >
                  {copied ? (
                    <Check className="h-4 w-4 text-green-400" />
                  ) : (
                    <Copy className="h-4 w-4" />
                  )}
                </Button>
              </div>
            </motion.div>
          )}
        </AnimatePresence>
      </CardContent>
    </Card>
  );
}

export default SearchKey;


