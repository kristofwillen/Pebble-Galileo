#include <pebble.h>

Window *my_window;
static GBitmap *s_sun_bitmap, *s_moon_bitmap, *s_earth_bitmap, *s_stars_bitmap, *s_ufo_bitmap;
static BitmapLayer *s_sun_layer, *s_moon_layer, *s_earth_layer, *s_stars_layer, *s_ufo_layer;
static int xsun, ysun, xmoon, ymoon, oldxsun, oldysun, oldxmoon, oldymoon = 0;
  

void animation_stopped(Animation *animation, bool finished, void *data) {
   
  layer_set_hidden((Layer *)s_ufo_layer, true);
  
  // Cleanup animations, maybe this fixes the crashes ?
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
  static PropertyAnimation *s_sun_animation, *s_moon_animation, *s_ufo_animation;
  static GRect sun_from_frame, sun_to_frame, moon_to_frame, moon_from_frame, ufo_to_frame, ufo_from_frame;
  static int xufo, yufo;
  
  int sun_distance   = 60;
  int moon_distance  = 40;
  int32_t sun_angle  = TRIG_MAX_ANGLE * t->tm_min/60;
  int32_t moon_angle = (TRIG_MAX_ANGLE * (((t->tm_hour % 12) * 6) + (t->tm_min / 10))) / (12 * 6);
  
  xsun = (int16_t)(sin_lookup(sun_angle)    * sun_distance  / TRIG_MAX_RATIO) + 72 - 16;
  ysun = (int16_t)(-cos_lookup(sun_angle)   * sun_distance  / TRIG_MAX_RATIO) + 86 - 16;
  xmoon = (int16_t)(sin_lookup(moon_angle)  * moon_distance / TRIG_MAX_RATIO) + 72 - 14;
  ymoon = (int16_t)(-cos_lookup(moon_angle) * moon_distance / TRIG_MAX_RATIO) + 86 - 14;
  
  sun_from_frame  = GRect(oldxsun, oldysun, 32, 32);
  sun_to_frame    = GRect(xsun, ysun, 32, 32);
  
  moon_from_frame = GRect(oldxmoon, oldymoon, 28,28);
  moon_to_frame   = GRect(xmoon, ymoon, 28,28);
  
  s_sun_animation = property_animation_create_layer_frame((Layer *)s_sun_layer, &sun_from_frame, &sun_to_frame);
  animation_set_duration((Animation *)s_sun_animation,1000);
  
  
  s_moon_animation = property_animation_create_layer_frame((Layer *)s_moon_layer, &moon_from_frame, &moon_to_frame);
  //animation_set_duration((Animation *)s_moon_animation,100);

  animation_set_handlers((Animation*) s_sun_animation, (AnimationHandlers) {
    .stopped = (AnimationStoppedHandler) animation_stopped,
  }, NULL);
  
  animation_set_handlers((Animation*) s_moon_animation, (AnimationHandlers) {
    .stopped = (AnimationStoppedHandler) animation_stopped,
  }, NULL);
  
  animation_schedule((Animation*) s_sun_animation);
  animation_schedule((Animation*) s_moon_animation);
  
  oldxsun  = xsun;
  oldysun  = ysun;
  oldxmoon = xmoon;
  oldymoon = ymoon;
  
  if ((t->tm_min % 10) == 0) {
    xufo = rand() % 144;
    yufo = rand() % 168;
    ufo_from_frame  = GRect(-29,yufo,29,16);
    ufo_to_frame    = GRect(xufo,168,29,16);
    layer_set_hidden((Layer *)s_ufo_layer, false);
    s_ufo_animation = property_animation_create_layer_frame((Layer *)s_ufo_layer, &ufo_from_frame, &ufo_to_frame);
    
    animation_set_handlers((Animation*) s_ufo_animation, (AnimationHandlers) {
      .stopped = (AnimationStoppedHandler) animation_stopped,
    }, NULL);
  
    animation_set_duration((Animation *)s_ufo_animation,5000);
    animation_schedule((Animation*) s_ufo_animation);
    
  }
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();
  
}


void handle_init(void) {
  my_window = window_create();
 
  s_earth_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_EARTH_ICON);
  s_sun_bitmap   = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_SUN_SMALL_ICON);
  s_moon_bitmap  = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_MOON_SMALL_ICON);
  s_stars_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_STARS_BACKGROUND);
  s_ufo_bitmap   = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_UFO_ICON);
  
  s_stars_layer = bitmap_layer_create(GRect(0,0,144,168));
  bitmap_layer_set_bitmap(s_stars_layer, s_stars_bitmap);
  
  s_earth_layer = bitmap_layer_create(GRect(52, 68, 40, 32));
  bitmap_layer_set_bitmap(s_earth_layer, s_earth_bitmap);
  
  s_sun_layer = bitmap_layer_create(GRect(58,72,32,32));
  bitmap_layer_set_bitmap(s_sun_layer, s_sun_bitmap);
  
  s_moon_layer = bitmap_layer_create(GRect(58,72,28,28));
  bitmap_layer_set_bitmap(s_moon_layer, s_moon_bitmap);

  s_ufo_layer = bitmap_layer_create(GRect(52, 68, 29, 16));
  bitmap_layer_set_bitmap(s_ufo_layer, s_ufo_bitmap);
  layer_set_hidden((Layer *)s_ufo_layer, true);
  
  #ifdef PBL_COLOR
    bitmap_layer_set_compositing_mode(s_moon_layer,GCompOpSet);
    bitmap_layer_set_compositing_mode(s_sun_layer,GCompOpSet);
  #else
    bitmap_layer_set_compositing_mode(s_moon_layer,GCompOpOr);
    bitmap_layer_set_compositing_mode(s_sun_layer,GCompOpOr);
    bitmap_layer_set_compositing_mode(s_ufo_layer,GCompOpOr);
    bitmap_layer_set_compositing_mode(s_stars_layer,GCompOpAnd);
  #endif  
  
  layer_add_child(window_get_root_layer(my_window), bitmap_layer_get_layer(s_stars_layer));  
  layer_add_child(window_get_root_layer(my_window), bitmap_layer_get_layer(s_earth_layer));
  layer_add_child(window_get_root_layer(my_window), bitmap_layer_get_layer(s_sun_layer));
  layer_add_child(window_get_root_layer(my_window), bitmap_layer_get_layer(s_moon_layer));
  layer_add_child(window_get_root_layer(my_window), bitmap_layer_get_layer(s_ufo_layer));
  
  window_stack_push(my_window, true);
  
  update_time();
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
}

void handle_deinit(void) {
  bitmap_layer_destroy(s_stars_layer);
  bitmap_layer_destroy(s_earth_layer);
  bitmap_layer_destroy(s_sun_layer);
  bitmap_layer_destroy(s_moon_layer);
  bitmap_layer_destroy(s_ufo_layer);
  window_destroy(my_window);
}

int main(void) {
  handle_init();
  app_event_loop();
  handle_deinit();
}
