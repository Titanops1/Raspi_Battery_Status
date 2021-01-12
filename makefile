LIBS = `pkg-config --libs gtk+-3.0`

CFLAGS = `pkg-config --cflags gtk+-3.0`

all: battery_stat

gtk_battery: battery_stat.c
	gcc -o battery_stat battery_stat.c $(LIBS) $(CFLAGS)
