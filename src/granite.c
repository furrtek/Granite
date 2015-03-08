#include <pebble.h>
 
Window *window;
BitmapLayer *digit_layer;
Layer *bg_layer;
GBitmap *canvas_bmp;
GBitmap *numbers_bmp;

uint32_t icos;
uint32_t angle[4];

#define REFRESH 40

#ifdef PBL_COLOR
	uint8_t *numbers_data;
	uint8_t *canvas_data;
#endif

bool tapped = false;

void render_3d_pane(uint8_t d, uint16_t start, uint16_t stop) {
	uint8_t ya, yb, wa, wb, c, x, y, cl, putx, puty, getxx, getyy, ci, px;
	int16_t getxd, getyd, getx, gety, wd, w;
	
	px = d * 38;
	
	ya = 35 + ((cos_lookup(angle[d] + start) * 35)>>16);
	yb = 35 + ((cos_lookup(angle[d] + stop) * 35)>>16);
	wa = 16 - ((cos_lookup((angle[d] + start) * 2) * 4)>>16);
	wb = 16 - ((cos_lookup((angle[d] + stop) * 2) * 4)>>16);
	
	c = yb - ya;
	w = wa<<8;
    if (c > 0) {
		wd = ((wb - wa)<<8) / c;
		gety = 0;
		getyd = (70<<8) / c;
		for (y = 0; y < c; y++) {
			getx = 0;
			getxd = (32<<8) / ((w>>8) * 2);
			for (x = 0; x < (w>>8) * 2; x++) {
				getxx = getx>>8;
				getyy = gety>>8;
				#ifdef PBL_COLOR
					cl = ((uint8_t *)numbers_data)[(d*32)+getxx+(getyy*320)];
					if (cl&0x3F)
						cl=0x80;
					else
						cl=0;
				#else
					cl = ((uint8_t *)numbers_bmp->addr)[(d*4)+(getxx>>3)+(getyy*40)];
					cl = (cl << (7-(getxx&7))) & 0x80;
				#endif
				putx = 20 + x - (w>>8) + px;
				puty = ya + y;
				#ifdef PBL_COLOR
					ci = ((uint8_t*)canvas_data)[(putx>>3)+(puty*20)];
					ci |= (cl >> (7-(putx&7)));
					((uint8_t*)canvas_data)[(putx>>3)+(puty*20)] = ci;
				#else
					ci = ((uint8_t*)canvas_bmp->addr)[(putx>>3)+(puty*20)];
					ci |= (cl >> (7-(putx&7)));
					((uint8_t*)canvas_bmp->addr)[(putx>>3)+(puty*20)] = ci;
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
		memset(canvas_data,0,160*80/8);
	#else
		memset(canvas_bmp->addr,0,160*80/8);
	#endif
	
	for (d = 0; d < 4; d++) {
		render_3d_pane(d,(0x7FFF * 3 / 4),(0x7FFF * 5 / 4));
		render_3d_pane(d,(0x7FFF * 5 / 4),(0x7FFF * 7 / 4));
	}
	
	layer_mark_dirty(bitmap_layer_get_layer(digit_layer));
}

void shake(void *data) {
	uint8_t d;
	render();
	for (d = 0; d < 4; d++) {
		angle[d] += (0x400 + (cos_lookup(icos+(d*0x1000))>>6));
		if (angle[d] > 0x3FFF) angle[d] = 0;
	}
	icos += 500;
	if (icos < 0xFFFF) app_timer_register(REFRESH, shake, NULL);
}

void handle_timechanges(struct tm *tick_time, TimeUnits units_changed) {
	uint8_t d;
	/*static char time_buffer[10];
	static char date_buffer[10];
	strftime(time_buffer, sizeof(time_buffer), "%H:%M:%S", tick_time);
	text_layer_set_text(text_layer, time_buffer);
	strftime(date_buffer, sizeof(date_buffer), "%b %e", tick_time);
	text_layer_set_text(date_layer, date_buffer);*/
	if (tapped == false) {
		render();
		for (d = 0; d < 4; d++) {
			angle[d] += 400;
			if (angle[d] > 0x3FFF) angle[d] = 0;
		}
	}
}
	
static void tap_handler(AccelAxisType axis, int32_t direction) {
	if (abs(direction) > 1000) {
		icos = 0;
		app_timer_register(REFRESH, shake, NULL);
		tapped = true;
	}
}
	
void bg_fill(Layer *me, GContext* ctx) {
	graphics_context_set_fill_color(ctx,GColorBlack);
	graphics_context_set_stroke_color(ctx,GColorBlack);
	graphics_fill_rect(ctx, (GRect) {{0,0},{144,168}}, 0, 0);
}
 
void handle_init(void) {
	uint8_t d;

	window = window_create();
	Layer *window_layer = window_get_root_layer(window);
	GRect bounds = layer_get_frame(window_layer);
	
	bg_layer = layer_create(GRect(0, 0, 144, 168));
	layer_set_update_proc(bg_layer, bg_fill);
	layer_add_child(window_layer, bg_layer);
	
	digit_layer = bitmap_layer_create(GRect(-4,bounds.origin.y,152,168));
	#ifdef PBL_COLOR
		canvas_bmp = gbitmap_create_blank(GSize(160, 80),GBitmapFormat1Bit);
	#else
		canvas_bmp = gbitmap_create_blank(GSize(160, 80));
	#endif
	numbers_bmp = gbitmap_create_with_resource(RESOURCE_ID_NUMBERS);
	bitmap_layer_set_bitmap(digit_layer, canvas_bmp);
	//bitmap_layer_set_compositing_mode(digit_layer, GCompOpSet);
	layer_add_child(window_layer, bitmap_layer_get_layer(digit_layer));
	
	/*#ifdef PBL_COLOR
		APP_LOG(APP_LOG_LEVEL_DEBUG,"Format: %u",gbitmap_get_format(numbers_bmp));
	#endif*/
	
	#ifdef PBL_COLOR
		numbers_data = gbitmap_get_data(numbers_bmp);
		canvas_data = gbitmap_get_data(canvas_bmp);
	#endif
	
	for (d=0;d<4;d++) {
		angle[d] = d*0x1000;
	}
	
	accel_tap_service_subscribe(tap_handler);
	
	time_t now = time(NULL);
	//handle_timechanges(localtime(&now), SECOND_UNIT);
	tick_timer_service_subscribe(SECOND_UNIT, handle_timechanges);

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
