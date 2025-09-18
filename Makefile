INPUT_DIR = traces
OUTPUT_DIR = outputs
EXPECTED_DIR = expected_outputs
BIN_DIR = bin

EXEC = $(BIN_DIR)/memsim


build: memsim.c | $(BIN_DIR)
	gcc -o $(EXEC) memsim.c

test: $(OUTPUT_DIR)
	./$(EXEC) ./$(INPUT_DIR)/trace1 4 esc quiet > ./$(OUTPUT_DIR)/trace1-4frames-clock
	./$(EXEC) ./$(INPUT_DIR)/trace1 4 lru quiet > ./$(OUTPUT_DIR)/trace1-4frames-lru
	./$(EXEC) ./$(INPUT_DIR)/trace1 6 esc quiet > ./$(OUTPUT_DIR)/trace2-6frames-clock
	./$(EXEC) ./$(INPUT_DIR)/trace1 6 lru quiet > ./$(OUTPUT_DIR)/trace2-6frames-lru
	./$(EXEC) ./$(INPUT_DIR)/trace1 4 esc quiet > ./$(OUTPUT_DIR)/trace3-4frames-clock
	./$(EXEC) ./$(INPUT_DIR)/trace1 4 lru quiet > ./$(OUTPUT_DIR)/trace3-4frames-lru
	./$(EXEC) ./$(INPUT_DIR)/trace1 8 esc quiet > ./$(OUTPUT_DIR)/trace3-8frames-clock
	./$(EXEC) ./$(INPUT_DIR)/trace1 8 lru quiet > ./$(OUTPUT_DIR)/trace3-8frames-lru

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(OUTPUT_DIR):
	mkdir -p $(OUTPUT_DIR)

clean:
	rm -rf $(BIN_DIR) $(OUTPUT_DIR)