"use client";

import { useState, useCallback } from "react";
import { motion, AnimatePresence } from "framer-motion";
import { Plus, Key, FileText, Loader2, CheckCircle, AlertCircle } from "lucide-react";
import { Button } from "@/components/ui/button";
import {
  Dialog,
  DialogContent,
  DialogDescription,
  DialogFooter,
  DialogHeader,
  DialogTitle,
  DialogTrigger,
} from "@/components/ui/dialog";
import { Input } from "@/components/ui/input";
import { Label } from "@/components/ui/label";
import { useSetValue } from "@/lib/hooks";

interface SetValueDialogProps {
  onSuccess?: () => void;
}

/**
 * Modal dialog for setting key-value pairs
 */
export function SetValueDialog({ onSuccess }: SetValueDialogProps) {
  const [open, setOpen] = useState(false);
  const [key, setKey] = useState("");
  const [value, setValue] = useState("");
  
  const { 
    setValue: submitValue, 
    isSubmitting, 
    error, 
    successMessage, 
    clearMessages 
  } = useSetValue();

  const handleSubmit = useCallback(async (e: React.FormEvent) => {
    e.preventDefault();
    
    if (!key.trim() || !value.trim()) return;
    
    const success = await submitValue(key.trim(), value.trim());
    
    if (success) {
      // Clear form and close dialog after short delay
      setTimeout(() => {
        setKey("");
        setValue("");
        clearMessages();
        setOpen(false);
        onSuccess?.();
      }, 1000);
    }
  }, [key, value, submitValue, clearMessages, onSuccess]);

  const handleOpenChange = useCallback((newOpen: boolean) => {
    setOpen(newOpen);
    if (!newOpen) {
      // Reset form when closing
      setKey("");
      setValue("");
      clearMessages();
    }
  }, [clearMessages]);

  return (
    <Dialog open={open} onOpenChange={handleOpenChange}>
      <DialogTrigger asChild>
        <Button 
          className="gap-2 cyber-glow hover:cyber-glow-intense transition-all"
          size="lg"
        >
          <Plus className="h-5 w-5" />
          Set Value
        </Button>
      </DialogTrigger>
      
      <DialogContent className="sm:max-w-md border-primary/30 bg-card/95 backdrop-blur">
        <DialogHeader>
          <DialogTitle className="flex items-center gap-2 text-xl">
            <div className="p-2 rounded-lg bg-primary/10 border border-primary/30">
              <Plus className="h-5 w-5 text-primary" />
            </div>
            Set Key-Value Pair
          </DialogTitle>
          <DialogDescription>
            Add or update a value in the shared memory store.
          </DialogDescription>
        </DialogHeader>
        
        <form onSubmit={handleSubmit}>
          <div className="space-y-4 py-4">
            {/* Key input */}
            <div className="space-y-2">
              <Label htmlFor="key" className="flex items-center gap-2">
                <Key className="h-4 w-4 text-muted-foreground" />
                Key
              </Label>
              <Input
                id="key"
                placeholder="Enter key (max 63 chars)"
                value={key}
                onChange={(e) => setKey(e.target.value)}
                maxLength={63}
                disabled={isSubmitting}
                className="font-mono bg-background/50 border-border/50 focus:border-primary/50 focus:ring-primary/20"
              />
              <p className="text-xs text-muted-foreground text-right">
                {key.length}/63
              </p>
            </div>
            
            {/* Value input */}
            <div className="space-y-2">
              <Label htmlFor="value" className="flex items-center gap-2">
                <FileText className="h-4 w-4 text-muted-foreground" />
                Value
              </Label>
              <Input
                id="value"
                placeholder="Enter value (max 255 chars)"
                value={value}
                onChange={(e) => setValue(e.target.value)}
                maxLength={255}
                disabled={isSubmitting}
                className="font-mono bg-background/50 border-border/50 focus:border-primary/50 focus:ring-primary/20"
              />
              <p className="text-xs text-muted-foreground text-right">
                {value.length}/255
              </p>
            </div>
            
            {/* Status messages */}
            <AnimatePresence mode="wait">
              {error && (
                <motion.div
                  initial={{ opacity: 0, y: -10 }}
                  animate={{ opacity: 1, y: 0 }}
                  exit={{ opacity: 0, y: -10 }}
                  className="flex items-center gap-2 p-3 rounded-lg bg-destructive/10 border border-destructive/30 text-destructive text-sm"
                >
                  <AlertCircle className="h-4 w-4 flex-shrink-0" />
                  <span>{error.detail || error.message}</span>
                </motion.div>
              )}
              
              {successMessage && (
                <motion.div
                  initial={{ opacity: 0, y: -10 }}
                  animate={{ opacity: 1, y: 0 }}
                  exit={{ opacity: 0, y: -10 }}
                  className="flex items-center gap-2 p-3 rounded-lg bg-green-500/10 border border-green-500/30 text-green-400 text-sm"
                >
                  <CheckCircle className="h-4 w-4 flex-shrink-0" />
                  <span>{successMessage}</span>
                </motion.div>
              )}
            </AnimatePresence>
          </div>
          
          <DialogFooter>
            <Button
              type="button"
              variant="outline"
              onClick={() => handleOpenChange(false)}
              disabled={isSubmitting}
            >
              Cancel
            </Button>
            <Button
              type="submit"
              disabled={!key.trim() || !value.trim() || isSubmitting}
              className="gap-2"
            >
              {isSubmitting ? (
                <>
                  <Loader2 className="h-4 w-4 animate-spin" />
                  Saving...
                </>
              ) : (
                <>
                  <Plus className="h-4 w-4" />
                  Set Value
                </>
              )}
            </Button>
          </DialogFooter>
        </form>
      </DialogContent>
    </Dialog>
  );
}

export default SetValueDialog;


