CC=gcc
CFLAGS=-Wall -g
MMFLAGS=-I include/MM -I include/shared
PMFLAGS=-I include/PM -I include/shared
MM_SRC=src/MM/MM.c src/MM/MM_main.c src/MM/MM_tools.c src/MM/MM_comm.c
PM_SRC=src/PM/PM.c src/PM/PM_main.c
SHARED_SRC=src/shared/shared.c
BIN_DIR=bin

all: compile_MM compile_PM

run: compile_MM compile_PM
	./$(BIN_DIR)/MM -k 10 -f 200 -q 10

run_debug: compile_MM_debug compile_PM
	./$(BIN_DIR)/MM -k 10 -f 200 -q 5 -m 10

compile_MM: $(MM_SRC)
	@$(CC) $(CFLAGS) $(MMFLAGS) $(MM_SRC) $(SHARED_SRC) -o $(BIN_DIR)/MM

compile_PM: $(PM_SRC)
	@$(CC) $(CFLAGS) $(PMFLAGS) $(PM_SRC) $(SHARED_SRC) -o $(BIN_DIR)/PM

compile_MM_debug: $(MM_SRC)
	@$(CC) -DDEBUG=1 $(CFLAGS) $(MMFLAGS) $(MM_SRC) $(SHARED_SRC) -o $(BIN_DIR)/MM

clean: 
	rm -rf bin/*