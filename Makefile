# Compiler
CC = g++

# Include paths
CFLAGS = \
    -ID:/msys64/mingw64/include \
    -I./ffmpeg \
    -I./interface

# FFmpeg libraries
FFMPEG_LIBS = \
    -LD:/msys64/mingw64/lib \
    -lavformat \
    -lavcodec \
    -lavutil \
    -lswresample \
    -lswscale \
    -lavfilter \
    -lz \
    -lws2_32

# Other libraries
LDFLAGS = \
    -LD:/msys64/mingw64/lib \
    -lglew32 -lglfw3 -lopengl32 -lgdi32 \
    $(FFMPEG_LIBS)

# All source files
SRC = \
    main.cpp \
    ffmpeg/video_decoder.cpp \
    ffmpeg/video_encoder.cpp \
    ffmpeg/video_redecoder.cpp \
    interface/ui.cpp \
    interface/video_widget.cpp

# Output executable
OUT = main.exe

# Build rule
all:
	$(CC) $(CFLAGS) $(SRC) -o $(OUT) $(LDFLAGS)

# Clean rule
clean:
	rm -f $(OUT)
