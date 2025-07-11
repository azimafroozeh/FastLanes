# ----------------------------------------------------------
# Makefile
# ----------------------------------------------------------
#  Targets
#  -------
#    make / make run    Build venv (once) + generate floats_binary.txt
#    make deps          (Re)install packages from requirements.txt
#    make shell         Open a shell with the venv activated
#    make clean         Remove floats_binary.txt and the venv
#    make time          Time a full build
#    make help          Short target summary
# ----------------------------------------------------------

# -------- configurable variables --------------------------
# virtual-env directory
VENV ?= .venv

# executables inside the venv
PY  := $(VENV)/bin/python
PIP := $(VENV)/bin/pip

# script & output names
SCRIPT ?= generate_floats_binary.py
OUTPUT ?= floats_binary.txt

# sentinel so Make knows the venv is ready
STAMP := $(VENV)/.stamp
# ----------------------------------------------------------

# Default target ----------------------------------------------------------
.PHONY: all run
all: $(OUTPUT)
run: all

# ------------------------------------------------------------------------
# 1) Create venv once, upgrade pip, and mark with $(STAMP)
$(STAMP):
	@echo "Creating virtual env in $(VENV)…"
	@python3 -m venv $(VENV)
	@$(PIP) install --upgrade pip
	@touch $(STAMP)

# 2) Optional dependency installation ------------------------------------
.PHONY: deps
deps: $(STAMP)
ifneq ($(wildcard requirements.txt),)
	@echo "Installing dependencies from requirements.txt…"
	@$(PIP) install -r requirements.txt
else
	@echo "(No requirements.txt found – skipping dependency install)"
endif

# 3) Generate the binary-encoded floats -----------------------------------
$(OUTPUT): deps $(SCRIPT)
	@echo "Running $(SCRIPT)…"
	@$(PY) $(SCRIPT)

# ------------------------------------------------------------------------
# Developer utilities
.PHONY: shell
shell: deps
	@echo "Opening a new shell with the virtual env activated"
	@. $(VENV)/bin/activate && bash --login

.PHONY: time
time:
	@time $(MAKE) --no-print-directory

.PHONY: clean
clean:
	@echo "Removing $(OUTPUT) and $(VENV)…"
	@rm -rf $(OUTPUT) $(VENV)

.PHONY: help
help:
	@grep -E '^(##|#  )' Makefile | sed -e 's/^##\s*//' -e 's/^#  //'
# ----------------------------------------------------------
