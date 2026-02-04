DOCS_DIR := docs
SCRIPTS_DIR := scripts
PROPOSALS_DIR := $(SCRIPTS_DIR)/proposals
OUT_PROPOSALS := $(DOCS_DIR)/proposals.pdf
IN_PROPOSALS := $(PROPOSALS_DIR)/proposals.typ

$(IN_PROPOSALS_DIR):
	@mkdir -p $(IN_PROPOSALS_DIR)

$(OUT_PROPOSALS_DIR):
	@mkdir -p $(OUT_PROPOSALS_DIR)

proposals: $(IN_PROPOSALS_DIR) | $(OUT_PROPOSALS_DIR)
	@echo Generating proposals...
	typst compile $(IN_PROPOSALS) $(OUT_PROPOSALS)
	@echo Proposals generated successfully.