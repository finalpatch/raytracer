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
DC=~/Apps/gdc/bin/gdc
DFLAGS = -O3 -fno-bounds-check -frelease
DFLAGS += -o $(EXECUTABLE)
LDFLAGS = $(addprefix -Xlinker -L,$(LIBPATH)) $(addprefix -Xlinker -l,$(LIBS))

RM-F = rm -f
SOURCE = $(wildcard *.d)

all : $(EXECUTABLE)

clean :
	$(RM-F) *.obj
	$(RM-F) *.exe
	$(RM-F) *.o

rebuild: clean all

$(EXECUTABLE) : $(SOURCE)
	$(DC) $(DFLAGS) -I$(IMPORTS) $(SOURCE) $(LDFLAGS)
