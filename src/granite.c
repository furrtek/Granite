//Granite 1.0 watchface for Pebble and Pebble Time
//Furrtek 2015

#include <pebble.h>
 
Window *window;
BitmapLayer *digit_layer;
Layer *bg_layer;
GBitmap *canvas_bmp;

#ifdef PBL_COLOR
	GBitmap *zero_bmp;
	GBitmap *one_bmp;
	GBitmap *two_bmp;
	GBitmap *three_bmp;
	GBitmap *four_bmp;
	GBitmap *five_bmp;
	GBitmap *six_bmp;
	GBitmap *seven_bmp;
	GBitmap *eight_bmp;
	GBitmap *nine_bmp;
#else
	GBitmap *numbers_bmp;
#endif

int32_t icos;
int16_t angle[4];
uint8_t digit_top[4];
uint8_t digit_bottom[4];
bool rotob;
uint8_t rotomask;

#define REFRESH 40

#ifdef PBL_COLOR
	uint8_t *zero_data;
	uint8_t *one_data;
	uint8_t *two_data;
	uint8_t *three_data;
	uint8_t *four_data;
	uint8_t *five_data;
	uint8_t *six_data;
	uint8_t *seven_data;
	uint8_t *eight_data;
	uint8_t *nine_data;
	uint8_t *digit_data;
	uint8_t *canvas_data;
#endif

bool tapped = false;

void render_3d_pane(uint8_t d, uint8_t i, uint16_t start, uint16_t stop) {
	uint8_t ya, yb, wa, wb, c, x, y, cl, xo, putx, puty, getxx, getyy, ci;
	int16_t getxd, getyd, getx, gety, wd, w;
	
	// See accompanying PDF for visual explanations of this routine
	
	// Get top and bottom bounds
	ya = 45 + ((cos_lookup(angle[i] + start) * 45) >> 16);
	yb = 45 + ((cos_lookup(angle[i] + stop) * 45) >> 16);
	
	// Get top and bottom widths
	wa = 16 - ((cos_lookup((angle[i] + start) * 2) * 4) >> 16);
	wb = 16 - ((cos_lookup((angle[i] + stop) * 2) * 4) >> 16);
	
	xo = i * 38;	// Digit X offset from the left
	c = yb - ya;	// Delta (height of pane)
	w = wa << 8;	// Initialize width accumulator
    if (c > 0) {
		wd = ((wb - wa) << 8) / c;	// Width accumulator increment value (width delta / height)
		gety = 0;					// Initialize Y stretch accumulator
		getyd = (80 << 8) / c;		// Y stretch accumulator increment value (digit height / height)
		for (y = 0; y < c; y++) {
			getx = 0;								// Initialize X stretch accumulator
			getxd = (32 << 8) / ((w >> 8) * 2);		// X stretch accumulator increment value (digit width / current line width)
			for (x = 0; x < (w >> 8) * 2; x++) {
				getxx = getx >> 8;					// Get rid of precision
				getyy = gety >> 8;
				#ifdef PBL_COLOR
					// Get pixel color from 8-bit bitmap
					switch (d) {
						case 0:
							digit_data = zero_data;
							break;
						case 1:
							digit_data = one_data;
							break;
						case 2:
							digit_data = two_data;
							break;
						case 3:
							digit_data = three_data;
							break;
						case 4:
							digit_data = four_data;
							break;
						case 5:
							digit_data = five_data;
							break;
						case 6:
							digit_data = six_data;
							break;
						case 7:
							digit_data = seven_data;
							break;
						case 8:
							digit_data = eight_data;
							break;
						case 9:
							digit_data = nine_data;
							break;
					}
					cl = ((uint8_t *)digit_data)[getxx + (getyy * 32)];
				#else
					// Get pixel color from 1-bit bitmap
					cl = ((uint8_t *)numbers_bmp->addr)[(d * 4) + (getxx >> 3) + (getyy * 40)];
					// "Neutralize" bit position
					cl = (cl << (7 - (getxx & 7))) & 0x80;
				#endif
				// Compute X pixel plot position
				putx = 20 + x - (w >> 8) + xo;
				puty = ya + y;
				// The only difference for plotting here is the data access pointer:
				#ifdef PBL_COLOR
					((uint8_t*)canvas_data)[putx + (puty * 160)] = cl;
				#else
					// Get previous 8 pixels (1 byte)
					ci = ((uint8_t*)canvas_bmp->addr)[(putx >> 3) + (puty * 20)];
					// Merge with new pixel
					ci |= (cl >> (7 - (putx & 7)));
					// Put back
					((uint8_t*)canvas_bmp->addr)[(putx >> 3) + (puty * 20)] = ci;
				#endif
				getx = getx + getxd;	// Next pixel
			}
			gety = gety + getyd;		// Next line
			w = w + wd;					// Next width
		}
	}	
}

void render() {
	uint8_t d;
	
	// First, clear canvas with black
	#ifdef PBL_COLOR
		memset(canvas_data, 0, 160 * 90);
	#else
		memset(canvas_bmp->addr, 0, 160 * 90 / 8);
	#endif
	
	// For each of the 4 digits, render top and bottom pane
	for (d = 0; d < 4; d++) {
		render_3d_pane(digit_top[d], d, (0x7FFF * 3 / 4), (0x7FFF * 5 / 4));
		render_3d_pane(digit_bottom[d], d, (0x7FFF * 5 / 4), (0x7FFF * 7 / 4));
	}
	
	// Kindly ask Pebble OS to refresh the layer
	layer_mark_dirty(bitmap_layer_get_layer(digit_layer));
}

void shake(void *data) {
	uint8_t d;
	
	for (d = 0; d < 4; d++) {
		angle[d] += (sin_lookup(icos<<2)>>3) >> ((icos+0x4000)>>14);
		if (angle[d] > 0x3FFF) {
			angle[d] = 0;
			// Swap panes
			digit_bottom[d] = digit_top[d];
			digit_top[d] = (digit_top[d] + 1) % 10;
		}
		if (angle[d] < 0) {
			angle[d] = 0x3FFF;
			// Swap panes
			digit_top[d] = digit_bottom[d];
			// Maybe a simpler way to do this: wrap around 10
			if (digit_bottom[d] - 1 >= 0)
				digit_bottom[d] = (digit_bottom[d] - 1) % 10;
			else
				digit_bottom[d] = 9;
		}
	}
	icos += 0x600;
	if (icos < 0x10000)
		app_timer_register(REFRESH, shake, NULL);
	else
		tapped = false;
		
	render();
}

void roto(void *data) {
	uint8_t d;
	
	rotob = false;
	
	for (d = 0; d < 4; d++) {
		// See if digit needs to be rotated smoothly
		if (rotomask & (1<<d)) {
			if (angle[d] < 0x3000) {
				rotob = true;
				angle[d] += 0x200;
				render();
				app_timer_register(REFRESH, roto, NULL);
			}
		}
	}
	if (rotob == false) rotomask = 0;	// All rotations done
}

void handle_timechanges(struct tm *tick_time, TimeUnits units_changed) {
	uint8_t sec_ten, min_unit, min_ten, hour_unit, hour_ten;
	bool needrender;

	if (tapped == false) {

		if (rotob == false) {
			
			// By default, nothing has to change on screen (9/10th of the time)
			needrender = false;
			
			// Get individual digits from current time
			hour_ten = (tick_time->tm_hour / 10);
			hour_unit = (tick_time->tm_hour % 10);
			min_ten = (tick_time->tm_min / 10);
			min_unit = (tick_time->tm_min % 10);
			sec_ten = (tick_time->tm_sec / 10);
			
			// 4 times (almost) the same thing for each digit:
			
			if (hour_unit == 0) {
				// Asks for smooth rotation (wrap)...
				rotob = true;
				rotomask |= 1;
				app_timer_register(REFRESH, roto, NULL);
			} else {
 				// ...or "tick" every 10 seconds 
				angle[0] = 0x3000 + ((hour_unit * 0x2000) / 10);
				needrender = true;
			}
			if (angle[0] > 0x3FFF) {
				angle[0] -= 0x4000;
				// Swap panes
				digit_top[0] = (hour_ten + 1) % 3;	
				digit_bottom[0] = hour_ten;
			} else {
				digit_top[0] = hour_ten;
				if (hour_ten - 1 >= 0)
					digit_bottom[0] = (hour_ten - 1) % 3;
				else
					digit_bottom[0] = 2;
			}
			
			if (min_ten == 0) {
				rotob = true;
				rotomask |= 2;
				app_timer_register(REFRESH, roto, NULL);
			} else {
				angle[1] = 0x3000 + ((min_ten * 0x2000) / 6);
				needrender = true;
			}
			if (angle[1] > 0x3FFF) {
				angle[1] -= 0x4000;
				digit_top[1] = (hour_unit + 1) % 10;	
				digit_bottom[1] = hour_unit;
			} else {
				digit_top[1] = hour_unit;
				if (hour_unit - 1 >= 0)
					digit_bottom[1] = (hour_unit - 1) % 10;
				else
					digit_bottom[1] = 9;
			}
			
			if (min_unit == 0) {
				rotob = true;
				rotomask |= 4;
				app_timer_register(REFRESH, roto, NULL);
			} else {
				angle[2] = 0x3000 + ((min_unit * 0x2000) / 10);
				needrender = true;
			}
			if (angle[2] > 0x3FFF) {
				angle[2] -= 0x4000;
				digit_top[2] = (min_ten + 1) % 6;
				digit_bottom[2] = min_ten;
			} else {
				digit_top[2] = min_ten;
				if (min_ten - 1 >= 0)
					digit_bottom[2] = (min_ten - 1) % 6;
				else
					digit_bottom[2] = 5;
			}
			
			if (sec_ten == 0) {
				rotob = true;
				rotomask |= 8;
				app_timer_register(REFRESH, roto, NULL);
			} else {
				angle[3] = 0x3000 + ((sec_ten * 0x2000) / 6);
				needrender = true;
			}
			if (angle[3] > 0x3FFF) {
				angle[3] -= 0x4000;
				digit_top[3] = (min_unit + 1) % 10;	
				digit_bottom[3] = min_unit;
			} else {
				digit_top[3] = min_unit;
				if (min_unit - 1 >= 0)
					digit_bottom[3] = (min_unit - 1) % 10;
				else
					digit_bottom[3] = 9;
			}
			
			if (needrender == true) render();
		}
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
	uint32_t isin = 0;
	int32_t acc = 0;
	uint8_t y;
	
	graphics_context_set_fill_color(ctx,GColorBlack);
	graphics_context_set_stroke_color(ctx,GColorBlack);
	
	graphics_fill_rect(ctx, GRect(0, 0, 144, 168), 0, 0);
	
	#ifdef PBL_COLOR
		graphics_context_set_stroke_color(ctx,(GColor8){0b11010101});
	#else
		graphics_context_set_stroke_color(ctx,GColorWhite);
	#endif
	
	// Fill BG layer with some sort of procedural gradient

    for (y = 0; y < 84; y++) {
        acc += sin_lookup(isin);
        if (acc > 0x7FFF) {
            acc -= 0x10000;
        } else {
            graphics_draw_line(ctx,GPoint(0, y),GPoint(144, y));
            graphics_draw_line(ctx,GPoint(0, 168-y),GPoint(144, 168-y));
        }
        isin += (0x8000 / 100);
    }
}
 
void handle_init(void) {
	window = window_create();
	Layer *window_layer = window_get_root_layer(window);
	GRect bounds = layer_get_frame(window_layer);
	
	// Black fullscreen background layer
	bg_layer = layer_create(GRect(0, 0, 144, 168));		
	layer_set_update_proc(bg_layer, bg_fill);
	layer_add_child(window_layer, bg_layer);
	
	// Rendering layer, a bit wider than screen (4 pixels out left and right)
	digit_layer = bitmap_layer_create(GRect(-4, bounds.origin.y, 152, 168));
	#ifdef PBL_COLOR
		canvas_bmp = gbitmap_create_blank(GSize(160, 90),GBitmapFormat8Bit);
		bitmap_layer_set_compositing_mode(digit_layer, GCompOpSet);
	#else
		canvas_bmp = gbitmap_create_blank(GSize(160, 90));
	#endif
	bitmap_layer_set_bitmap(digit_layer, canvas_bmp);
	layer_add_child(window_layer, bitmap_layer_get_layer(digit_layer));
	
	// Load numbers "font", drawn with love in MSPaint
	#ifdef PBL_COLOR
		zero_bmp = gbitmap_create_with_resource(RESOURCE_ID_ZERO);
		one_bmp = gbitmap_create_with_resource(RESOURCE_ID_ONE);
		two_bmp = gbitmap_create_with_resource(RESOURCE_ID_TWO);
		three_bmp = gbitmap_create_with_resource(RESOURCE_ID_THREE);
		four_bmp = gbitmap_create_with_resource(RESOURCE_ID_FOUR);
		five_bmp = gbitmap_create_with_resource(RESOURCE_ID_FIVE);
		six_bmp = gbitmap_create_with_resource(RESOURCE_ID_SIX);
		seven_bmp = gbitmap_create_with_resource(RESOURCE_ID_SEVEN);
		eight_bmp = gbitmap_create_with_resource(RESOURCE_ID_EIGHT);
		nine_bmp = gbitmap_create_with_resource(RESOURCE_ID_NINE);
	#else
		numbers_bmp = gbitmap_create_with_resource(RESOURCE_ID_NUMBERS);
	#endif
	
	// Get pointers to raw data of the bitmap and canvas
	#ifdef PBL_COLOR
		zero_data = gbitmap_get_data(zero_bmp);
		one_data = gbitmap_get_data(one_bmp);
		two_data = gbitmap_get_data(two_bmp);
		three_data = gbitmap_get_data(three_bmp);
		four_data = gbitmap_get_data(four_bmp);
		five_data = gbitmap_get_data(five_bmp);
		six_data = gbitmap_get_data(six_bmp);
		seven_data = gbitmap_get_data(seven_bmp);
		eight_data = gbitmap_get_data(eight_bmp);
		nine_data = gbitmap_get_data(nine_bmp);
		canvas_data = gbitmap_get_data(canvas_bmp);
	#endif
	
	/*#ifdef PBL_COLOR
	text_layersg = text_layer_create((GRect){{32, 72}, {bounds.size.w-32-32, 20}});
	switch (gbitmap_get_format(numbers_bmp)) {
		case GBitmapFormat1Bit:
			text_layer_set_text(text_layersg, "1BIT");
			break;
		case GBitmapFormat8Bit:
			text_layer_set_text(text_layersg, "8BIT");
			break;
		case GBitmapFormat1BitPalette:
			text_layer_set_text(text_layersg, "1BP");
			break;
		case GBitmapFormat2BitPalette:
			text_layer_set_text(text_layersg, "2BP");
			break;
		case GBitmapFormat4BitPalette:
			text_layer_set_text(text_layersg, "4BP");
			break;
	}	
	text_layer_set_text_alignment(text_layersg, GTextAlignmentCenter);
	layer_add_child(window_layer, text_layer_get_layer(text_layersg));
#endif*/

	// Get notified of taps
	accel_tap_service_subscribe(tap_handler);
	
	// Get a refresh call every second (really refreshed each 10 seconds)
	time_t now = time(NULL);
	handle_timechanges(localtime(&now), SECOND_UNIT);
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
