# mk/compression_report.mk

# Locate python3 or fallback to python
PYTHON := $(shell command -v python3 2>/dev/null || command -v python 2>/dev/null)

# Path to the report generator script
SCRIPT := .github/scripts/compression_report.py

# Default CSV inputs and output MD
BASELINE   ?= dev-compression.csv
CURRENT    ?= curr-compression.csv
REPORT     ?= report.md

.PHONY: all report

all: report

report: $(BASELINE) $(CURRENT) $(SCRIPT)
	@echo "Generating compression regression report..."
	@$(PYTHON) $(SCRIPT) \
	  --baseline $(BASELINE) \
	  --current  $(CURRENT)  \
	  --out      $(REPORT)
	@echo "Written $(REPORT)"

