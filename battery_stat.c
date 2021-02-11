/*
original by https://github.com/juzam/pi-top-battery-status
*/

#define VERSION			"1.0"

#include <cairo.h>
#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <time.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_ANSWER_SIZE 64                     // Maximum size of answer string
#define MAX_COUNT       20                     // Maximum number of trials

#define WINDOW_WIDTH    260
#define WINDOW_HEIGHT   100
#define TEXT_OFFSET      20

#define GRAY_LEVEL      0.93

#define MAKELOG                0     // log file batteryLog in home directory (0 = no log file)
#define LOW_BATTERY_WARNING   15     // Warning if capacity <= this value 
#define SHUTDOWN_CAPACITY     10     // Automatic shutdown if capacity is <= this value

#define BATTERY_CAPACITY		3500 // mAh
#define BATTERY_POWER			29400 //mWh	8.4V*3500mAh


cairo_surface_t *surface;
gint width;
gint height;
GtkWidget *MainWindow;
GtkWidget *StatusLabel1, *StatusLabel2, *StatusLabel3, *StatusLabel4;
int lastCapacity;
int shutdownCounter;
long stat_good, stat_total;
int  old_capacity = -1;
static int lowBattery;

FILE *logFile;

void printLogEntry(const char *s, int i) {
	time_t rawtime;
	struct tm *timeinfo;
	char timeString[80];
	
	time(&rawtime);
	timeinfo = localtime(&rawtime);
	strftime(timeString, 80, "%D %R:%S", timeinfo);
	// printf("%s:  %s %d\n", timeString, s, i);
	if (i != -1)
		fprintf(logFile,"%s - %s %d%%\n", timeString, s, i);
	else
		fprintf(logFile,"%s - %s\n", timeString, s);
	fflush(logFile);
}

int i2cget(char *command, char *answer)
{
	FILE *fp;
	
	fp = popen(command, "r");
	if (fp == NULL) {
		printLogEntry("Failure to run i2cget", -1);
		exit(1);
	}
	
	if (fgets(answer, MAX_ANSWER_SIZE - 1, fp) != 0) {
		pclose(fp);
		if ((answer[0] != '0') || (answer[1] != 'x')) {  // if not a hex value
			usleep(2000);
			return 1;
		
		}
		else
			return 0;	   
	}
	else {
		pclose(fp);
		return 1;
	}
}

static gboolean timer_event(GtkWidget *widget)
{
	cairo_t *cr;
	GdkPixbuf *new_pixbuf;
	int i, w;
	char str[255];
	char timeStr[255];
	char shortTimeStr[32];
	float result_tmp = 0.00;
	
	char answer[MAX_ANSWER_SIZE];
	int capacity, status, capacity_tmp;
	int count, result;
	char *sstatus;
	int time;
	
	// capacity
	count = 0;
	capacity = -1; 
	capacity_tmp = -1;
	while ((capacity  <  0) && (count++ < MAX_COUNT)) {
		result = i2cget("/usr/sbin/i2cget -y 1 0x48 0x94 b 2>&1", answer);
		if (result == 0) {
			//if (count > 1) printf("count = %d, answer = %s\n", count, answer);	
			sscanf(answer, "%x", &capacity_tmp);
		}
		result = i2cget("/usr/sbin/i2cget -y 1 0x48 0x94 b 2>&1", answer);
		stat_total++;
		if (result == 0) {
			//if (count > 1) printf("count = %d, answer = %s\n", count, answer);	
			sscanf(answer, "%x", &capacity);
			if(capacity == capacity_tmp){
				old_capacity = capacity;
				result_tmp = ((capacity/255.0*3.3)*3);
				//printLogEntry("Voltage =", result_tmp);
				capacity = (result_tmp-7)/1.40*100;
				//printLogEntry("capacity =", capacity);
				if ((capacity > 100) || (capacity < 0))
					capacity = -1;              // capacity out of limits
				else
				stat_good++;
			}else {
				capacity = old_capacity;
			}
		}
	}

	// status
	sstatus = "discharging";
	
	if (capacity != lastCapacity) {	
		printLogEntry(sstatus, capacity);
		lastCapacity = capacity;
	}
	
	time = 600;
	if (time <= 90) {
		sprintf(timeStr, "Estimated life time: %d minutes\n", time);
		sprintf(shortTimeStr, "%d min", time);
	}else {
		sprintf(timeStr, "Estimated life time: %.1f hours\n", (double)time / 60.0);  
		sprintf(shortTimeStr, "%.1f hours", (float)time / 60.0);
	}
	stat_good++;

	
	cr = cairo_create (surface);
	
	// fill the battery symbol
	if (capacity < 0)         // capacity out of limits
	  w = 0;
	else
	  w = (99 * capacity) / 400;
	if (strcmp(sstatus,"charging") == 0)
		cairo_set_source_rgb (cr, 1, 1, 0);
	else if (capacity < 20)
		cairo_set_source_rgb (cr, 1, 0, 0);
	else if (strcmp(sstatus,"external power") == 0)
	    cairo_set_source_rgb (cr, 0.5, 0.5, 0.7);
	else
		cairo_set_source_rgb (cr, 0, 1, 0);
	cairo_rectangle (cr, 5, 4, w, 12);
	cairo_fill (cr);
	if (w < 23) {
		cairo_set_source_rgb (cr, 1, 1, 1);
		cairo_rectangle (cr, 5 + w, 4, 24 - w, 12);
		cairo_fill (cr);
	}
	
	// display the capacity figure
	cairo_set_source_rgb (cr, GRAY_LEVEL, GRAY_LEVEL, GRAY_LEVEL);
	cairo_rectangle (cr, 2, 20, 32, 15);
	cairo_fill (cr);   
	cairo_set_source_rgb (cr, 0, 0, 0);
	cairo_select_font_face(cr, "Dosis", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
	cairo_set_font_size(cr, 12);
	if (capacity >= 0) {
	  int x = 5;
	  if (capacity < 10)
	    x += 2;
	  else if (capacity > 99)
	    x -= 2;
	  cairo_move_to(cr, x, 32);
	  sprintf(str,"%2d %%",capacity);
	  cairo_show_text(cr, str);
	}
		
	// Create a new pixbuf from the modified surface and display icon
	new_pixbuf = gdk_pixbuf_get_from_surface(surface, 0, 0, width, height);
	gtk_window_set_icon(GTK_WINDOW(MainWindow), new_pixbuf);         
	g_object_unref(new_pixbuf);
	cairo_destroy (cr);
	
	// Update status in main Window
	sprintf(str, "<span size=\"medium\">Battery status: %s</span>", sstatus);
	gtk_label_set_markup(GTK_LABEL(StatusLabel2), str);
	if (capacity >= 0)
	  sprintf(str,"<span size=\"medium\">Capacity: %2d %%</span>", capacity);
	else
	  sprintf(str,"<span size=\"medium\">Capacity: unknown</span>", capacity);
	gtk_label_set_markup(GTK_LABEL(StatusLabel3), str);
	sprintf(str,"<span size=\"medium\">%s</span>", timeStr);
	gtk_label_set_markup(GTK_LABEL(StatusLabel4), str);
	
	// display the remaining time in the title
	gtk_window_set_title(GTK_WINDOW(MainWindow), shortTimeStr);

	if (capacity > SHUTDOWN_CAPACITY)
	  shutdownCounter = 0;
	
	if ((capacity > 0) && (capacity <= lowBattery) && (strcmp(sstatus,"charging"))) {

		if (capacity <= SHUTDOWN_CAPACITY) {
		    shutdownCounter++;
			sprintf(str,
				"Battery capacity very low! To protect battery, automatic shutdown will happen in %d sec! (To abort shutdown: type 'pkill gtk_battery' in terminal window)",
				120 - (5 * shutdownCounter));
			}
		else
			sprintf(str, 
				"Battery capacity low! Automatic shutdown will happen at %d percent",
				SHUTDOWN_CAPACITY);
		
		GtkWidget *dialog = gtk_message_dialog_new (GTK_WINDOW(MainWindow),
			GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, str);
				
		// open as non modal window
		g_signal_connect_swapped (dialog, "response",
			G_CALLBACK (gtk_widget_destroy), dialog);
		gtk_widget_show_all(dialog);
		
		if ((capacity <= SHUTDOWN_CAPACITY) && (shutdownCounter >= 20)) {
			printLogEntry("Shutdown, capacity =", capacity);
			if (MAKELOG)
				fclose(logFile);
			system("sudo shutdown -h now &");
			return FALSE;        // no further checks, system will shut down anyway
		}
		
		// reduce warning level for next warning
		if (lowBattery > SHUTDOWN_CAPACITY)
			lowBattery -= 2;
			
		// avoid multiple warnings if battery is already low
		if (lowBattery > (capacity - 1))
			lowBattery = capacity - 1;
			
		// in any case, warn if capacity is below shutdown level
		if (lowBattery < SHUTDOWN_CAPACITY)
			  lowBattery = SHUTDOWN_CAPACITY;
	}
	
	
	// initialize warning again, if battery is charging
	if (strcmp(sstatus,"charging") == 0)
			lowBattery = LOW_BATTERY_WARNING;
	
	return TRUE;
}

static void iconify(void)
{
	gtk_window_iconify(GTK_WINDOW(MainWindow));
}

int main(int argc, char *argv[])
{
	GdkPixbuf *pixbuf, *new_pixbuf;
	cairo_t *cr;
	cairo_format_t format;
	
	lowBattery = LOW_BATTERY_WARNING;
	
	lastCapacity = 0;
	shutdownCounter = 0;
	stat_good = 0;
	stat_total = 0;
	
	if (MAKELOG) {
		logFile = fopen("/home/pi/batteryLog.txt","a");
		printLogEntry(VERSION, -1);
	}
	else
		logFile = stdout;

	gtk_init(&argc, &argv);
	
	MainWindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);         // Create the main windows

	// Define main window event handlers  
	gtk_widget_add_events(MainWindow, GDK_BUTTON_PRESS_MASK);
	// g_signal_connect(MainWindow, "destroy", G_CALLBACK(gtk_main_quit), NULL); 
	g_signal_connect(MainWindow, "delete_event", G_CALLBACK(gtk_window_iconify), NULL);
	g_signal_connect(MainWindow, "button-press-event", G_CALLBACK(iconify), NULL); 
 
	// Position, size, icon and title of main window
	GdkScreen *screen = gtk_window_get_screen(GTK_WINDOW(MainWindow));
	int screenWidth = gdk_screen_get_width(screen);
	gtk_window_move(GTK_WINDOW(MainWindow), screenWidth - WINDOW_WIDTH - 2, 37);
	gtk_window_set_default_size(GTK_WINDOW(MainWindow), WINDOW_WIDTH, WINDOW_HEIGHT);
	gtk_window_set_title(GTK_WINDOW(MainWindow), "pi-top battery");
	// gtk_window_set_decorated(GTK_WINDOW(MainWindow), FALSE);
	gtk_window_set_keep_above(GTK_WINDOW(MainWindow), TRUE);
	gtk_window_iconify(GTK_WINDOW(MainWindow));
	
	// create the drawing surface and fill with icon
	char *iconPath = "/home/pi/bin/battery_icon.png";
	pixbuf = gdk_pixbuf_new_from_file (iconPath, NULL);
	if (pixbuf == NULL) {
		printf("Cannot open %s\n", iconPath);
		return(1);
	}
	format = (gdk_pixbuf_get_has_alpha (pixbuf)) ? CAIRO_FORMAT_ARGB32 : CAIRO_FORMAT_RGB24;
	width = gdk_pixbuf_get_width (pixbuf);
	height = gdk_pixbuf_get_height (pixbuf);
	surface = cairo_image_surface_create (format, width, height);
	g_assert (surface != NULL);
	
	// Draw icon onto the surface
	cr = cairo_create (surface);     
	gdk_cairo_set_source_pixbuf (cr, pixbuf, 0, 0);
	cairo_paint (cr);
	cairo_destroy (cr);
	
	// Add timer event
	// Register the timer and set time in mS.
	// The timer_event() function is called repeatedly until it returns FALSE. 
	g_timeout_add(5000, (GSourceFunc) timer_event, (gpointer) MainWindow);
	
	// Create label in main window
	GtkWidget *fixed;
	fixed = gtk_fixed_new();
	gtk_widget_set_size_request(fixed, WINDOW_WIDTH, WINDOW_HEIGHT);
	gtk_container_add(GTK_CONTAINER(MainWindow), fixed);
	gtk_widget_show(fixed);
	
	StatusLabel1 = gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL(StatusLabel1),
	  "<span size=\"x-large\">pi-top</span>");
	gtk_label_set_justify(GTK_LABEL(StatusLabel1), GTK_JUSTIFY_LEFT);
	gtk_fixed_put(GTK_FIXED(fixed), StatusLabel1, 64, 6);
	
	StatusLabel2 = gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL(StatusLabel2),
	  "<span size=\"medium\">Battery status:</span>");
	gtk_label_set_justify(GTK_LABEL(StatusLabel2), GTK_JUSTIFY_LEFT);
	gtk_fixed_put(GTK_FIXED(fixed), StatusLabel2, 8, 16 + TEXT_OFFSET);
	
	StatusLabel3 = gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL(StatusLabel3),
	  "<span size=\"medium\">Capacity:</span>");
	gtk_label_set_justify(GTK_LABEL(StatusLabel3), GTK_JUSTIFY_LEFT);
	gtk_fixed_put(GTK_FIXED(fixed), StatusLabel3, 8, 16 + 2 * TEXT_OFFSET);
	
	StatusLabel4 = gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL(StatusLabel4),
	  "<span size=\"medium\">Time:</span>");
	gtk_label_set_justify(GTK_LABEL(StatusLabel4), GTK_JUSTIFY_LEFT);
	gtk_fixed_put(GTK_FIXED(fixed), StatusLabel4, 8, 16 + 3 * TEXT_OFFSET);
	
	// display the window
	gtk_widget_show_all(MainWindow);
	// Call the timer function because we don't want to wait for the first time period triggered call
	timer_event(MainWindow);
	
	gtk_main();
	
	if (MAKELOG) {
	  printLogEntry("gtk_battery window stopped, capacity = ", lastCapacity);
	  fclose(logFile);
	}
	
	return 0;
}
