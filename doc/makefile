.PHONY:	clean

FIGURES := $(shell ls *.odg)
FIGURES_SVG := $(FIGURES:%.odg=%.svg)

all: $(FIGURES_SVG) 

%.svg:	%.odg
	soffice  --invisible --convert-to svg $<


clean:
	rm *.svg 
