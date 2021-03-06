/*
Futura Plus
Written by Edwin Finch (http://www.github.com/edwinfinch/futuraplus)
Under MIT license

Credits:
Weather icons: http://maxcdn.webappers.com/img/2012/05/weather-icons.jpg
OpenWeatherMap API used for weather :)

Big, big thanks to Martin Norland (developer of Timely) for making his watchface open source. Without that,
I probably would not have persistent storage working yet ;)

#sixseasonsandamovie
*/

#include <pebble.h>
#include "defineElements.h"

//Define glance_this function early
void glance_this(int sentence, bool vibrate, int vibrateNum, int animationLength, bool notifyFull);
static void weatherPlease(void);

void process_tuple(Tuple *t)
{
	//Get key
	int key = t->key;

	//Get integer value
	int value = t->value->int32;

	//Get string value
	char string_value[32];
	strcpy(string_value, t->value->cstring);
  switch (key) {
	  //If the icon has been extracted
    case WEATHER_ICON_KEY:
	  //Reset icon
      if (weather_icon) {
        gbitmap_destroy(weather_icon);
      }
	  //Set icon and layer
      weather_icon = gbitmap_create_with_resource(WEATHER_ICONS[t->value->uint8]);
      bitmap_layer_set_bitmap(weather_icon_layer, weather_icon);
	  settings.previousIcon = t->value->uint8;
	  if(settings.previousIcon == 5 && nightTime == 1){
		  weather_icon = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_CLEAR_MOON);
      	  bitmap_layer_set_bitmap(weather_icon_layer, weather_icon);
		  settings.previousIcon = 10;
	  }
	  else if(settings.previousIcon == 6 && nightTime == 1){
		  weather_icon = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_CLOUDY_MOON);
      	  bitmap_layer_set_bitmap(weather_icon_layer, weather_icon);
		  settings.previousIcon = 11;
	  }
      break;
	  //If it's the temp.
    case WEATHER_TEMPERATURE_KEY:
	  //Set textlayer accordingly and write temp for previousTemp variable
	    temperaturePreConvert = value;	  
	 	if(settings.tempPref == 5){
				  //Fahrenheit
		  		  temperature = (1.8*(temperaturePreConvert-273)+32);
			  }
		else if(settings.tempPref == 4){
			//Celsius
		  	temperature = (temperaturePreConvert - 273.15);
		}
	    snprintf(temperature_buffer, sizeof("-XX"), "%d", temperature);
      	text_layer_set_text(weather_text_layer, (char*) &temperature_buffer);
	    settings.previousTemp = temperature;
		
	  goto updateWeather;
      break;
	case KEY_TEMP_PREF:
	  updateWeather:;
			  if(value == 5){
				  //Fahrenheit
		  		  temperature = (1.8*(temperaturePreConvert-273)+32);
				  settings.tempPref = 5;
			  }
			  else if(value == 4){
				  //Celsius
		  		  temperature = (temperaturePreConvert - 273.15);
				  settings.tempPref = 4;
			  }
	    snprintf(temperature_buffer, sizeof("-XX"), "%d", temperature);
      	text_layer_set_text(weather_text_layer, (char*) &temperature_buffer);
	    settings.previousTemp = temperature;
	  if(booted == 1){
		 glance_this(4, 0, 0, 5000, 0); 
	  }
		
	  break;
	case KEY_BATTERY_SAVE:
	  //Inverted
	  if(value == 5){
		  settings.batterySaveMode = 5;
	  }
	  //Normal
	  else{
		  settings.batterySaveMode = 4;
	  }
	  break;
	case KEY_WEATHER_INTERVAL:
	  if(value == 4){
		  weatherInterval = 1800000;
		  settings.weatherIntervalValue = 4;
	  }
	  else if(value == 5){
		  weatherInterval = 900000;
		  settings.weatherIntervalValue = 5;
	  }
	  else if(value == 6){
		  weatherInterval = 2700000;
		  settings.weatherIntervalValue = 6;
	  }
	  else if(value == 7){
		  weatherInterval = 3600000;
		  settings.weatherIntervalValue = 7;
	  }
	  break;
	case KEY_DATE_FORMAT:
	  if(value == 4){
		  settings.dateFormat = 4;
	  }
	  else if(value == 5){
		  settings.dateFormat = 5;
	  }
	  else if(value == 6){
		  settings.dateFormat = 6;
	  }
	  break;
	case KEY_SECONDS:
	  if(value == 5){
		  settings.secondsEnabled = 5;
	  }
	  else{
		  settings.secondsEnabled = 4;
	  }
	  break;
	case KEY_BOOTANIMATION:
	  settings.bootAnimation = value;
	  break;
	case KEY_DISWARN:
	  settings.disconnectWarn = value;
	  break;
	case KEY_REWARN:
	  settings.reconnectWarn = value;
	  glance_this(1, 1, 2, 3000, 0);
	  weatherPlease();
	  break;
  }
}

//When we get a message from the phone
static void in_received_handler(DictionaryIterator *iter, void *context) 
{
	(void) context;

	//Get data
	Tuple *t = dict_read_first(iter);
	if(t)
	{
		process_tuple(t);
	}
	//Get next
	while(t != NULL)
	{
		t = dict_read_next(iter);
		if(t)
		{
			//Process that data
			process_tuple(t);
		}
	}
}

//Send random data to JavaScript
static void weatherPlease(void) {
  Tuplet value = TupletInteger(1, 1);

  DictionaryIterator *iter;
  app_message_outbox_begin(&iter);

  if (iter == NULL) {
    return;
  }

  dict_write_tuplet(iter, &value);
  dict_write_end(iter);

  app_message_outbox_send();
}

//When the timer has been fired
void timerCallback(void *data) {
	//Request for weather
	weatherPlease();
	//Start timer again
	timer = app_timer_register(weatherInterval, (AppTimerCallback) timerCallback, NULL);
}

//Timer for glance_this, incase there is already something being animated
void glanceTimerCallback(void *data){
	if(currentlyGlancing == 1){
		glanceTimer = app_timer_register(500, (AppTimerCallback) glanceTimerCallback, NULL);
	}
	else{
		glance_this(holdUpSentence, holdUpVibrate, holdUpVibrateNum, holdUpAnimationLength, holdUpFullNotify);
	}
	glanceTimerCalled++;
	if(glanceTimerCalled > 2){
		currentlyGlancing = 0;
		glanceTimerCalled = 0;
	}
}

//Define textlayerinit function, which handles init of text layers
//Saves lots of code and time
static TextLayer* textLayerInit(GRect location, GColor colour, GColor background, GTextAlignment alignment)
{
	TextLayer *layer = text_layer_create(location);
	text_layer_set_text_color(layer, colour);
	text_layer_set_background_color(layer, background);
	text_layer_set_text_alignment(layer, alignment);

	return layer;
}

//When an animation has stopped
void on_animation_stopped(Animation *anim, bool finished, void *context)
{
    //Free the memoery used by the Animation
    property_animation_destroy((PropertyAnimation*) anim);
	animationNumber++;
	
	//If the watchface time has hit it's location, set it as booted time to 
	//prevent minute change animation from firing wrongly
	if(animationNumber > 1 && bootedTime == 0){
		bootedTime = 1;
	}
	//When all watchface elements have booted, set variables as true
	if(animationNumber > 7 && booted == 0){
		booted = 1;
		animationNumber = 0;
	}
	
	if(animationNumber > 1 && booted == 1){
		currentlyGlancing = 0;
		animationNumber = 0;
	}
}
 
void animate_layer(Layer *layer, GRect *start, GRect *finish, int duration, int delay)
{
	if(settings.batterySaveMode == 4 || booted == false){
    	//Declare animation
    	PropertyAnimation *anim = property_animation_create_layer_frame(layer, start, finish);
     
    	//Set characteristics
    	animation_set_duration((Animation*) anim, duration);
    	animation_set_delay((Animation*) anim, delay);
     
    	//Set stopped handler to free memory
		AnimationHandlers handlers = {
        	//The reference to the stopped handler is the only one in the array
        .stopped = (AnimationStoppedHandler) on_animation_stopped
        };
    	animation_set_handlers((Animation*) anim, handlers, NULL);
     
    	//Start animation
    	animation_schedule((Animation*) anim);
	}
}

void tick_handler(struct tm *tick_time, TimeUnits units_changed)
{
    //Format the Time buffer string using tick_time as the time source
	if(settings.secondsEnabled == 5){
		text_layer_set_font(time_layer, futuraBold24);
		if(hourmode == 1){
			strftime(timeBufferSeconds, sizeof(timeBufferSeconds), "%H:%M:%S", tick_time);
		}
		else{
			strftime(timeBufferSeconds, sizeof(timeBufferSeconds), "%I:%M:%S", tick_time);
		}
	}
	else if(settings.secondsEnabled == 4){
		text_layer_set_font(time_layer, futuraBold);
		strftime(timeBuffer, sizeof(timeBuffer), "%H:%M", tick_time);
	}
	if(settings.dateFormat == 4){
		strftime(dateBuffer, sizeof(dateBuffer), "%B %e", tick_time);
	}
	else if(settings.dateFormat == 5){
		strftime(dateBuffer, sizeof(dateBuffer), "%d-%m-%Y", tick_time);
	}
	else if(settings.dateFormat == 6){
		strftime(dateBuffer, sizeof(dateBuffer), "%m-%d-%Y", tick_time);
	}
	
	//12 and 24 hour support
	if(clock_is_24h_style()){
      strftime(timeBuffer, sizeof(timeBuffer), "%H:%M", tick_time);
	  hourmode = 1;
    }
    else{
      strftime(timeBuffer,sizeof(timeBuffer),"%I:%M",tick_time);
		hourmode = 0;
    }
	//Take seconds from tick_time();
	seconds = tick_time->tm_sec;
	minutes = tick_time->tm_min;
	hours = tick_time->tm_hour;
	
	//if the seconds is close to time-change, but not hour change
	if(seconds == 59 && minutes < 59){
		if(settings.secondsEnabled == 4){
			//Animate layer
	    	GRect start = GRect(180, 10, 144, 35);
	    	GRect finish = GRect(80, 10, 144, 35);
	    	animate_layer(text_layer_get_layer(cover_layer), &start, &finish, 1000, 10);
			GRect start1 = GRect(80, 10, 144, 35);
	    	GRect finish1 = GRect(180, 10, 144, 35);
	    	animate_layer(text_layer_get_layer(cover_layer), &start1, &finish1, 1000, 1010);
		}
		else{
			//Animate layer
	    	GRect start = GRect(180, 10, 144, 35);
	    	GRect finish = GRect(48, 10, 144, 35);
	    	animate_layer(text_layer_get_layer(cover_layer), &start, &finish, 1000, 10);
			GRect start1 = GRect(48, 10, 144, 35);
	    	GRect finish1 = GRect(180, 10, 144, 35);
	    	animate_layer(text_layer_get_layer(cover_layer), &start1, &finish1, 1000, 1010);
		}
	}
	//different hour change animation
	else if(seconds == 58 && minutes == 59){
		GRect start = GRect(180, 10, 144, 35);
	    GRect finish = GRect(0, 10, 144, 35);
	    animate_layer(text_layer_get_layer(cover_layer), &start, &finish, 2000, 10);
		GRect start1 = GRect(0, 10, 144, 35);
	    GRect finish1 = GRect(180, 10, 144, 35);
	    animate_layer(text_layer_get_layer(cover_layer), &start1, &finish1, 1000, 2010);
	}
	
    //Change the TextLayer(s) text to show the new time and date
	if(settings.secondsEnabled == 5){
		text_layer_set_text(time_layer, timeBufferSeconds);
	}
	else{
		text_layer_set_font(time_layer, futuraBold);
		text_layer_set_text(time_layer, timeBuffer);
	}
	text_layer_set_text(date_layer, dateBuffer);
	if(hours > 18 || hours < 7){
		nightTime = 1;
	}
}

void handle_bt(bool connected){
	//If the watch has disconnected
	if(connected == 0){
		//Update icon and glance_this alert
		bluetooth_connected_icon = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_DISCONNECTED_ICON);
		if(settings.disconnectWarn == 5 && booted == 1){
			glance_this(2, 1, 3, 4000, 0);
		}
  }
  	if(connected == 1){
		//If it's been reconnected, update icon and glance_this alert
  	    bluetooth_connected_icon = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_CONNECTED_ICON);
		if(settings.reconnectWarn == 5 && booted == 1){
			glance_this(3, 1, 2, 3000, 0);
		}
		weatherPlease();
  }
	//Set icon
	bitmap_layer_set_bitmap(bluetooth_connected_layer, bluetooth_connected_icon);
}

//Battery section
void handle_battery(BatteryChargeState charge_state) {
  static char battery_text[] = "100%";
//If the watchface is charging
  if (charge_state.is_charging) {
	  //Set icon to charge icon
      battery_icon = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BATTERY_CHARGE);
	  snprintf(battery_text, sizeof(battery_text), "%d%%", charge_state.charge_percent);
	  bitmap_layer_set_bitmap(battery_icon_layer, battery_icon);
    } else {
	  //Otherwise set icon to standard battery image
	    battery_icon = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BATTERY);
        snprintf(battery_text, sizeof(battery_text), "%d%%", charge_state.charge_percent);
	    bitmap_layer_set_bitmap(battery_icon_layer, battery_icon);
      }
	//Update battery text
  text_layer_set_text(battery_text_layer, battery_text);
}

//Section for initializing all layers and resources
void window_load(Window *window){
	Layer *window_layer = window_get_root_layer(window);
	futuraBold = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_FUTURA_BOLD_35));
	futuraBold24 = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_FUTURA_BOLD_24));
	futuraLight = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_FUTURA_LIGHT_29));
	futuraExtended = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_FUTURA_EXTENDED_21));
	
	time_layer = textLayerInit(GRect(0, -300, 140, 168), GColorBlack, GColorClear, GTextAlignmentCenter);
	text_layer_set_font(time_layer, futuraBold);
  	layer_add_child(window_layer, (Layer*) time_layer);
	
	date_layer = textLayerInit(GRect(0, -300, 140, 168), GColorBlack, GColorClear, GTextAlignmentCenter);
	text_layer_set_font(date_layer, futuraLight);
	layer_add_child(window_layer, (Layer*) date_layer);
	
	cover_layer = textLayerInit(GRect(180,0,144,84), GColorBlack, GColorWhite, GTextAlignmentCenter);
	layer_add_child(window_layer, (Layer*) cover_layer);
	
	invert_layer_half = inverter_layer_create(GRect(0,0,144,84));
	layer_add_child(window_layer, (Layer*) invert_layer_half);
	
	battery_text_layer = textLayerInit(GRect(0, -300, 140, 168), GColorBlack, GColorClear, GTextAlignmentCenter);
	text_layer_set_font(battery_text_layer, futuraExtended);
	layer_add_child(window_layer, (Layer *) battery_text_layer);
	text_layer_set_text(battery_text_layer, "0");
	
	weather_text_layer = textLayerInit(GRect (0, -300, 140, 168), GColorBlack, GColorClear, GTextAlignmentCenter);
	text_layer_set_font(weather_text_layer, futuraExtended);
	layer_add_child(window_layer, (Layer *) weather_text_layer);
	snprintf(temperature_buffer, sizeof("-XX"), "%d", settings.previousTemp);
	text_layer_set_text(weather_text_layer, temperature_buffer);
	
	bluetooth_icon = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BLUETOOTH_ICON);
	bluetooth_icon_layer = bitmap_layer_create(GRect(0, -300, 140, 168));
	bitmap_layer_set_background_color(bluetooth_icon_layer, GColorClear);
	bitmap_layer_set_bitmap(bluetooth_icon_layer, bluetooth_icon);
	layer_add_child(window_layer, (Layer *) bluetooth_icon_layer);
	
	battery_icon = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BATTERY);
	battery_icon_layer = bitmap_layer_create(GRect(0, -300, 140, 168));
	bitmap_layer_set_background_color(battery_icon_layer, GColorClear);
	bitmap_layer_set_bitmap(battery_icon_layer, battery_icon);
	layer_add_child(window_layer, bitmap_layer_get_layer(battery_icon_layer));
	
	if(settings.previousIcon == 10){
		if(hours > 18 || hours < 7){
			weather_icon = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_CLEAR_MOON);
		}
		else{
			weather_icon = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_SUN);
		}
	}
	else if(settings.previousIcon == 11){
		if(hours > 18 || hours < 7){
			weather_icon = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_CLOUDY_MOON);
		}
		else{
			weather_icon = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_TEST_WEATHER_ICON);
		}
	}
	else{
		weather_icon = gbitmap_create_with_resource(WEATHER_ICONS[settings.previousIcon]);
	}
	weather_icon_layer = bitmap_layer_create(GRect(0, -300, 140, 168));
	bitmap_layer_set_background_color(weather_icon_layer, GColorClear);
	bitmap_layer_set_bitmap(weather_icon_layer, weather_icon);
	layer_add_child(window_layer, bitmap_layer_get_layer(weather_icon_layer));
	
	bluetooth_connected_icon = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_DISCONNECTED_ICON);
	bluetooth_connected_layer = bitmap_layer_create(GRect(0, -300, 140, 168));
	bitmap_layer_set_background_color(bluetooth_connected_layer, GColorClear);
	layer_add_child(window_layer, (Layer *) bluetooth_connected_layer);
	
	update_at_a_glance = textLayerInit(GRect(0, 300, 144, 168), GColorBlack, GColorWhite, GTextAlignmentCenter);
	text_layer_set_font(update_at_a_glance, fonts_get_system_font(FONT_KEY_GOTHIC_18));
    layer_add_child(window_layer, (Layer*) update_at_a_glance);
	
	//Set timer for half an hour for weather to update
	  if(settings.weatherIntervalValue == 4){
		  weatherInterval = 1800000;
	  }
	  else if(settings.weatherIntervalValue == 5){
		  weatherInterval = 900000;
	  }
	  else if(settings.weatherIntervalValue == 6){
		  weatherInterval = 2700000;
	  }
	  else if(settings.weatherIntervalValue == 7){
		  weatherInterval = 3600000;
	  }
	else{
		weatherInterval = 1800000;
	}
	
	timer = app_timer_register(weatherInterval, (AppTimerCallback) timerCallback, NULL);
	
	struct tm *t;
  	time_t temp;        
  	temp = time(NULL);        
  	t = localtime(&temp);
	
	BatteryChargeState btchg = battery_state_service_peek();
    handle_battery(btchg);
	
	bool connected = bluetooth_connection_service_peek();
  	handle_bt(connected);
	
	tick_handler(t, MINUTE_UNIT);
}
//Unload resources to save memory
void window_unload(Window *window){
	app_timer_cancel(timer);
	text_layer_destroy(time_layer);
	text_layer_destroy(date_layer);
	text_layer_destroy(battery_text_layer);
	text_layer_destroy(weather_text_layer);
	text_layer_destroy(cover_layer);
	text_layer_destroy(update_at_a_glance);
	inverter_layer_destroy(invert_layer_half);
	bitmap_layer_destroy(battery_icon_layer);
	bitmap_layer_destroy(weather_icon_layer);
	bitmap_layer_destroy(bluetooth_icon_layer);
	bitmap_layer_destroy(bluetooth_connected_layer);
	gbitmap_destroy(battery_icon);
	gbitmap_destroy(weather_icon);
	gbitmap_destroy(bluetooth_icon);
	gbitmap_destroy(bluetooth_connected_icon);
}
//Initialize watchface
void handle_init(void) {
	window = window_create();
	//Set handlers
	window_set_window_handlers(window, (WindowHandlers) {
  		.load = window_load,
  		.unload = window_unload,
    });
	//Open app message port and it's function "in_received_handler"		 
	app_message_register_inbox_received(in_received_handler);					 
	app_message_open(512, 512);		//Large input and output buffer sizes
	weatherPlease();
	
	//Set tick_handler for seconds for more accurate animations
	tick_timer_service_subscribe(SECOND_UNIT, (TickHandler) tick_handler);
	
	valueRead = persist_read_data(SETTINGS_KEY, &settings, sizeof(settings));
	APP_LOG(APP_LOG_LEVEL_WARNING, "Read %d bytes from settings", valueRead);
	
	//Push window
	window_stack_push(window, true);
	
	//Subscribe to check bluetooth connection
	bluetooth_connection_service_subscribe(&handle_bt);
	
	//Subscribe to watch battery status
	battery_state_service_subscribe(&handle_battery);
	
	//Animate all layers
	if(settings.bootAnimation == 4){
		//Bottom
		start = GRect(0, 400, 144, 168);
		goto animate;
	}
	else if(settings.bootAnimation == 5){
		//Top
		start = GRect(0, -400, 144, 168);
		goto animate;
	}
	else if(settings.bootAnimation == 6){
		//Left
		start = GRect(-300, 50, 144, 168);
		goto animate;
	}
	else if(settings.bootAnimation == 7){
		//Right
		start = GRect(400, 50, 144, 168);
		goto animate;
	}
	else if(settings.bootAnimation == 8){
		//Top left
		start = GRect(-300, -400, 144, 168);
		goto animate;
	}
	else if(settings.bootAnimation == 9){
		//Top right
		start = GRect(300, -400, 144, 168);
		goto animate;
	}
	else if(settings.bootAnimation == 10){
		//Bottom left
		start = GRect(-300, 400, 144, 168);
		goto animate;
	}
	else if(settings.bootAnimation == 11){
		//Bottom right
		start = GRect(400, 400, 144, 168);
		goto animate;
	}
	else if(settings.bootAnimation == 12){
		//Garbled
		start = GRect(72, 84, 144, 168);
		goto animate;
	}
	animate:;
	finish = GRect(0, 10, 144, 168);
	finish1 = GRect(-45, 126, 144, 168);
	finish2 = GRect(0, 45, 140, 168);
	finish3 = GRect(45,126,144,168);
	finish4 = GRect(45, 25, 144, 168);
	finish5 = GRect(-45, 25, 144, 168);
	finish6 = GRect(0, 25, 144, 168);
	finish7 = GRect(3,55,144,168);
	animate_layer(text_layer_get_layer(time_layer), &start, &finish, 1200, 210);
	animate_layer(text_layer_get_layer(date_layer), &start, &finish2, 1200, 400);
	animate_layer(bitmap_layer_get_layer(weather_icon_layer), &start, &finish4, 1200, 600);
	animate_layer(bitmap_layer_get_layer(bluetooth_icon_layer), &start, &finish6, 1200, 800);
	animate_layer(bitmap_layer_get_layer(battery_icon_layer), &start, &finish5, 1200, 1000);
	animate_layer(text_layer_get_layer(weather_text_layer), &start, &finish3, 1200, 1200);
	animate_layer(bitmap_layer_get_layer(bluetooth_connected_layer), &start, &finish7, 1200, 1400);
	animate_layer(text_layer_get_layer(battery_text_layer), &start, &finish1, 1200, 1600);
}

//Deinit animations, subscribed elements ie. bluetooth, and persist settings
void handle_deinit(void) {
	valueWritten = persist_write_data(SETTINGS_KEY, &settings, sizeof(settings));
	APP_LOG(APP_LOG_LEVEL_WARNING, "Wrote %d bytes to settings struct", valueWritten);
	animation_unschedule_all();
	bluetooth_connection_service_unsubscribe();
	tick_timer_service_unsubscribe();
	battery_state_service_unsubscribe();
	window_destroy(window);
}

int main(void) {
	handle_init();
	app_event_loop();
	handle_deinit();
}

//Function taken from my other watchface, Clean and Simple
//Takes simple info and provides wearer with some text and optional vibration
void glance_this(int sentence, bool vibrate, int vibrateNum, int animationLength, bool fullNotify){
	//Update the text layer to the char provided by function call
	if(currentlyGlancing == 1){
		holdUpSentence = sentence;
		holdUpVibrate = vibrate;
		holdUpVibrateNum = vibrateNum;
		holdUpAnimationLength = animationLength;
		holdUpFullNotify = fullNotify;
		glanceTimer = app_timer_register(5000, (AppTimerCallback) glanceTimerCallback, NULL);
	}
	
	else if(currentlyGlancing == 0){
		if(booted == true){
			//if there's a vibration request,
			if(vibrate == true){
				if(settings.batterySaveMode == 4){
					//check what number it is and fufill accordingly.
					if(vibrateNum == 1){
						vibes_short_pulse();
		            }
					else if(vibrateNum == 2){
						vibes_double_pulse();
		            }
					else if(vibrateNum == 3){
						vibes_long_pulse();
		            }
			    }
				else{
				//Incase of future need
			    }
		    }
			//Update text and animate update_at_a_glance layer for fancy effect
			if(sentence == 1){
				text_layer_set_text(update_at_a_glance, "Settings updated.");
			}
			else if(sentence == 2){
				text_layer_set_text(update_at_a_glance, "Bluetooth disconnected.");
			}
			else if(sentence == 3){
				text_layer_set_text(update_at_a_glance, "Bluetooth reconnected.");
			}
			else if(sentence == 4){
				text_layer_set_text(update_at_a_glance, "Weather updated.");
			}
			currentlyGlancing = 1;
				if(fullNotify == 1){
					text_layer_set_background_color(update_at_a_glance, GColorWhite);
					GRect start01 = GRect(0, 300, 144, 168);
					GRect finish01 = GRect(0, 50, 144, 168);
					GRect start02 = GRect(0, 50, 144, 168);
					GRect finish02 = GRect(0, 300, 144, 168);
					animate_layer(text_layer_get_layer(update_at_a_glance), &start01, &finish01, 1000, 0);
					animate_layer(text_layer_get_layer(update_at_a_glance), &start02, &finish02, 1000, animationLength);
		        }
				else{
					text_layer_set_background_color(update_at_a_glance, GColorClear);
					GRect start01 = GRect(0, 300, 144, 168);
					GRect finish01 = GRect(0, 145, 144, 168);
					GRect start02 = GRect(0, 145, 144, 168);
					GRect finish02 = GRect(0, 300, 144, 168);
					animate_layer(text_layer_get_layer(update_at_a_glance), &start01, &finish01, 1000, 0);
					animate_layer(text_layer_get_layer(update_at_a_glance), &start02, &finish02, 1000, animationLength);
		        }
		
	    }
		else if(booted != true){
			holdUpSentence = sentence;
			holdUpVibrate = vibrate;
			holdUpVibrateNum = vibrateNum;
			holdUpAnimationLength = animationLength;
			holdUpFullNotify = fullNotify;
			glanceTimer = app_timer_register(5000, (AppTimerCallback) glanceTimerCallback, NULL);
		}
	}
}
