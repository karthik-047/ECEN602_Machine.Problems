TARGET = Server
CC = gcc
SERV_SRC = Server_Socket/src
SERV_INC = Server_Socket/include
SERV_OBJ = Server_Socket/obj
COMN_SRC = Common/src
COMN_INC = Common/include
COMN_OBJ = Common/obj

SERV_SOURCES := $(wildcard $(SERV_SRC)/*.c)
SERV_OBJECTS := $(patsubst $(SERV_SRC)/%.c, $(SERV_OBJ)/%.o, $(SERV_SOURCES))
COMN_SOURCES := $(wildcard $(COMN_SRC)/*.c)
COMN_OBJECTS := $(patsubst $(COMN_SRC)/%.c, $(COMN_OBJ)/%.o, $(COMN_SOURCES))

CFLAGS = -std=gnu99 -I$(SERV_INC) -I$(COMN_INC)

all: $(TARGET)

Server: $(SERV_OBJECTS) $(COMN_OBJECTS)
	$(CC) $^ -lm -o $@

$(SERV_OBJ)/%.o: $(SERV_SRC)/%.c
	mkdir -p $(SERV_OBJ)
	$(CC) $(CFLAGS) -c $< -lm -o $@


Common: $(COMN_OBJECTS)
	$(CC) $^ -o $@

$(COMN_OBJ)/%.o: $(COMN_SRC)/%.c
	mkdir -p $(COMN_OBJ)
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean
clean:
	rm -rfv $(SERV_OBJ) $(COMN_OBJ) $(TARGET) ./Client_[0-9]* ./temp_file.txt
