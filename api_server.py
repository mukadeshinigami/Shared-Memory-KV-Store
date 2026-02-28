"""
FastAPI server for Shared Memory KV Store.

Provides REST API endpoints to interact with the C shared memory KV store
through Python ctypes wrapper.
"""

import os
import sys
from contextlib import asynccontextmanager
from pathlib import Path
from typing import Optional

from fastapi import FastAPI, HTTPException, Request
from fastapi.middleware.cors import CORSMiddleware
from fastapi.responses import JSONResponse
from fastapi.exceptions import RequestValidationError
from pydantic import BaseModel, ValidationError

from kv_store_wrapper import KVStoreWrapper


# Path to shared library (relative to this file)
BUILD_DIR = Path(__file__).parent / "build"
LIB_PATH = BUILD_DIR / "libshared_memory_kv.so"


# Global wrapper instance
kv_store: Optional[KVStoreWrapper] = None


@asynccontextmanager
async def lifespan(app: FastAPI):
    """
    Lifespan context manager for FastAPI.
    
    Handles initialization and cleanup of shared memory store.
    """
    global kv_store
    
    # Startup: Initialize shared memory store
    try:
        if not LIB_PATH.exists():
            raise FileNotFoundError(
                f"Library not found: {LIB_PATH}. "
                f"Run 'make libso' to build it."
            )
        
        kv_store = KVStoreWrapper(str(LIB_PATH))
        
        # Try to open existing store first, create if doesn't exist
        print(f"Attempting to open shared memory store at '{LIB_PATH}'...")
        if not kv_store.open():
            print("Shared memory not found or invalid, creating new store...")
            if not kv_store.create():
                print("FAILED to create shared memory store.", file=sys.stderr)
                raise RuntimeError("Failed to create shared memory store")
            else:
                print("Created new shared memory store successfully")
        else:
            print("Opened existing shared memory store successfully")
        
        # Final check
        if kv_store.get_status() is not None:
            print("KV Store initialized and verified successfully")
        else:
            print("WARNING: KV Store initialized but status check failed", file=sys.stderr)
        
    except Exception as e:
        print(f"ERROR: Failed to initialize KV store: {e}", file=sys.stderr)
        sys.exit(1)
    
    yield
    
    # Shutdown: Cleanup
    if kv_store:
        print("Cleaning up KV store...")
        kv_store.destroy()
        # Note: We don't unlink here - only the creator process should unlink
        # Unlinking is typically done manually or by a separate cleanup script


# Create FastAPI app with lifespan
app = FastAPI(
    title="Shared Memory KV Store API",
    description="REST API for C-based shared memory key-value store",
    version="1.0.0",
    lifespan=lifespan
)

# Configure CORS for frontend access
# Important: CORS middleware must be added before exception handlers
# to ensure CORS headers are included in error responses
app.add_middleware(
    CORSMiddleware,
    allow_origins=[
        "http://localhost:3000",  # Next.js dev server
        "http://127.0.0.1:3000",
    ],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
    expose_headers=["*"],
)


# Exception handler for HTTPException to ensure CORS headers
# Note: CORSMiddleware should handle this automatically, but this ensures
# CORS headers are present even if middleware fails
@app.exception_handler(HTTPException)
async def http_exception_handler(request: Request, exc: HTTPException):
    """
    Handler for HTTPException to ensure CORS headers are included.
    CORSMiddleware should add them automatically, but this provides a fallback.
    """
    # Get origin from request headers
    origin = request.headers.get("origin")
    
    # Build response with CORS headers
    response = JSONResponse(
        status_code=exc.status_code,
        content={"detail": exc.detail}
    )
    
    # Add CORS headers if origin is in allowed list
    if origin in ["http://localhost:3000", "http://127.0.0.1:3000"]:
        response.headers["Access-Control-Allow-Origin"] = origin
        response.headers["Access-Control-Allow-Credentials"] = "true"
        response.headers["Access-Control-Allow-Methods"] = "*"
        response.headers["Access-Control-Allow-Headers"] = "*"
    
    return response


# Request/Response models
class SetRequest(BaseModel):
    """Request model for POST /set"""
    key: str
    value: str


class SetResponse(BaseModel):
    """Response model for POST /set"""
    success: bool
    message: str


class GetResponse(BaseModel):
    """Response model for GET /get/{key}"""
    key: str
    value: str


class StatusResponse(BaseModel):
    """Response model for GET /status"""
    version: int
    entry_count: int
    max_entries: int
    entries: list[dict]


@app.get("/")
async def root():
    """Root endpoint with API information."""
    return {
        "name": "Shared Memory KV Store API",
        "version": "1.0.0",
        "endpoints": {
            "GET /get/{key}": "Get value by key",
            "POST /set": "Set key-value pair",
            "GET /status": "Get store status and all entries"
        }
    }


@app.get("/get/{key}", response_model=GetResponse)
async def get_value(key: str):
    """
    Get value by key from the store.
    
    Args:
        key: Key to retrieve
        
    Returns:
        JSON with key and value
        
    Raises:
        HTTPException: If key not found or error occurs
    """
    if kv_store is None:
        raise HTTPException(status_code=503, detail="Store not initialized")
    
    value, error = kv_store.get(key)
    
    if error:
        if "not found" in error.lower():
            raise HTTPException(status_code=404, detail=error)
        raise HTTPException(status_code=500, detail=error)
    
    return GetResponse(key=key, value=value)


@app.post("/set", response_model=SetResponse)
async def set_value(request: SetRequest):
    """
    Set key-value pair in the store.
    
    Args:
        request: JSON body with key and value
        
    Returns:
        JSON with success status and message
        
    Raises:
        HTTPException: If operation fails
    """
    if kv_store is None:
        raise HTTPException(status_code=503, detail="Store not initialized")
    
    success, error = kv_store.set(request.key, request.value)
    
    if not success:
        status_code = 400
        if "too long" in error.lower():
            status_code = 413  # Payload Too Large
        elif "full" in error.lower() or "ENOSPC" in error:
            status_code = 507  # Insufficient Storage
        raise HTTPException(status_code=status_code, detail=error)
    
    return SetResponse(
        success=True,
        message=f"Key '{request.key}' set successfully"
    )


@app.get("/status", response_model=StatusResponse)
async def get_status():
    """
    Get store status including version, entry count, and all entries.
    
    Returns:
        JSON with store status and all key-value pairs
        
    Raises:
        HTTPException: If store not initialized or error occurs
    """
    if kv_store is None:
        raise HTTPException(status_code=503, detail="Store not initialized")
    
    try:
        status = kv_store.get_status()
        
        if status is None:
            raise HTTPException(status_code=500, detail="Failed to get status")
        
        # Validate and create response model
        # This may raise ValidationError if data structure is invalid
        return StatusResponse(**status)
    except Exception as e:
        # Re-raise HTTPException as-is (it will include CORS headers via middleware)
        if isinstance(e, HTTPException):
            raise
        # For unexpected errors, wrap in HTTPException
        raise HTTPException(
            status_code=500,
            detail=f"Internal server error: {str(e)}"
        )


if __name__ == "__main__":
    import uvicorn
    
    # Run server
    uvicorn.run(
        "api_server:app",
        host="0.0.0.0",
        port=8000,
        reload=False  # Disable reload in production
    )

