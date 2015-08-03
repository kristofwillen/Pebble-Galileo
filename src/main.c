#include <pebble.h>
#define KEY_HOUR_DIAL 0
#define KEY_PTOLMAIC 1
  
  
Window *my_window;
static GBitmap *s_min_bitmap, *s_hour_bitmap, *s_earth_bitmap, *s_stars_bitmap, *s_ufo_bitmap;
static BitmapLayer *s_min_layer, *s_hour_layer, *s_earth_layer, *s_stars_layer, *s_ufo_layer;
static int xmin, ymin, xhour, yhour, oldxmin, oldymin, oldxhour, oldyhour = 0;
static bool ScienceMovesForwardFlag = false;
static bool SunHourFlag = false;
  

void animation_ufo_stopped(Animation *animation, bool finished, void *data) {
   
  layer_set_hidden((Layer *)s_ufo_layer, true);
  
  #ifdef PBL_COLOR
    // Do nothing, Basalt does this automagically
  #else  
    property_animation_destroy((PropertyAnimation*) animation);
  #endif
}

void animation_sunmoon_stopped(Animation *animation, bool finished, void *data) {
   
  #ifdef PBL_COLOR
    // Do nothing, Basalt does this automagically
  #else  
    property_animation_destroy((PropertyAnimation*) animation);
  #endif
}



static void update_time() {
  // Get a tm structure
  time_t temp = time(NULL); 
  struct tm *t = localtime(&temp);
  static PropertyAnimation *s_min_animation, *s_hour_animation, *s_ufo_animation;
  static GRect min_from_frame, min_to_frame, hour_to_frame, hour_from_frame, ufo_to_frame, ufo_from_frame;
  static int x1ufo, x2ufo, y1ufo, y2ufo;
  
  int min_distance   = 64;
  int hour_distance  = 40;
  int32_t min_angle  = TRIG_MAX_ANGLE * t->tm_min/60;
  int32_t hour_angle = (TRIG_MAX_ANGLE * (((t->tm_hour % 12) * 6) + (t->tm_min / 10))) / (12 * 6);
  
  xmin = (int16_t)(sin_lookup(min_angle)    * min_distance  / TRIG_MAX_RATIO) + 72 - 16;
  ymin = (int16_t)(-cos_lookup(min_angle)   * min_distance  / TRIG_MAX_RATIO) + 86 - 16;
  xhour = (int16_t)(sin_lookup(hour_angle)  * hour_distance / TRIG_MAX_RATIO) + 72 - 14;
  yhour = (int16_t)(-cos_lookup(hour_angle) * hour_distance / TRIG_MAX_RATIO) + 86 - 14;
  
  min_from_frame  = GRect(oldxmin, oldymin, 32, 32);
  min_to_frame    = GRect(xmin, ymin, 32, 32);
  
  hour_from_frame = GRect(oldxhour, oldyhour, 28,28);
  hour_to_frame   = GRect(xhour, yhour, 28,28);
  
  s_min_animation = property_animation_create_layer_frame((Layer *)s_min_layer, &min_from_frame, &min_to_frame);
  animation_set_duration((Animation *)s_min_animation,1000);
  
  
  s_hour_animation = property_animation_create_layer_frame((Layer *)s_hour_layer, &hour_from_frame, &hour_to_frame);
  //animation_set_duration((Animation *)s_moon_animation,100);

  animation_set_handlers((Animation*) s_min_animation, (AnimationHandlers) {
    .stopped = (AnimationStoppedHandler) animation_sunmoon_stopped,
  }, NULL);
  
  animation_set_handlers((Animation*) s_hour_animation, (AnimationHandlers) {
    .stopped = (AnimationStoppedHandler) animation_sunmoon_stopped,
  }, NULL);
  
  animation_schedule((Animation*) s_min_animation);
  animation_schedule((Animation*) s_hour_animation);
  
  oldxmin  = xmin;
  oldymin  = ymin;
  oldxhour = xhour;
  oldyhour = yhour;
  
  if ((t->tm_min % 5) == 0) {
    x1ufo = -29;
    x2ufo = 144;
    y1ufo = rand() % 168;
    y2ufo = rand() % 168;
    ufo_from_frame  = GRect(x1ufo,y1ufo,29,16);
    ufo_to_frame    = GRect(x2ufo,y2ufo,29,16);
    layer_set_hidden((Layer *)s_ufo_layer, false);
    s_ufo_animation = property_animation_create_layer_frame((Layer *)s_ufo_layer, &ufo_from_frame, &ufo_to_frame);
    
    animation_set_handlers((Animation*) s_ufo_animation, (AnimationHandlers) {
      .stopped = (AnimationStoppedHandler) animation_ufo_stopped,
    }, NULL);
  
    animation_set_duration((Animation *)s_ufo_animation,8000);
    animation_schedule((Animation*) s_ufo_animation);
    
  }
}


static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();
  
}

static void in_recv_handler(DictionaryIterator *iterator, void *context) {
  //Get data
  Tuple *t = dict_read_first(iterator);
  
  while (t) {
   APP_LOG(APP_LOG_LEVEL_DEBUG, "[DBUG] Found dict key=%i", (int)t->key);
   switch(t->key) {

    case KEY_HOUR_DIAL:
      if (strcmp(t->value->cstring, "0") == 0) { SunHourFlag = false; }  else { SunHourFlag = true; }
      break;
     
    case KEY_PTOLMAIC:
      if (strcmp(t->value->cstring, "0") == 0) { ScienceMovesForwardFlag = false; }  else { ScienceMovesForwardFlag = true; }
      break;
   }
   t = dict_read_next(iterator);
  }
}



static void inbox_dropped(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "[DBUG] inbox_dropped %d", reason);
}


void handle_init(void) {
    
  app_message_register_inbox_received(in_recv_handler);
  app_message_register_inbox_dropped(inbox_dropped);
  app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
  
  if (persist_exists(KEY_HOUR_DIAL)) { SunHourFlag = persist_read_bool(KEY_HOUR_DIAL); }
  if (persist_exists(KEY_PTOLMAIC))  { ScienceMovesForwardFlag = persist_read_bool(KEY_PTOLMAIC); }
  
  my_window = window_create();
 
  #ifdef PBL_COLOR
    s_earth_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_EARTH_COLOR);
    if (ScienceMovesForwardFlag) {
      s_min_bitmap   = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_MOON_COLOR);
      s_hour_bitmap  = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_ISS);
    }
   else {
    if (SunHourFlag) {
      s_min_bitmap   = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_MOON_COLOR);
      s_hour_bitmap  = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_SUN_COLOR);
    }
    else {
      s_min_bitmap   = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_SUN_COLOR);
      s_hour_bitmap  = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_MOON_COLOR);
    }
   }
    s_stars_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_STARS_COLOR);
    s_ufo_bitmap   = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_UFO_COLOR);
  #else  
    s_earth_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_EARTH_ICON);
    if (ScienceMovesForwardFlag) {
      APP_LOG(APP_LOG_LEVEL_DEBUG, "[DBUG] SciencemovesForward !");
      s_min_bitmap   = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_MOON_SMALL_ICON);
      s_hour_bitmap  = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_ISS); 
    }
   else {
    if (SunHourFlag) {
      APP_LOG(APP_LOG_LEVEL_DEBUG, "[DBUG] SunHourFlag = true %d", SunHourFlag);
      s_min_bitmap   = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_MOON_SMALL_ICON);
      s_hour_bitmap  = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_SUN_SMALL_ICON);
    }
    else {
      APP_LOG(APP_LOG_LEVEL_DEBUG, "[DBUG] SunHourFlag= false %d", SunHourFlag);
      s_min_bitmap   = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_SUN_SMALL_ICON);
      s_hour_bitmap  = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_MOON_SMALL_ICON);
    }
   }  
    s_stars_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_STARS_BACKGROUND);
    s_ufo_bitmap   = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_UFO_ICON);     
  #endif
    
    
  s_stars_layer = bitmap_layer_create(GRect(0,0,144,168));
  bitmap_layer_set_bitmap(s_stars_layer, s_stars_bitmap);
  
  s_earth_layer = bitmap_layer_create(GRect(52, 68, 40, 32));
  bitmap_layer_set_bitmap(s_earth_layer, s_earth_bitmap);
  
  s_min_layer = bitmap_layer_create(GRect(58,72,32,32));
  bitmap_layer_set_bitmap(s_min_layer, s_min_bitmap);
  
  s_hour_layer = bitmap_layer_create(GRect(58,72,28,28));
  bitmap_layer_set_bitmap(s_hour_layer, s_hour_bitmap);

  s_ufo_layer = bitmap_layer_create(GRect(52, 68, 29, 16));
  bitmap_layer_set_bitmap(s_ufo_layer, s_ufo_bitmap);
  layer_set_hidden((Layer *)s_ufo_layer, true);
  
  #ifdef PBL_COLOR
    bitmap_layer_set_compositing_mode(s_hour_layer,GCompOpSet);
    bitmap_layer_set_compositing_mode(s_min_layer,GCompOpSet);
    bitmap_layer_set_compositing_mode(s_ufo_layer,GCompOpSet);
  #else
    bitmap_layer_set_compositing_mode(s_hour_layer,GCompOpOr);
    bitmap_layer_set_compositing_mode(s_min_layer,GCompOpOr);
    bitmap_layer_set_compositing_mode(s_ufo_layer,GCompOpOr);
    bitmap_layer_set_compositing_mode(s_stars_layer,GCompOpAnd);
  #endif  
  
  layer_add_child(window_get_root_layer(my_window), bitmap_layer_get_layer(s_stars_layer));  
  layer_add_child(window_get_root_layer(my_window), bitmap_layer_get_layer(s_earth_layer));
  layer_add_child(window_get_root_layer(my_window), bitmap_layer_get_layer(s_min_layer));
  layer_add_child(window_get_root_layer(my_window), bitmap_layer_get_layer(s_hour_layer));
  layer_add_child(window_get_root_layer(my_window), bitmap_layer_get_layer(s_ufo_layer));
  
  window_stack_push(my_window, true);
  
  update_time();
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
}

void handle_deinit(void) {
  persist_write_bool(KEY_HOUR_DIAL, SunHourFlag);
  persist_write_bool(KEY_PTOLMAIC, ScienceMovesForwardFlag);
  bitmap_layer_destroy(s_stars_layer);
  bitmap_layer_destroy(s_earth_layer);
  bitmap_layer_destroy(s_min_layer);
  bitmap_layer_destroy(s_hour_layer);
  bitmap_layer_destroy(s_ufo_layer);
  window_destroy(my_window);
  tick_timer_service_unsubscribe();
}

int main(void) {
  handle_init();
  app_event_loop();
  handle_deinit();
}