#include <pebble.h>
 
Window *window;
BitmapLayer *digit_layer;
Layer *bg_layer;
GBitmap *canvas_bmp;
GBitmap *numbers_bmp;

uint32_t icos;
int16_t angle[4];
uint8_t digit_top[4];
uint8_t digit_bottom[4];

#define REFRESH 40

#ifdef PBL_COLOR
	uint8_t *numbers_data;
	uint8_t *canvas_data;
#endif

bool tapped = false;

void render_3d_pane(uint8_t d, uint8_t i, uint16_t start, uint16_t stop) {
	uint8_t ya, yb, wa, wb, c, x, y, cl, xo, putx, puty, getxx, getyy, ci;
	int16_t getxd, getyd, getx, gety, wd, w;
	
	ya = 45 + ((cos_lookup(angle[i] + start) * 45) >> 16);
	yb = 45 + ((cos_lookup(angle[i] + stop) * 45) >> 16);
	wa = 16 - ((cos_lookup((angle[i] + start) * 2) * 4) >> 16);
	wb = 16 - ((cos_lookup((angle[i] + stop) * 2) * 4) >> 16);
	
	xo = i * 38;
	c = yb - ya;
	w = wa << 8;
    if (c > 0) {
		wd = ((wb - wa) << 8) / c;
		gety = 0;
		getyd = (80 << 8) / c;
		for (y = 0; y < c; y++) {
			getx = 0;
			getxd = (32 << 8) / ((w >> 8) * 2);
			for (x = 0; x < (w >> 8) * 2; x++) {
				getxx = getx >> 8;
				getyy = gety >> 8;
				#ifdef PBL_COLOR
					cl = ((uint8_t *)numbers_data)[(d * 32) + getxx + (getyy * 320)];
					if (cl & 0x3F)
						cl = 0x80;
					else
						cl = 0;
				#else
					cl = ((uint8_t *)numbers_bmp->addr)[(d * 4) + (getxx >> 3) + (getyy * 40)];
					cl = (cl << (7 - (getxx & 7))) & 0x80;
				#endif
				putx = 20 + x - (w >> 8) + xo;
				puty = ya + y;
				#ifdef PBL_COLOR
					ci = ((uint8_t*)canvas_data)[(putx >> 3) + (puty * 20)];
					ci |= (cl >> (7 - (putx & 7)));
					((uint8_t*)canvas_data)[( putx>>3 ) + (puty * 20)] = ci;
				#else
					ci = ((uint8_t*)canvas_bmp->addr)[(putx >> 3) + (puty * 20)];
					ci |= (cl >> (7 - (putx & 7)));
					((uint8_t*)canvas_bmp->addr)[(putx >> 3) + (puty * 20)] = ci;
				#endif
				getx = getx + getxd;
			}
			gety = gety + getyd;
			w = w + wd;
		}
	}	
}

void render() {
	uint8_t d;
	
	#ifdef PBL_COLOR
		memset(canvas_data,0,160 * 90 / 8);
	#else
		memset(canvas_bmp->addr,0,160 * 90 / 8);
	#endif
	
	for (d = 0; d < 4; d++) {
		render_3d_pane(digit_top[d],d,(0x7FFF * 3 / 4),(0x7FFF * 5 / 4));
		render_3d_pane(digit_bottom[d],d,(0x7FFF * 5 / 4),(0x7FFF * 7 / 4));
	}
	
	layer_mark_dirty(bitmap_layer_get_layer(digit_layer));
}

void shake(void *data) {
	uint8_t d;
	render();
	for (d = 0; d < 4; d++) {
		angle[d] += cos_lookup(icos<<2)>>7;
		if (angle[d] > 0x3FFF) angle[d] = 0;
		if (angle[d] < 0) angle[d] = 0x3FFF;
	}
	icos += 0x400;
	if (icos < 0x10000)
		app_timer_register(REFRESH, shake, NULL);
	else
		tapped = false;
}

bool rotob;
uint8_t rotomask;

void roto(void *data) {
	uint8_t d;
	rotob = false;
	for (d = 0; d < 4; d++) {
		if (rotomask & (1<<d)) {
			if (angle[d] < 0x3000) {
				rotob = true;
				angle[d] += 0x400;
				render();
				app_timer_register(REFRESH, roto, NULL);
			}
		}
	}
	if (rotob == false) rotomask = 0;
}

void handle_timechanges(struct tm *tick_time, TimeUnits units_changed) {
	uint8_t d;
	uint8_t sec_unit, sec_ten, min_unit, min_ten, hour_unit, hour_ten;

	if (tapped == false) {

		if (rotob == false) {
			hour_unit = (tick_time->tm_hour % 10);
			hour_ten = (tick_time->tm_hour / 10);
			min_unit = (tick_time->tm_min % 10);
			min_ten = (tick_time->tm_min / 10);
			//sec_unit = (tick_time->tm_sec % 10);
			sec_ten = (tick_time->tm_sec / 10);
			
			if (hour_unit == 0) {
				rotob = true;
				rotomask |= 1;
				app_timer_register(REFRESH, roto, NULL);
			} else {
				angle[0] = 0x3000 + ((hour_unit * 0x2000) / 10);
			}
			if (angle[0] > 0x3FFF) {
				angle[0] -= 0x4000;
				digit_top[0] = (hour_ten + 1) % 6;	
				digit_bottom[0] = hour_ten;
			} else {
				digit_top[0] = hour_ten;
				digit_bottom[0] = (hour_ten - 1) % 6;
			}
			
			if (min_ten == 0) {
				rotob = true;
				rotomask |= 2;
				app_timer_register(REFRESH, roto, NULL);
			} else {
				angle[1] = 0x3000 + ((min_ten * 0x2000) / 6);
			}
			if (angle[1] > 0x3FFF) {
				angle[1] -= 0x4000;
				digit_top[1] = (hour_unit + 1) % 10;	
				digit_bottom[1] = hour_unit;
			} else {
				digit_top[1] = hour_unit;
				digit_bottom[1] = (hour_unit - 1) % 10;
			}
			
			if (min_unit == 0) {
				rotob = true;
				rotomask |= 4;
				app_timer_register(REFRESH, roto, NULL);
			} else {
				angle[2] = 0x3000 + ((min_unit * 0x2000) / 10);
			}
			if (angle[2] > 0x3FFF) {
				angle[2] -= 0x4000;
				digit_top[2] = (min_ten + 1) % 6;
				digit_bottom[2] = min_ten;
			} else {
				digit_top[2] = min_ten;
				digit_bottom[2] = (min_ten - 1) % 6;
			}
			
			if (sec_ten == 0) {
				rotob = true;
				rotomask |= 8;
				app_timer_register(REFRESH, roto, NULL);
			} else {
				angle[3] = 0x3000 + ((sec_ten * 0x2000) / 6);
			}
			if (angle[3] > 0x3FFF) {
				angle[3] -= 0x4000;
				digit_top[3] = (min_unit + 1) % 10;	
				digit_bottom[3] = min_unit;
			} else {
				digit_top[3] = min_unit;
				digit_bottom[3] = (min_unit - 1) % 10;
			}
		}
		
		render();
	}
}
	
static void tap_handler(AccelAxisType axis, int32_t direction) {
	if (tapped == false) {
		icos = 0;
		app_timer_register(REFRESH, shake, NULL);
		tapped = true;
	}
}
	
void bg_fill(Layer *me, GContext* ctx) {
	graphics_context_set_fill_color(ctx,GColorBlack);
	graphics_context_set_stroke_color(ctx,GColorBlack);
	graphics_fill_rect(ctx, GRect(0, 0, 144, 168), 0, 0);
}
 
void handle_init(void) {
	uint8_t d;

	window = window_create();
	Layer *window_layer = window_get_root_layer(window);
	GRect bounds = layer_get_frame(window_layer);
	
	bg_layer = layer_create(GRect(0, 0, 144, 168));
	layer_set_update_proc(bg_layer, bg_fill);
	layer_add_child(window_layer, bg_layer);
	
	digit_layer = bitmap_layer_create(GRect(-4, bounds.origin.y, 152, 168));
	#ifdef PBL_COLOR
		canvas_bmp = gbitmap_create_blank(GSize(160, 90),GBitmapFormat1Bit);
	#else
		canvas_bmp = gbitmap_create_blank(GSize(160, 90));
	#endif
	numbers_bmp = gbitmap_create_with_resource(RESOURCE_ID_NUMBERS);
	bitmap_layer_set_bitmap(digit_layer, canvas_bmp);
	//bitmap_layer_set_compositing_mode(digit_layer, GCompOpSet);
	layer_add_child(window_layer, bitmap_layer_get_layer(digit_layer));
	
	#ifdef PBL_COLOR
		numbers_data = gbitmap_get_data(numbers_bmp);
		canvas_data = gbitmap_get_data(canvas_bmp);
	#endif

	accel_tap_service_subscribe(tap_handler);
	
	time_t now = time(NULL);
	handle_timechanges(localtime(&now), MINUTE_UNIT);
	tick_timer_service_subscribe(MINUTE_UNIT, handle_timechanges);

	window_stack_push(window, true);
}
 
void handle_deinit(void) {
	bitmap_layer_destroy(digit_layer);
	window_destroy(window);
}
 
int main(void) {
	handle_init();
	app_event_loop();
	handle_deinit();
} 
