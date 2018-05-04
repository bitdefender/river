TEX=pdflatex
BIBTEX=bibtex

ALL=main
SOURCES := $(wildcard *.bib *.sty *.cls *.tex media/* media/*/*.pdf)

# Sources are copied to the .build folder to manage LaTeX's flurry of
# intermediate files.
PROCESSED_SOURCES := $(patsubst %,.build/%,${SOURCES})
PROCESSED_SOURCES := $(sort ${PROCESSED_SOURCES})

all: ${ALL}.pdf

${ALL}.pdf: ${PROCESSED_SOURCES}
	rm -f "$@"
	cd .build && \
	  ${TEX} ${ALL} && \
	  ${BIBTEX} ${ALL} && \
	  ${TEX} ${ALL} && \
	  ${TEX} ${ALL}
	cp -a .build/${ALL}.pdf "$@"

.build/%: %
	mkdir -p $$(dirname "$@") && cp -a "$^" "$@"

.PHONY: clean
clean:
	rm -rf .build/ ${ALL}.pdf
