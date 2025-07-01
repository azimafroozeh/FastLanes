# mk/embeddings.mk — Generate sentence embeddings
# -----------------------------------------------------------
# Targets:
#   make install      – create .venv & install deps
#   make generate     – run generate_embedding.py
#   make clean        – delete __pycache__
#   make venv-clean   – remove .venv entirely
# -----------------------------------------------------------

PROJECT_ROOT := $(abspath $(dir $(lastword $(MAKEFILE_LIST)))/..)
VENV_DIR     := $(PROJECT_ROOT)/.venv
SCRIPTS_DIR  := $(PROJECT_ROOT)/scripts
SCRIPT       := $(abspath $(SCRIPTS_DIR)/generate_embedding.py)

ifeq ($(OS),Windows_NT)
  VENV_PY := $(VENV_DIR)/Scripts/python.exe
else
  VENV_PY := $(VENV_DIR)/bin/python
endif

.DEFAULT_GOAL := help
.PHONY: help venv install generate clean-embeddings venv-clean

help:
	@echo "Targets:"
	@echo "  make install     – create .venv & install deps"
	@echo "  make generate    – run generate_embedding.py"
	@echo "  make clean       – delete __pycache__"
	@echo "  make venv-clean  – remove .venv"

# 1️⃣  Create virtual-env if missing (uses the interpreter first on PATH)
venv:
	@if [ ! -d "$(VENV_DIR)" ]; then \
		echo "Creating virtualenv with interpreter: $$(python --version)"; \
		python -m venv "$(VENV_DIR)"; \
	else \
		echo ".venv already exists."; \
	fi

# 2️⃣  Install (and auto-repair if the venv was built with Python 3.13)
install: venv
	@if [ -f "$(VENV_DIR)/pyvenv.cfg" ] && \
	    grep -qE '^version = 3\.13' "$(VENV_DIR)/pyvenv.cfg"; then \
	    echo "⚠️  .venv uses Python 3.13 — recreating with ‘python’ on PATH"; \
	    rm -rf "$(VENV_DIR)"; \
	    python -m venv "$(VENV_DIR)"; \
	fi

	"$(VENV_PY)" -m pip install --upgrade pip
	"$(VENV_PY)" -m pip install torch torchvision numpy pandas pyarrow \
	              --extra-index-url https://download.pytorch.org/whl/cpu

# 3️⃣  Generate embeddings
generate: install
	@echo "Running generate_embedding.py with $$( "$(VENV_PY)" --version ) …"
	"$(VENV_PY)" "$(SCRIPT)"

# 4️⃣  House-keeping helpers
clean-embeddings:
	find . -type d -name "__pycache__" -exec rm -rf {} +

venv-clean:
	rm -rf "$(VENV_DIR)"
