.PHONY: all clean nm ss client
# make with no argument runs everything

all: nm ss client

nm:
	$(MAKE) -C nm

ss:
	$(MAKE) -C ss

client:
	$(MAKE) -C client

clean:
	$(MAKE) -C nm clean
	$(MAKE) -C ss clean
	$(MAKE) -C client clean