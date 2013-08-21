EXECUTABLE = raytracer
LIBS = DerelictUtil DerelictSDL2
IMPORTS = ../../Derelict3/import

# DMD
# DC=dmd
# DFLAGS = -O -noboundscheck -inline -release
# DFLAGS += -of$(EXECUTABLE)
# ifeq ($(OS),Windows_NT)
# LDFLAGS = $(patsubst %,$(LIBPATH)/%.lib,$(LIBS))
# else
# LDFLAGS = $(patsubst %,$(LIBPATH)/lib%.a,$(LIBS))
# endif
# LIBPATH = ../../code/Derelict3/lib/dmd

# GDC
DC = gdc
DFLAGS = -O3 -fno-bounds-check -frelease
DFLAGS += -o $(EXECUTABLE)
LDFLAGS = $(addprefix -Xlinker -L,$(LIBPATH)) $(addprefix -Xlinker -l,$(LIBS))
LIBPATH = ../../Derelict3/lib/gdc

# LDC
# DC = ldc
# DFLAGS = -O0 -g
# DFLAGS += -of=$(EXECUTABLE)
# LDFLAGS = $(addprefix -L=-L,$(LIBPATH)) $(addprefix -L=-l,$(LIBS))
# LIBPATH = ../../Derelict3/lib/ldc

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
