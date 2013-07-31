CC=gcc-4.4
CFLAGS = -DARCH_64 -ggdb `xml2-config --cflags` `agar-config --cflags`  -I/usr/include -DLINUX -DOPJ_HAVE_INTTYPES_H -DUSE_JPIP

LOCALPATH=/usr/local
LFLAGS = -lopenjpeg -lglut  -lSDL -lSDL_net -lSDL_Clipboard -lm -lGL -lGLU `xml2-config --libs` `agar-config --libs` -L${LOCALPATH}/lib -L${LOCALPATH}/lib64

all: bio.o cidx_manager.o cio.o color.o convert.o dwt.o event.o function_list.o image.o index.o invert.o j2k.o jp2.o mct.o mqc.o openjpeg.o opj_clock.o opj_decompress.o opj_getopt.o pi.o raw.o t1.o t2.o tcd.o tgt.o

bio.o: bio.c
	$(CC) $(CFLAGS) -Wall -c $< -o $@

cidx_manager.o: cidx_manager.c
	$(CC) $(CFLAGS) -Wall -c $< -o $@

cio.o: cio.c
	$(CC) $(CFLAGS) -Wall -c $< -o $@

color.o: color.c
	$(CC) $(CFLAGS) -Wall -c $< -o $@

convert.o: convert.c
	$(CC) $(CFLAGS) -Wall -c $< -o $@

dwt.o: dwt.c
	$(CC) $(CFLAGS) -Wall -c $< -o $@

event.o: event.c
	$(CC) $(CFLAGS) -Wall -c $< -o $@

function_list.o: function_list.c
	$(CC) $(CFLAGS) -Wall -c $< -o $@

image.o: image.c
	$(CC) $(CFLAGS) -Wall -c $< -o $@

index.o: index.c
	$(CC) $(CFLAGS) -Wall -c $< -o $@

invert.o: invert.c
	$(CC) $(CFLAGS) -Wall -c $< -o $@

j2k.o: j2k.c
	$(CC) $(CFLAGS) -Wall -c $< -o $@

jp2.o: jp2.c
	$(CC) $(CFLAGS) -Wall -c $< -o $@

mct.o: mct.c
	$(CC) $(CFLAGS) -Wall -c $< -o $@

mqc.o: mqc.c
	$(CC) $(CFLAGS) -Wall -c $< -o $@

openjpeg.o: openjpeg.c
	$(CC) $(CFLAGS) -Wall -c $< -o $@

opj_clock.o: opj_clock.c
	$(CC) $(CFLAGS) -Wall -c $< -o $@

opj_decompress.o: opj_decompress.c
	$(CC) $(CFLAGS) -Wall -c $< -o $@

opj_getopt.o: opj_getopt.c
	$(CC) $(CFLAGS) -Wall -c $< -o $@

pi.o: pi.c
	$(CC) $(CFLAGS) -Wall -c $< -o $@

raw.o: raw.c
	$(CC) $(CFLAGS) -Wall -c $< -o $@

t1.o: t1.c
	$(CC) $(CFLAGS) -Wall -c $< -o $@

t2.o: t2.c
	$(CC) $(CFLAGS) -Wall -c $< -o $@

tcd.o: tcd.c
	$(CC) $(CFLAGS) -Wall -c $< -o $@

tgt.o: tgt.c
	$(CC) $(CFLAGS) -Wall -c $< -o $@

