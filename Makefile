.PHONY: all clean img makeImg cleanImg

CC = g++ -g -std=c++11
INC_PATH = ./include/
SRC_PATH = ./src/
OBJ_PATH = ./obj/
SRC = $(wildcard $(SRC_PATH)*.cpp)
OBJ = $(patsubst $(SRC_PATH)%.cpp, $(OBJ_PATH)%.o, $(SRC))

HEADER = $(wildcard $(INC_PATH)*.h)
TARGET = SecondFileSystem

IMGSRC = ./src/main.cpp ./src/INode.cpp ./src/FileSystem.cpp ./include/INode.h ./include/FileSystem.h ./include/Buf.h ./include/BufferManager.h ./src/BufferManager.cpp ./src/Img.cpp ./include/Img.h

# all: $(TARGET)

# $(TARGET): $(OBJ)
# 	$(CC) -I$(INC_PATH) -o $@ $^

# $(OBJ):$(OBJ_PATH)%.o: $(SRC_PATH)%.cpp $(HEADER)
# 	$(CC) -I$(INC_PATH) -o $@ $<

all:
	$(CC) -I$(INC_PATH) -o $(TARGET) $(SRC) 

clean:
	rm -rf $(TARGET) *.img core.Second*
# rm -rf $(OBJ)*.o $(TARGET)

# user:
# g++ -o user ./src/User.cpp ./include/User.h ./inlcude/File.h ./include/INode.h ./include/FileManager.h

img: 
	g++ -o makeImg $(IMGSRC) -g

#./include/INode.h ./include/FileSystem.h

makeImg: makeImg
	@./makeImg

cleanImg: 
	rm -rf makeImg *.img core.makeImg.*