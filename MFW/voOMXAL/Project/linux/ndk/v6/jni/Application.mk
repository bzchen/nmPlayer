# Build both ARMv5TE and ARMv7-A machine code.
APP_CFLAGS := -O3
APP_ABI := armeabi-v7a

# release/debug
APP_OPTIM?= debug

# VisualOn Info
VOMODVER ?= 3.0.0.0001
VOBRANCH ?= trunk 
VOSRCNO ?= 11803 

VOBUILDTOOL ?= NDKr7b
VOBUILDNUM ?= 0000
VOGPVER ?= 3.3.18