EXECUTABLE = raytracer
LIBS = DerelictSDL DerelictUtil
IMPORTS = C:/Users/fengli/code/Derelict2/import
LIBPATH = C:/Users/fengli/code/Derelict2/lib

# DMD
# DC=~/Apps/dmd2/windows/bin/dmd
# DFLAGS = -O -noboundscheck -inline -release
# DFLAGS += -of$(EXECUTABLE)
# LDFLAGS = $(patsubst %,$(LIBPATH)/%.lib,$(LIBS))

# GDC
DC = ~/Apps/gdc/bin/gdc
DFLAGS = -O3 -fno-bounds-check -frelease
DFLAGS += -o $(EXECUTABLE)
LDFLAGS = $(addprefix -Xlinker -L,$(LIBPATH)) $(addprefix -Xlinker -l,$(LIBS))

# LDC
# DC = ldc
# DFLAGS = -O0 -g
# DFLAGS += -of=$(EXECUTABLE)
# LDFLAGS = $(addprefix -L=-L,$(LIBPATH)) $(addprefix -L=-l,$(LIBS))

# DMD on Mac
# DC=dmd
# DFLAGS = -O -noboundscheck -inline -release
# DFLAGS += -of$(EXECUTABLE)
# LDFLAGS = $(patsubst %,$(LIBPATH)/lib%.a,$(LIBS))

RM-F = rm -f
SOURCE = $(wildcard *.d)

all : $(EXECUTABLE)

clean :
	$(RM-F) *.obj
	$(RM-F) *.o
	$(RM-F) *.exe
	$(RM-F) $(EXECUTABLE)

rebuild: clean all

$(EXECUTABLE) : $(SOURCE)
	$(DC) $(DFLAGS) -I$(IMPORTS) $(SOURCE) $(LDFLAGS)
