import ctypes
from ctypes import POINTER, Structure, c_int

class Foo(Structure):
    _fields_ = [("a", c_int)]

def test_null_pointer():
    P = POINTER(Foo)
    p = P()  # This creates a NULL pointer
    
    print(f"p: {p}")
    print(f"bool(p): {bool(p)}")
    
    try:
        void_ptr = ctypes.cast(p, ctypes.c_void_p)
        print(f"void_ptr.value: {void_ptr.value}")
        if void_ptr.value is None or void_ptr.value == 0:
            print("Detected NULL via cast")
        else:
            print(f"Accessing contents: {p.contents}")
    except ValueError as e:
        print(f"Caught ValueError in cast/access: {e}")

    try:
        if not p:
            print("Detected NULL via bool(p)")
        else:
            print(f"Accessing contents: {p.contents}")
    except ValueError as e:
        print(f"Caught ValueError in access: {e}")

if __name__ == "__main__":
    test_null_pointer()
