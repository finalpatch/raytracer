EXECUTABLE = raytracer
LIBS = mingw32 SDLmain SDL

CC = gcc
CXX = g++
CFLAGS = -O3 -march=native
CXXFLAGS = $(CFLAGS) -std=c++11
LD = $(CXX)
LDFLAGS =

RM-F = rm -f

SOURCE = $(wildcard *.c) $(wildcard *.cpp)
OBJS = $(patsubst %.c,%.o,$(patsubst %.cpp,%.o,$(SOURCE)))
DEPS = $(patsubst %.o,%.d,$(OBJS))
MISSING_DEPS = $(filter-out $(wildcard $(DEPS)),$(DEPS))
MISSING_DEPS_SOURCES = $(wildcard $(patsubst %.d,%.c,$(MISSING_DEPS)) \
$(patsubst %.d,%.cpp,$(MISSING_DEPS)))
CPPFLAGS += -MD

.PHONY : all deps objs clean veryclean rebuild

all : $(EXECUTABLE)

deps : $(DEPS)

objs : $(OBJS)

clean :
	$(RM-F) *.o
	$(RM-F) *.d

veryclean: clean
	$(RM-F) $(EXECUTABLE)

rebuild: veryclean all

ifneq ($(MISSING_DEPS),)
$(MISSING_DEPS) :
	$(RM-F) $(patsubst %.d,%.o,$@)
endif

-include $(DEPS)

$(EXECUTABLE) : $(OBJS)
	$(LD) $(LDFLAGS) -o $(EXECUTABLE) $(OBJS) $(addprefix -l,$(LIBS))
