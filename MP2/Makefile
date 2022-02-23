TARGET = Server Client
CC = gcc
SERV_SRC = Server_Socket/src
SERV_INC = Server_Socket/include
SERV_OBJ = Server_Socket/obj
CLNT_SRC = Client_Socket/src
CLNT_OBJ = Client_Socket/obj
COMN_SRC = Common/src
COMN_INC = Common/include
COMN_OBJ = Common/obj

SERV_SOURCES := $(wildcard $(SERV_SRC)/*.c)
SERV_OBJECTS := $(patsubst $(SERV_SRC)/%.c, $(SERV_OBJ)/%.o, $(SERV_SOURCES))
CLNT_SOURCES := $(wildcard $(CLNT_SRC)/*.c)
CLNT_OBJECTS := $(patsubst $(CLNT_SRC)/%.c, $(CLNT_OBJ)/%.o, $(CLNT_SOURCES))
COMN_SOURCES := $(wildcard $(COMN_SRC)/*.c)
COMN_OBJECTS := $(patsubst $(COMN_SRC)/%.c, $(COMN_OBJ)/%.o, $(COMN_SOURCES))

CFLAGS = -std=gnu99 -I$(SERV_INC) -I$(COMN_INC)

all: $(TARGET)

Server: $(SERV_OBJECTS) $(COMN_OBJECTS)
	$(CC) $^ -o $@

$(SERV_OBJ)/%.o: $(SERV_SRC)/%.c
	mkdir -p $(SERV_OBJ)
	$(CC) $(CFLAGS) -c $< -o $@

Client: $(CLNT_OBJECTS) $(COMN_OBJECTS)
	$(CC) $^ -o Client_1
	$(CC) $^ -o Client_2
	$(CC) $^ -o Client_3

$(CLNT_OBJ)/%.o: $(CLNT_SRC)/%.c
	mkdir -p $(CLNT_OBJ)
	$(CC) $(CFLAGS) -c $< -o $@

Common: $(COMN_OBJECTS)
	$(CC) $^ -o $@

$(COMN_OBJ)/%.o: $(COMN_SRC)/%.c
	mkdir -p $(COMN_OBJ)
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean
clean:
	rm -rfv $(CLNT_OBJ) $(SERV_OBJ) $(COMN_OBJ) $(TARGET) ./Client_[0-9]*
