DIRS = SeqSolver SimpleShell
DIRS_MAKE = $(addprefix make., $(DIRS))
DIRS_CLEAN = $(addprefix clean., $(DIRS))

all: $(DIRS_MAKE)

$(DIRS_MAKE):
	@cd $(subst make.,,$@); make

clean: $(DIRS_CLEAN)

$(DIRS_CLEAN):
	@cd $(subst clean.,,$@); make clean