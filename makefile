
# FLAGS = -std=c++2a -stdlib=libc++ -Wall -Wextra -g -fsanitize=address
FLAGS = -std=c++2a -stdlib=libc++ -Wall -Wextra -g
LIBS = -lglfw -lGLEW  
FRAMEWORKS = -Ivendor/ -framework CoreVideo -framework OpenGL -framework IOKit -framework Cocoa -framework Carbon

LIB_SRC_DIR := ./engine
LIB_SRC_FILES := $(wildcard $(LIB_SRC_DIR)/*.cpp)
LIB_OBJ_DIR:= ./output/engine
LIB_OBJ_FILES := $(patsubst $(LIB_SRC_DIR)/%.cpp, $(LIB_OBJ_DIR)/%.o, $(LIB_SRC_FILES))
LIB_H_FILES := $(wildcard $(LIB_SRC_DIR)/*.h)
LIB_D_FILES := $(patsubst $(LIB_SRC_DIR)/%.h,$(LIB_OBJ_DIR)/%.d,$(LIB_SRC_FILES))
LIBRARY := ./output/libengine.a

SRC_DIR := ./super
OBJ_DIR := ./output/super
SRC_FILES := $(wildcard $(SRC_DIR)/*.cpp)
OBJ_FILES := $(patsubst $(SRC_DIR)/%.cpp, $(OBJ_DIR)/%.o, $(SRC_FILES))
DEPENDS := $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.d,$(SRC_FILES))

EXE_DIR := $(OBJ_DIR)
EXE := $(OBJ_DIR)/super.exe

all: super

super: pch $(LIBRARY) $(OBJ_FILES)
	clang++ $(FLAGS) $(LIBS) $(FRAMEWORKS) -o $(EXE) ./super/main.cpp $(LIBRARY)
	DEBUG=123 ./$(EXE)

pch: $(LIB_H_FILES)
	clang++ -c engine/pch.hpp -o ./output/pch.d $(FLAGS) 

$(LIBRARY): $(LIB_OBJ_FILES) 
	ar rcs $(LIBRARY) $(LIB_OBJ_FILES)

$(LIB_OBJ_DIR)/%.o: $(LIB_SRC_DIR)/%.cpp 
	clang++ $(FLAGS) -MMD -MP -c $< -o $@ 

$(LIB_OBJ_DIR)/%.d: $(LIB_SRC_DIR)/%.h
	clang++ $(FLAGS) -MMD -MP -c $< -o $@ 

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp 
	clang++ $(FLAGS) -MMD -MP -c $< -o $@ 

clean:
	$(RM) $(OBJ_FILES) $(LIB_OBJ_FILES) $(DEPENDS) ${LIBRARY}

gettrace:
	rm -f super.exe.trace
	../apitracedyld/build/apitrace trace --api gl $(EXE)

viewtrace:
	../apitrace/build/qapitrace super.exe.trace

.PHONY: all clean
