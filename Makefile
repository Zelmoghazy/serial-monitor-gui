CC=g++

OPT=-O0
DBG=-g
WARNINGS=
DEPFLAGS=-MP -MD

INCS=$(foreach DIR,$(INC_DIRS),-I$(DIR))
LIBS=$(foreach DIR,$(LIB_DIRS),-L$(DIR))
LIBS+=-lopengl32 -lglfw3 -lglew32 -lgdi32 -limm32 -lcomdlg32
CFLAGS=$(DBG) $(OPT) $(INCS) $(LIBS) $(WARNINGS) $(DEPFLAGS)

INC_DIRS=./inc/ ./external/inc/ ./external/inc/GL/ ./external/inc/GLFW/ ./external/inc/IMGUI/ 
LIB_DIRS=./external/lib/
BUILD_DIR=build
CODE_DIRS=. src external/src
VPATH=$(CODE_DIRS)

SRC=$(foreach DIR,$(CODE_DIRS),$(wildcard $(DIR)/*.cpp))

OBJ=$(addprefix $(BUILD_DIR)/,$(notdir $(SRC:.cpp=.o)))
DEP=$(addprefix $(BUILD_DIR)/,$(notdir $(SRC:.cpp=.d)))


EXEC=Main.exe

all: $(BUILD_DIR)/$(EXEC)
	@echo "========================================="
	@echo "              BUILD SUCCESS              "
	@echo "========================================="

$(BUILD_DIR)/%.o: %.cpp | $(BUILD_DIR)
	$(CC) -c  $< -o $@ $(CFLAGS)

$(BUILD_DIR)/$(EXEC): $(OBJ)
	$(CC)  $^ -o $@ $(CFLAGS)

$(BUILD_DIR):
	mkdir $@
	$(foreach DIR,$(LIB_DIRS),cp $(DIR)*.dll  ./$(BUILD_DIR)/ ;)
	$(info SRC_DIRS : $(CODE_DIRS))
	$(info INC_DIRS : $(INC_DIRS))
	$(info INCS     : $(INCS))
	$(info SRC_FILES: $(SRC))
	$(info OBJ_FILES: $(OBJ))
	@echo "========================================="

clean:
	rm -fR $(BUILD_DIR)

-include $(DEP)

.PHONY: all clean