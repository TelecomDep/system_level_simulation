INPUT_FILE = my_broker.cpp
OUTPUT_FILE = broker.out
LOG_FILE = broker_log.txt
CONFIGS_FILES = *.conf

all:
	@g++ -I.. -I/usr/local/include/srsran $(INPUT_FILE) -g -o $(OUTPUT_FILE) -lboost_program_options -lzmq

run:
	@./$(OUTPUT_FILE) $(CONFIGS_FILES)

test:
	@./$(OUTPUT_FILE) $(CONFIGS_FILES) > $(LOG_FILE)


