DIRS = ParSolver SimpleShell
DIRS_MAKE = $(addprefix make., $(DIRS))
DIRS_CLEAN = $(addprefix clean., $(DIRS))
DIRS_DEBUG = $(addprefix debug., $(DIRS))

all: $(DIRS_MAKE)

$(DIRS_MAKE):
	@cd $(subst make.,,$@); make

clean: $(DIRS_CLEAN)

$(DIRS_CLEAN):
	@cd $(subst clean.,,$@); make clean

debug: $(DIRS_DEBUG)

$(DIRS_DEBUG):
	@cd $(subst debug.,,$@); make debug