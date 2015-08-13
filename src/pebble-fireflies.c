#include <stdio.h>
#include <math.h>
#include <pebble.h>
#include "tinymt32.h"
#include "numbers.h"

#define maximum(a,b) a > b ? a : b
#define minimum(a,b) ((a) < (b) ? (a) : (b))
#define NUM_PARTICLES 140
#define COOKIE_ANIMATION_TIMER 1
#define COOKIE_SWARM_TIMER 2
#define COOKIE_DISPERSE_TIMER 3
#define GOAL_DISTANCE 16
#define NORMAL_POWER 400.0F
#define TIGHT_POWER 1.0F
#define MAX_SPEED 1.0F
#define SWARM_SPEED 2.0F
#define SCREEN_MARGIN 0.0F
#define JITTER 0.1F
#define MAX_SIZE 3.0F
#define MIN_SIZE 0.0F


  #ifdef PBL_COLOR
  int color_select[NUM_PARTICLES];
  struct GBitmap *s_0_bitmap;
  struct GBitmap *s_1_bitmap;
  struct GBitmap *s_2_bitmap;
  struct GBitmap *s_3_bitmap;
  struct GBitmap *s_4_bitmap;
  struct GBitmap *s_5_bitmap;
  struct GBitmap *s_6_bitmap;
  struct GBitmap *s_7_bitmap;
  struct GBitmap *s_8_bitmap;
  struct GBitmap *s_9_bitmap;
  #else
  static const GBitmap *number_bitmaps[10] = { 
    &s_0_bitmap, &s_1_bitmap, &s_2_bitmap, 
    &s_3_bitmap, &s_4_bitmap, &s_5_bitmap,
    &s_6_bitmap, &s_7_bitmap, &s_8_bitmap, 
    &s_9_bitmap 
  };
  #endif




typedef struct FPoint
{
  float x;
  float y;
} FPoint;

// typedefs
typedef struct FParticle 
{
  FPoint position;
  FPoint grav_center;
  float dx;
  float dy;
  float power;
  float size;
  float goal_size;
  float ds;
  bool swarming;
} FParticle;
#define FParticle(px, py, gx, gy, power) ((FParticle){{(px), (py)}, {(gx), (gy)}, 0.0F, 0.0F, power, 0.0F, 0.0F, 0.0F, false})
#define FPoint(x, y) ((FPoint){(x), (y)})

// globals
FParticle particles[NUM_PARTICLES];
// TODO: Figure out what replaces AppContextRef in 2.0.
// AppContextRef app;
Window *window;
Layer *particle_layer;
TextLayer *text_header_layer;
tinymt32_t rndstate;
int showing_time = 0;
int color = 0;

int random_in_range(int min, int max) {
  return min + (int)(tinymt32_generate_float01(&rndstate) * ((max - min) + 1));
}

float random_in_rangef(float min, float max) {
  return min + (float)(tinymt32_generate_float01(&rndstate) * ((max - min)));
}

GPoint random_point_in_screen() {
  GRect window_bounds = layer_get_bounds(window_get_root_layer(window));
  return GPoint(random_in_range(0, window_bounds.size.w+1),
		random_in_range(0, window_bounds.size.h+1));
}

GPoint random_point_roughly_in_screen(int margin, int padding) {
  GRect window_bounds = layer_get_bounds(window_get_root_layer(window));
  return GPoint(random_in_range(0-margin+padding, window_bounds.size.w+1+margin-padding),
		random_in_range(0-margin+padding, window_bounds.size.h+1+margin-padding));
}

void update_particle(int i) {
  if(tinymt32_generate_float01(&rndstate) < 0.4F) {
    particles[i].dx += random_in_rangef(-JITTER, JITTER);
    particles[i].dy += random_in_rangef(-JITTER, JITTER);
  }

  // gravitate towards goal
  particles[i].dx += -(particles[i].position.x - particles[i].grav_center.x)/particles[i].power;
  particles[i].dy += -(particles[i].position.y - particles[i].grav_center.y)/particles[i].power;

  // stop swarming if goal reached
  if (particles[i].swarming) {
    float dx, dy;
    dx = particles[i].position.x - particles[i].grav_center.x;
    dy = particles[i].position.y - particles[i].grav_center.y;
    if (((dx*dx)+(dy*dy)) < GOAL_DISTANCE) {
      particles[i].swarming = false;
    }
  }

  // damping
  particles[i].dx *= 0.999F;
  particles[i].dy *= 0.999F;

  // snap to max
  float speed;
  if (particles[i].swarming)
    speed = SWARM_SPEED;
  else
    speed = MAX_SPEED;
  if(particles[i].dx >  speed) particles[i].dx =  speed;
  if(particles[i].dx < -speed) particles[i].dx = -speed;
  if(particles[i].dy >  speed) particles[i].dy =  speed;
  if(particles[i].dy < -speed) particles[i].dy = -speed;

  particles[i].position.x += particles[i].dx;
  particles[i].position.y += particles[i].dy;

  // update size
  // when we're showing the time, don't blink like you normally would
  if(showing_time == 0) {
    if((abs(particles[i].size - MIN_SIZE) < 0.001F) && (tinymt32_generate_float01(&rndstate) < 0.0008F)) {
      particles[i].goal_size = MAX_SIZE;
    }

    if(abs(particles[i].size - MAX_SIZE) < 0.001F) {
      particles[i].goal_size = MIN_SIZE;
    }
  }

  particles[i].ds += -(particles[i].size - particles[i].goal_size)/random_in_rangef(1000.0F, 5000.0F);
  if(abs(particles[i].size - particles[i].goal_size) > 0.01) {
    particles[i].size += particles[i].ds;
  }
  if(particles[i].size > MAX_SIZE) particles[i].size = MAX_SIZE;
  if(particles[i].size < MIN_SIZE) particles[i].size = MIN_SIZE;
}

void draw_particle(GContext* ctx, int i) {
  graphics_fill_circle(ctx, GPoint((int)particles[i].position.x,
                                   (int)particles[i].position.y),
                       particles[i].size);
}

void update_particles_layer(Layer *me, GContext* ctx) {
  (void)me;
  #ifdef PBL_COLOR
     int colorchange = random_in_range(0, NUM_PARTICLES);
  #else
  #endif
  for(int i=0;i<NUM_PARTICLES;i++) {
  #ifdef PBL_COLOR
     if (colorchange == i)
     {
     if (color_select[i] == 1)
     {
     color_select[i] = 0;
     }
     else{
     color_select[i] = 1;    
     }
     }
     if (color_select[i] == 0)
     {
     graphics_context_set_fill_color(ctx, GColorBabyBlueEyes);
     }
     else
     {
     graphics_context_set_fill_color(ctx, GColorMalachite);  
     }

  #else
    graphics_context_set_fill_color(ctx, GColorWhite);
  #endif
    update_particle(i);
    draw_particle(ctx, i);
  }
}

void swarm_to_a_different_location() {
  GPoint new_gravity = random_point_roughly_in_screen(0, 30);
  FPoint new_gravityf = FPoint(new_gravity.x, new_gravity.y);
  for(int i=0;i<NUM_PARTICLES;i++) {
    particles[i].grav_center = new_gravityf;
  }
}

void disperse_particles() {
  for(int i=0;i<NUM_PARTICLES;i++) {
    particles[i].power = NORMAL_POWER;
    particles[i].goal_size = 0.0F;
    particles[i].swarming = false;
  }
  swarm_to_a_different_location();
}

void handle_animation_timer() {
  layer_mark_dirty(particle_layer);
  app_timer_register(50, handle_animation_timer, NULL);
}

void handle_swarm_timer() {
  if (showing_time == 0) {
    swarm_to_a_different_location();
  }
  app_timer_register(random_in_range(5000, 15000), handle_swarm_timer, NULL);
}

void handle_disperse_timer() {
  showing_time = 0;
  disperse_particles();
}

// Hopefully this is all replaced by the handle_*_timer functions above.
/* void handle_timer(AppContextRef ctx, AppTimerHandle handle, uint32_t cookie) { */
/*   (void)ctx; */
/*   (void)handle; */

/*   if (cookie == COOKIE_ANIMATION_TIMER) { */
/*      layer_mark_dirty(&particle_layer); */
/*      timer_handle = app_timer_send_event(ctx, 50 /\* milliseconds *\/, COOKIE_ANIMATION_TIMER); */
/*   } else if (cookie == COOKIE_SWARM_TIMER) { */
/*     if(showing_time == 0) { */
/*       swarm_to_a_different_location(); */
/*     } */
/*     app_timer_send_event(ctx, random_in_range(5000,15000) /\* milliseconds *\/, COOKIE_SWARM_TIMER); */
/*   } else if (cookie == COOKIE_DISPERSE_TIMER) { */
/*     showing_time = 0; */
/*     disperse_particles(); */
/*   } */
/* } */

void layer_update_callback(Layer *me, GContext* ctx) {
  (void)me;
  (void)ctx;
}

void swarm_to_digit(int digit, int start_idx, int end_idx, int offset_x, int offset_y) {
  int end = minimum(end_idx, NUM_PARTICLES);

  for(int i=start_idx; i<end; i++) {


  #ifdef PBL_COLOR
  	struct GBitmap *bitmap;
	switch (digit) {
		case 0: bitmap = s_0_bitmap; break;
		case 1: bitmap = s_1_bitmap; break;
		case 2: bitmap = s_2_bitmap; break;
		case 3: bitmap = s_3_bitmap; break;
		case 4: bitmap = s_4_bitmap; break;
		case 5: bitmap = s_5_bitmap; break;
		case 6: bitmap = s_6_bitmap; break;
		case 7: bitmap = s_7_bitmap; break;
		case 8: bitmap = s_8_bitmap; break;
		case 9: bitmap = s_9_bitmap; break;
		default: bitmap = s_0_bitmap; break;
	}
  #else
  GBitmap bitmap   = *number_bitmaps[digit];
  #endif
     	
  #ifdef PBL_COLOR
    uint8_t *pixels = gbitmap_get_data(bitmap); 
  #else
    const uint8_t *pixels = bitmap.addr;
  #endif




    // pick a random point in the four image
    // get the image
    // pick a random number the length of the array
    // if the value is zero, pick a differnet number increment until you get there
    // move to that position

  #ifdef PBL_COLOR
      int hight = gbitmap_get_bounds(bitmap).size.h - 1;
      int length = gbitmap_get_bytes_per_row(bitmap);
      int image_length = length * hight;
  #else
      int image_length = bitmap.row_size_bytes * bitmap.bounds.size.h - 1;
  #endif


    int idx = random_in_range(0, image_length);
    uint8_t pixel = pixels[idx];

    // guarantee we're on a non-zero byte
    while(pixel == 0x00) {
     idx = random_in_range(0, image_length);
     pixel = pixels[idx];
    }

    // pick a non-zero bit
    int bit_pos = (idx * 8);
    int bit_add = random_in_range(0,7);
    int tries = 8;
    while(!(pixel & (1 << bit_add))) {
      if(tries < 0) break;
      bit_add = random_in_range(0,7);
      tries--;
    }
    bit_pos += bit_add;

    #ifdef PBL_COLOR
      int row_size_bits = gbitmap_get_bytes_per_row(bitmap) * 8;
    #else
      int row_size_bits = bitmap.row_size_bytes * 8;
    #endif
    


    int pixel_row = bit_pos / row_size_bits;
    int pixel_col = bit_pos % row_size_bits;

    float scale = 1.0F;
    GPoint goal = GPoint(scale*pixel_col+offset_x, scale*pixel_row+offset_y); // switch row & col

    particles[i].grav_center = FPoint(goal.x, goal.y);
    particles[i].power = TIGHT_POWER;
    particles[i].goal_size = random_in_rangef(2.0F, 3.5F);
    particles[i].swarming = true;
  }

}

unsigned short get_display_hour(unsigned short hour) {
  if (clock_is_24h_style()) { return hour; }
  unsigned short display_hour = hour % 12;
  return display_hour ? display_hour : 12; // Converts "0" to "12"
}

void display_time(struct tm *tick_time) {
  showing_time = 1;
  unsigned short hour = get_display_hour(tick_time->tm_hour);
  int min = tick_time->tm_min;

  //int particles_per_group = NUM_PARTICLES / 2;

  int hr_digit_tens = hour / 10; 
  int hr_digit_ones = hour % 10; 
  int min_digit_tens = min / 10; 
  int min_digit_ones = min % 10; 
  // GRect window_bounds = layer_get_bounds(window_get_root_layer(window));
  // int w = window_bounds.size.w;
  // int h = window_bounds.size.h;

  // take out 5 particles
  // 2 for colon
  // 3 for floaters
  int save = 5;

  if(hr_digit_tens == 0) {
    int particles_per_group = (NUM_PARTICLES - save)/ 3;
    swarm_to_digit(hr_digit_ones,                          0, particles_per_group,   25, 60);
    swarm_to_digit(min_digit_tens,     particles_per_group, particles_per_group*2,   65, 60);
    swarm_to_digit(min_digit_ones, (particles_per_group*2), NUM_PARTICLES - save,    95, 60);

    // top colon
    particles[NUM_PARTICLES-2].grav_center = FPoint(57, 69);
    particles[NUM_PARTICLES-2].power = TIGHT_POWER;
    particles[NUM_PARTICLES-2].goal_size = 3.0F;

    // bottom colon
    particles[NUM_PARTICLES-1].grav_center = FPoint(57, 89);
    particles[NUM_PARTICLES-1].power = TIGHT_POWER;
    particles[NUM_PARTICLES-1].goal_size = 3.0F;

  } else {
    int particles_per_group = (NUM_PARTICLES - save)/ 4;
    swarm_to_digit(hr_digit_tens,                          0, particles_per_group,   10, 60);
    swarm_to_digit(hr_digit_ones,      particles_per_group, particles_per_group*2, 40, 60);
    swarm_to_digit(min_digit_tens, (particles_per_group*2), particles_per_group*3, 80, 60);
    swarm_to_digit(min_digit_ones, (particles_per_group*3), NUM_PARTICLES - save,  110, 60);

    // top colon
    particles[NUM_PARTICLES-2].grav_center = FPoint(68, 69);
    particles[NUM_PARTICLES-2].power = TIGHT_POWER;
    particles[NUM_PARTICLES-2].goal_size = 3.0F;

    // bottom colon
    particles[NUM_PARTICLES-1].grav_center = FPoint(68, 89);
    particles[NUM_PARTICLES-1].power = TIGHT_POWER;
    particles[NUM_PARTICLES-1].goal_size = 3.0F;
  }

}

void kickoff_display_time() {
  time_t t = time(NULL);
  struct tm *current_time = localtime(&t);
  display_time(current_time);
  app_timer_register(40000, handle_disperse_timer, NULL);
}

void handle_tick(struct tm *now, TimeUnits units_changed) {
  kickoff_display_time();
}

void handle_tap(AccelAxisType axis, int32_t direction) {
  kickoff_display_time();
}

void init_particles() {
  GRect window_bounds = layer_get_bounds(window_get_root_layer(window));
  for(int i=0; i<NUM_PARTICLES; i++) {

    GPoint start = random_point_roughly_in_screen(10, 0);
    GPoint goal = GPoint(window_bounds.size.w/2, window_bounds.size.h/2);

    // GPoint start = goal;
    float initial_power = NORMAL_POWER;
    // float initial_power = 1.0F;
    particles[i] = FParticle(start.x, start.y, 
                             goal.x, goal.y, 
                             initial_power);
    particles[i].size = particles[i].goal_size = 0.0F;
  }
}

void handle_init() {
  #ifdef PBL_COLOR
  int inv = 0;
  for (int o = 0; o < NUM_PARTICLES ;o++)
  {
     inv = o % 2;
     if (inv > 0)
     {
	color_select[o] = 1;
     }
     else
     {
	color_select[o] = 0;
     }
  }

  s_0_bitmap = gbitmap_create_with_data(s_0_pixels);
  s_1_bitmap = gbitmap_create_with_data(s_1_pixels);
  s_2_bitmap = gbitmap_create_with_data(s_2_pixels);
  s_3_bitmap = gbitmap_create_with_data(s_3_pixels);
  s_4_bitmap = gbitmap_create_with_data(s_4_pixels);
  s_5_bitmap = gbitmap_create_with_data(s_5_pixels);
  s_6_bitmap = gbitmap_create_with_data(s_6_pixels);
  s_7_bitmap = gbitmap_create_with_data(s_7_pixels);
  s_8_bitmap = gbitmap_create_with_data(s_8_pixels);
  s_9_bitmap = gbitmap_create_with_data(s_9_pixels);
  
  gbitmap_set_data(s_0_bitmap, s_0_pixels, GBitmapFormat1Bit, 4, 0);
  gbitmap_set_data(s_1_bitmap, s_1_pixels, GBitmapFormat1Bit, 4, 0);
  gbitmap_set_data(s_2_bitmap, s_2_pixels, GBitmapFormat1Bit, 4, 0);
  gbitmap_set_data(s_3_bitmap, s_3_pixels, GBitmapFormat1Bit, 4, 0);
  gbitmap_set_data(s_4_bitmap, s_4_pixels, GBitmapFormat1Bit, 4, 0);
  gbitmap_set_data(s_5_bitmap, s_5_pixels, GBitmapFormat1Bit, 4, 0);
  gbitmap_set_data(s_6_bitmap, s_6_pixels, GBitmapFormat1Bit, 4, 0);
  gbitmap_set_data(s_7_bitmap, s_7_pixels, GBitmapFormat1Bit, 4, 0);
  gbitmap_set_data(s_8_bitmap, s_8_pixels, GBitmapFormat1Bit, 4, 0);
  gbitmap_set_data(s_9_bitmap, s_9_pixels, GBitmapFormat1Bit, 4, 0);

  gbitmap_set_bounds(s_0_bitmap, GRect(0, 0, 24, 38));
  gbitmap_set_bounds(s_1_bitmap, GRect(0, 0, 13, 36));
  gbitmap_set_bounds(s_2_bitmap, GRect(0, 0, 24, 37));
  gbitmap_set_bounds(s_3_bitmap, GRect(0, 0, 23, 38));
  gbitmap_set_bounds(s_4_bitmap, GRect(0, 0, 27, 36));
  gbitmap_set_bounds(s_5_bitmap, GRect(0, 0, 22, 37));
  gbitmap_set_bounds(s_6_bitmap, GRect(0, 0, 23, 38));
  gbitmap_set_bounds(s_7_bitmap, GRect(0, 0, 24, 36));
  gbitmap_set_bounds(s_8_bitmap, GRect(0, 0, 24, 38));
  gbitmap_set_bounds(s_9_bitmap, GRect(0, 0, 24, 38));
  #else

  #endif


  uint32_t seed = 4;
  tinymt32_init(&rndstate, seed);

  window = window_create();
  //window_set_fullscreen(window, true);
  window_stack_push(window, true /* Animated */);

  #ifdef PBL_COLOR
   window_set_background_color(window, GColorBlack);
  #else
    window_set_background_color(window, GColorBlack);
  #endif




  // resource_init_current_app(&APP_RESOURCES);

  // Init the layer for the minute display
  // layer_init(&layer, window.layer.frame);
  // layer.update_proc = &layer_update_callback;
  // layer_add_child(&window.layer, &layer);

  init_particles();

  // setup debugging text layer
  GRect window_bounds = layer_get_bounds(window_get_root_layer(window));
  text_header_layer = text_layer_create(window_bounds);
  text_layer_set_text_color(text_header_layer, GColorWhite);
  text_layer_set_background_color(text_header_layer, GColorClear);
  text_layer_set_font(text_header_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(text_header_layer));
 
  particle_layer = layer_create(window_bounds);
  layer_set_update_proc(particle_layer, update_particles_layer);
  layer_add_child(window_get_root_layer(window), particle_layer);

  app_timer_register(50, handle_animation_timer, NULL);
  app_timer_register(random_in_range(5000, 15000), handle_swarm_timer, NULL);

  tick_timer_service_subscribe(MINUTE_UNIT, handle_tick);
  accel_tap_service_subscribe(handle_tap);
}

void handle_deinit() {
  layer_destroy(particle_layer);
  text_layer_destroy(text_header_layer);
  window_destroy(window);
}

int main() {
  handle_init();
  app_event_loop();
  handle_deinit();
}


