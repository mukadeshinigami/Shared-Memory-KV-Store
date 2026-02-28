"""
Python wrapper for shared_memory_kv C library using ctypes.

This module provides Python bindings for the C shared memory KV store
functions, allowing Python code to interact with the shared memory store.
"""

import ctypes
import os
import sys
from ctypes import Structure, c_char, c_int, c_uint, c_long, POINTER
from typing import Optional, Tuple


# Constants from shared_memory_kv.h
MAX_ENTRIES = 10
KEY_SIZE = 64
VALUE_SIZE = 256
SHM_NAME = "/gitflow_kv_store"


# C structure definitions using ctypes
class KVPair(Structure):
    """C structure: kv_pair_t"""
    _fields_ = [
        ("key", c_char * KEY_SIZE),
        ("value", c_char * VALUE_SIZE),
        ("timestamp", c_long),  # time_t is typically long
    ]


class SharedMemoryKVStore(Structure):
    """C structure: shared_memory_kv_store_t"""
    _fields_ = [
        ("kv_table", KVPair * MAX_ENTRIES),
        # sem_t is opaque, we'll use ctypes.c_byte array for its size
        # On Linux x86_64, sem_t is typically 32 bytes
        # On other systems it may vary, but 32 bytes should be sufficient
        # for most POSIX-compliant systems
        ("sem", ctypes.c_byte * 32),  # Size for sem_t (platform-dependent)
        ("version", c_uint),
        ("entry_count", c_uint),
    ]


class KVStoreWrapper:
    """
    Wrapper class for shared memory KV store C functions.
    
    Handles loading the shared library and providing Python-friendly
    interface to C functions.
    """
    
    def __init__(self, lib_path: str):
        """
        Initialize wrapper and load C library.
        
        Args:
            lib_path: Path to libshared_memory_kv.so
        """
        if not os.path.exists(lib_path):
            raise FileNotFoundError(f"Library not found: {lib_path}")
        
        # Load shared library
        self.lib = ctypes.CDLL(lib_path)
        
        # Define function signatures
        self._setup_functions()
        
        # Store state
        self.store_ptr = None
        self.fd = ctypes.c_int(-1)
        
    def _check_store(self) -> bool:
        """
        Check if store is initialized and pointer is not NULL.
        
        Returns:
            True if store is valid, False otherwise
        """
        if self.store_ptr is None:
            return False
            
        # In ctypes, bool(pointer) is False if pointer is NULL
        if not self.store_ptr:
            self.store_ptr = None
            return False
            
        return True
    
    def _setup_functions(self):
        """Configure C function signatures for ctypes."""
        
        # shared_memory_kv_create
        self.lib.shared_memory_kv_create.argtypes = [POINTER(c_int)]
        self.lib.shared_memory_kv_create.restype = POINTER(SharedMemoryKVStore)
        
        # shared_memory_kv_open
        self.lib.shared_memory_kv_open.argtypes = [POINTER(c_int)]
        self.lib.shared_memory_kv_open.restype = POINTER(SharedMemoryKVStore)
        
        # shared_memory_kv_destroy
        self.lib.shared_memory_kv_destroy.argtypes = [c_int, POINTER(SharedMemoryKVStore)]
        self.lib.shared_memory_kv_destroy.restype = None
        
        # shared_memory_kv_unlink
        self.lib.shared_memory_kv_unlink.argtypes = []
        self.lib.shared_memory_kv_unlink.restype = c_int
        
        # shared_memory_kv_set
        self.lib.shared_memory_kv_set.argtypes = [
            POINTER(SharedMemoryKVStore),
            ctypes.c_char_p,
            ctypes.c_char_p
        ]
        self.lib.shared_memory_kv_set.restype = c_int
        
        # shared_memory_kv_get
        self.lib.shared_memory_kv_get.argtypes = [
            POINTER(SharedMemoryKVStore),
            ctypes.c_char_p,
            ctypes.c_char_p
        ]
        self.lib.shared_memory_kv_get.restype = c_int
        
        # shared_memory_kv_delete
        self.lib.shared_memory_kv_delete.argtypes = [
            POINTER(SharedMemoryKVStore),
            ctypes.c_char_p
        ]
        self.lib.shared_memory_kv_delete.restype = c_int
    
    def create(self) -> bool:
        """
        Create new shared memory store.
        
        Returns:
            True on success, False on error
        """
        fd_ptr = ctypes.pointer(self.fd)
        self.store_ptr = self.lib.shared_memory_kv_create(fd_ptr)
        
        # Check for NULL pointer
        if not self._check_store():
            return False
        
        return True
    
    def open(self) -> bool:
        """
        Open existing shared memory store.
        
        Returns:
            True on success, False on error
        """
        fd_ptr = ctypes.pointer(self.fd)
        self.store_ptr = self.lib.shared_memory_kv_open(fd_ptr)
        
        # Check for NULL pointer
        if not self._check_store():
            return False
        
        return True
    
    def set(self, key: str, value: str) -> Tuple[bool, Optional[str]]:
        """
        Set key-value pair in store.
        
        Args:
            key: Key string
            value: Value string
            
        Returns:
            Tuple of (success: bool, error_message: Optional[str])
        """
        if not self._check_store():
            return False, "Store not initialized"
        
        # Convert strings to bytes
        key_bytes = key.encode('utf-8')
        value_bytes = value.encode('utf-8')
        
        # Check lengths
        if len(key_bytes) >= KEY_SIZE:
            return False, f"Key too long (max {KEY_SIZE-1} bytes)"
        if len(value_bytes) >= VALUE_SIZE:
            return False, f"Value too long (max {VALUE_SIZE-1} bytes)"
        
        result = self.lib.shared_memory_kv_set(
            self.store_ptr,
            key_bytes,
            value_bytes
        )
        
        if result == -1:
            # Get errno from C library
            errno_val = ctypes.get_errno()
            return False, f"Error setting key: errno={errno_val}"
        
        return True, None
    
    def get(self, key: str) -> Tuple[Optional[str], Optional[str]]:
        """
        Get value by key from store.
        
        Args:
            key: Key string
            
        Returns:
            Tuple of (value: Optional[str], error_message: Optional[str])
        """
        if not self._check_store():
            return None, "Store not initialized"
        
        key_bytes = key.encode('utf-8')
        
        if len(key_bytes) >= KEY_SIZE:
            return None, f"Key too long (max {KEY_SIZE-1} bytes)"
        
        # Allocate buffer for value
        value_buffer = ctypes.create_string_buffer(VALUE_SIZE)
        
        result = self.lib.shared_memory_kv_get(
            self.store_ptr,
            key_bytes,
            value_buffer
        )
        
        if result == -1:
            errno_val = ctypes.get_errno()
            if errno_val == 2:  # ENOENT - key not found
                return None, "Key not found"
            return None, f"Error getting key: errno={errno_val}"
        
        # Convert bytes to string
        value_str = value_buffer.value.decode('utf-8')
        return value_str, None
    
    def get_status(self) -> Optional[dict]:
        """
        Get store status (version, entry_count, all entries).
        
        Returns:
            Dictionary with status info, or None on error
        """
        if not self._check_store():
            return None
        
        try:
            store = self.store_ptr.contents
        except (ValueError, AttributeError, TypeError) as e:
            # Handle NULL pointer access or invalid pointer
            print(f"Error accessing store pointer contents: {e}", file=sys.stderr)
            self.store_ptr = None
            return None
        
        entries = []
        for i in range(MAX_ENTRIES):
            pair = store.kv_table[i]
            # Check if entry is not empty
            if pair.key:
                key_str = pair.key.decode('utf-8')
                value_str = pair.value.decode('utf-8')
                entries.append({
                    "key": key_str,
                    "value": value_str,
                    "timestamp": pair.timestamp
                })
        
        return {
            "version": store.version,
            "entry_count": store.entry_count,
            "max_entries": MAX_ENTRIES,
            "entries": entries
        }
    
    def destroy(self):
        """Clean up resources (munmap, close fd)."""
        if self._check_store():
            self.lib.shared_memory_kv_destroy(self.fd, self.store_ptr)
            self.store_ptr = None
            self.fd.value = -1
    
    def unlink(self) -> bool:
        """
        Unlink shared memory object (only creator should call this).
        
        Returns:
            True on success, False on error
        """
        result = self.lib.shared_memory_kv_unlink()
        return result == 0

