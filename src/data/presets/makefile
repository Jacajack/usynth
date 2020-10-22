PRESETS = $(sort $(wildcard *.prog))
HEADERS = $(patsubst %.prog,%.h,$(PRESETS))

generated.h: $(HEADERS)
	./combine_prog_headers.sh $(HEADERS) > $@
	
clean:
	-rm -rf $(HEADERS) generated.h
	
%.h: %.prog
	./prog2header.awk $< > $@
