# ─────────────────────────────────────────────────────────────
# Generate FlatBuffers C++ code for footer schema (with DataType)
# ─────────────────────────────────────────────────────────────
# Relative path of this Makefile: mk/flatbuffers.mk

MK_DIR      := $(dir $(lastword $(MAKEFILE_LIST)))
PROJECT_ROOT:= $(abspath $(MK_DIR)/..)

# Point to the top‐level flatbuffers_schemas and output directory
FBS_DIR     := $(PROJECT_ROOT)/flatbuffers_schemas
FBS_OUT     := $(PROJECT_ROOT)/src/include/fls/footer

.PHONY: generate_footer
generate_footer:
	@echo "🔍 Checking for schema files..."
	@if [ ! -d "$(FBS_DIR)" ]; then \
	  echo "❌ Schema directory '$(FBS_DIR)' not found."; exit 1; \
	fi
	@if [ ! -f "$(FBS_DIR)/datatype.fbs" ]; then \
	  echo "❌ '$(FBS_DIR)/datatype.fbs' not found."; exit 1; \
	fi
	@if [ ! -f "$(FBS_DIR)/footer.fbs" ]; then \
	  echo "❌ '$(FBS_DIR)/footer.fbs' not found."; exit 1; \
	fi

	@echo "📦 Generating FlatBuffers C++ (footer + datatype)…"
	@command -v flatc >/dev/null || { \
	  echo "❌ 'flatc' not found. Please install the FlatBuffers compiler."; \
	  exit 1; \
	}
	@mkdir -p "$(FBS_OUT)"
	@flatc --cpp \
	       --gen-object-api \
	       --scoped-enums \
	       --no-emit-min-max-enum-values \
	       -I "$(FBS_DIR)" \
	       -o "$(FBS_OUT)" \
	       "$(FBS_DIR)/datatype.fbs" \
	       "$(FBS_DIR)/decimal_type.fbs" \
	       "$(FBS_DIR)/footer.fbs" \
	       "$(FBS_DIR)/operator_token.fbs" \
	       "$(FBS_DIR)/rpn.fbs" \
	       "$(FBS_DIR)/column_descriptor.fbs" \
	       "$(FBS_DIR)/rowgroup_descriptor.fbs" \
	       "$(FBS_DIR)/table_descriptor.fbs"

	@echo "✅ FlatBuffers C++ generated in $(FBS_OUT)"
