CONTIKI_PROJECT = udp-client udp-server udp-serverIOT
all: $(CONTIKI_PROJECT)

CONTIKI_TARGET_SOURCEFILES += dht22.c adc-sensors.c

CONTIKI=../..
include $(CONTIKI)/Makefile.include
