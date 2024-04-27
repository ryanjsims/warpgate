from typing import Tuple
from numpy import int32, uint32, int8
import warnings


# Note: Forgelight uses a signed 32 bit int while performing the Jenkins hash, but represents it as unsigned
def oaat(key: bytes):
    with warnings.catch_warnings():
        warnings.filterwarnings("ignore", "overflow encountered in int_scalars", RuntimeWarning)
        warnings.filterwarnings("ignore", "overflow encountered in long_scalars", RuntimeWarning)
        i = 0
        hash = int32(0)
        while i < len(key):
            hash += int8(key[i])
            i += 1
            hash += int32(hash << 10)
            hash ^= int32(hash >> 6)
        hash += int32(hash << 3)
        hash ^= int32(hash >> 11)
        hash += int32(hash << 15)
        return int(uint32(hash))

def mix(a: int, b: int, c: int) -> Tuple[int, int, int]:
    a &= 0xFFFFFFFF
    b &= 0xFFFFFFFF
    c &= 0xFFFFFFFF

    a -= b
    a -= c
    a ^= c >> 13
    a &= 0xFFFFFFFF

    b -= c
    b -= a
    b ^= a << 8
    b &= 0xFFFFFFFF

    c -= a
    c -= b
    c ^= b >> 13
    c &= 0xFFFFFFFF

    a -= b
    a -= c
    a ^= c >> 12
    a &= 0xFFFFFFFF

    b -= c
    b -= a
    b ^= a << 16
    b &= 0xFFFFFFFF

    c -= a
    c -= b
    c ^= b >> 5
    c &= 0xFFFFFFFF

    a -= b
    a -= c
    a ^= c >> 3
    a &= 0xFFFFFFFF

    b -= c
    b -= a
    b ^= a << 10
    b &= 0xFFFFFFFF

    c -= a
    c -= b
    c ^= b >> 15
    c &= 0xFFFFFFFF

    return a, b, c

def lookup2(key: bytes, initval: int = 0):
    lenpos = len(key)
    length = lenpos
    a = b = 0x9e3779b9
    c = initval
    p = 0

    while lenpos >= 12:
        a += key[p+0] + key[p+1] << 8 + key[p+2] << 16 + key[p+3] << 24
        a &= 0xFFFFFFFF
        b += key[p+4] + key[p+5] << 8 + key[p+6] << 16 + key[p+7] << 24
        b &= 0xFFFFFFFF
        c += key[p+8] + key[p+9] << 8 + key[p+10] << 16 + key[p+11] << 24
        c &= 0xFFFFFFFF
        a, b, c = mix(a, b, c)
        p += 12
        lenpos -= 12
    
    c += length
    if (lenpos >= 11): c += key[p+10]<<24
    if (lenpos >= 10): c += key[p+9]<<16
    if (lenpos >= 9):  c += key[p+8]<<8
    # the first byte of c is reserved for the length
    if (lenpos >= 8):  b += key[p+7]<<24
    if (lenpos >= 7):  b += key[p+6]<<16
    if (lenpos >= 6):  b += key[p+5]<<8
    if (lenpos >= 5):  b += key[p+4]
    if (lenpos >= 4):  a += key[p+3]<<24
    if (lenpos >= 3):  a += key[p+2]<<16
    if (lenpos >= 2):  a += key[p+1]<<8
    if (lenpos >= 1):  a += key[p+0]

    a, b, c = mix(a, b, c)

    return c & 0xFFFFFFFF