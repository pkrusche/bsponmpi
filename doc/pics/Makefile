PICTURES=\
  exptable.eps    exptable.png\
  exptable-ex.eps exptable-ex.png\
  varszel-ex.eps  varszel-ex.png

.SUFFIXES: .fig .png .eps

all: $(PICTURES)


.fig.png:
	fig2dev -Lpng -m 5 $< temp.png
	convert -antialias -resize 20% temp.png $@
	rm temp.png

.fig.eps:
	fig2dev -Leps $< $@

.PHONY: .clean
clean: 
	rm *.eps *.png
