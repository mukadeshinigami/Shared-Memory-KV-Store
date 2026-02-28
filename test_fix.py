from kv_store_wrapper import KVStoreWrapper
import os
from pathlib import Path

# Path to shared library (relative to this file)
BUILD_DIR = Path(__file__).parent / "build"
LIB_PATH = BUILD_DIR / "libshared_memory_kv.so"

def verify_fix():
    print("--- Starting Verification ---")
    
    if not LIB_PATH.exists():
        print(f"Error: Library not found at {LIB_PATH}. Please run 'make libso' first.")
        return

    # Initialize wrapper
    print(f"Loading library from {LIB_PATH}...")
    wrapper = KVStoreWrapper(str(LIB_PATH))
    
    # store_ptr is initially None
    print(f"Store initialized? {wrapper._check_store()}")
    
    # Test get_status on None ptr
    print("Testing get_status() with store_ptr as None...")
    status = wrapper.get_status()
    print(f"Status (should be None): {status}")
    assert status is None, "status should be None when store_ptr is None"
    
    # Manually set store_ptr to a NULL pointer object (LP_SharedMemoryKVStore)
    # This is what happened in the crash
    print("Setting store_ptr to a NULL LP_SharedMemoryKVStore object...")
    import ctypes
    from kv_store_wrapper import SharedMemoryKVStore
    wrapper.store_ptr = ctypes.POINTER(SharedMemoryKVStore)()
    
    print(f"Store valid? {wrapper._check_store()}")
    assert wrapper._check_store() is False, "_check_store should return False for NULL pointer object"
    
    # Test get_status on NULL ptr object
    print("Testing get_status() with NULL store_ptr object...")
    try:
        status = wrapper.get_status()
        print(f"Status (should be None): {status}")
        assert status is None, "status should be None when store_ptr is NULL"
        print("SUCCESS: get_status() handled NULL pointer object gracefully.")
    except ValueError as e:
        print(f"FAILED: get_status() raised ValueError: {e}")
    except Exception as e:
        print(f"FAILED: get_status() raised unexpected exception: {e}")

    # Test other methods
    print("Testing set() with NULL store_ptr...")
    success, error = wrapper.set("test", "value")
    print(f"Set result: {success}, Error: {error}")
    assert success is False
    assert "not initialized" in error
    
    print("Testing get() with NULL store_ptr...")
    value, error = wrapper.get("test")
    print(f"Get result: {value}, Error: {error}")
    assert value is None
    assert "not initialized" in error

    print("--- Verification Completed Successfully ---")

if __name__ == "__main__":
    verify_fix()
