CC= gcc
OPT= -O -Wall #-g
DEFS=
LDFLAGS=
LIBS=

CFLAGS = $(OPT) -Wall $(DEFS) 

INCDIR=

PROGS=morsesend morsercv

all: $(PROGS)

clean:
	rm -f $(PROGS) *.o *~ core

morsesend: morsesend.c
	$(CC) $(CFLAGS) $(INCDIR) -o morsesend morsesend.c $(LDFLAGS) $(LIBS)

morsercv: morsercv.c
	$(CC) $(CFLAGS) $(INCDIR) -o morsercv morsercv.c $(LDFLAGS) $(LIBS)
