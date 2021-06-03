
.PHONY : construct msg all clean

construct: msg all

msg:
	@echo ----- constructing _frozen_mpy.c

all:
	@./gen.sh

clean:
	@./gen.sh clean
