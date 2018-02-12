# folders required throughout rules
STY_DIR = ../sty
FORMAT_DIR = ../metafiles/format
OUT_DIR = texfiles
CODE_DIR = code
IMG_DIR = img

PDFLATEX = pdflatex


#
# PDF output files
# * cheatsheet
# * abi
#
# BASENAME is defined in top-level Makefile including this file.
#
TARGET = $(addsuffix .pdf, $(BASENAME))

.PHONY: main all clean
.PHONY: target

# If running `make' with no arguments, only generate slides.
main: target

all: target

# Phony targets depend on PDF output files.
target: $(TARGET)

TARGET_TEX = $(addsuffix .tex, $(BASENAME))

$(TARGET): $(TARGET_TEX)
	# Rebuild source files, if any.
	-test -d $(CODE_DIR) && make -C $(CODE_DIR)
	# Rebuild image files, if any.
	-test -d $(IMG_DIR) && make -C $(IMG_DIR)
	# Create out directory.
	-test -d $(OUT_DIR) || mkdir $(OUT_DIR)
	# Run twice, so TOC is also updated.
	TEXINPUTS=$(STY_DIR)//: $(PDFLATEX) -output-directory $(OUT_DIR) -jobname $(basename $@) $<
	TEXINPUTS=$(STY_DIR)//: $(PDFLATEX) -output-directory $(OUT_DIR) -jobname $(basename $@) $<
	ln -f $(OUT_DIR)/$@ .


clean:
	-test -d $(OUT_DIR) && rm -fr $(OUT_DIR)
	-rm -f $(TARGET)
	-test -d $(CODE_DIR) && make -C $(CODE_DIR) clean
