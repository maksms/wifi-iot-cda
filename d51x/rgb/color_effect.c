/*
void set_color_effect__jump3(void *arg) {
	uint32_t delay = (uint32_t *)arg;
    static uint8_t mm = 0;
    uint8_t r,g,b;
	uint16_t duty = 255;
    while (1) {
        r=g=b=0;
        switch (mm) {
            case 0: r=duty; break;
            case 1: g=duty; break;
            case 2: b=duty; break;
        }

        ledctrl_set_color_duty(RED,    r);
        ledctrl_set_color_duty(GREEN,  g);
        ledctrl_set_color_duty(BLUE,   b);
        ledctrl_update();
        
        ++mm;
        if (mm == 3) mm = 0;

        vTaskDelay(delay / portTICK_RATE_MS);
    }
	vTaskDelete(NULL);
}
*/

/*

void set_color_effect__fade3(void *arg) {
	uint32_t delay = (uint32_t *)arg;
   	static uint8_t mm = 0;
   	static uint8_t r,g,b;
    r=g=b=0;
    uint8_t dir = 0;

	while( 1 ) {
       switch (mm) {
            case 0: 
				calc_color_duty_and_dir(&r, &dir);
                g=b=0;
                break;
            case 1: 
				calc_color_duty_and_dir(&g, &dir);
                r=b=0; 
                break;
            case 2: 
				calc_color_duty_and_dir(&b, &dir);
                g=r=0;
                break;
            default: 
			break;     
        }

        if ( !r && !g && !b ) {
            dir =0;
            ++mm;
            if (mm == 3) mm = 0;        
        }
        ledctrl_set_color_duty(RED,    r);
        ledctrl_set_color_duty(GREEN,  g);
        ledctrl_set_color_duty(BLUE,   b);
        ledctrl_update();
        vTaskDelay(delay / portTICK_RATE_MS);

	}

	vTaskDelete(NULL);
}

*/

/*

void set_color_effect__jump7(void *arg) {
	uint32_t delay = (uint32_t *)arg;

   	static uint8_t mm = 0;
    color_hsv_t hsv;
    color_rgb_t *rgb = (color_rgb_t *)malloc( sizeof(color_rgb_t));

    hsv.s = 255;
    hsv.v = 255;
    	
	while( 1 ) {
		
		switch (mm) {
			case 0: hsv.h = 0; break;	// red
			case 1: hsv.h = 30; break;	// orange
			case 2: hsv.h = 60; break;	// yellow
			case 3: hsv.h = 120; break;	// green
			case 4: hsv.h = 180; break;	// cyan
			case 5: hsv.h = 240; break;	// blue
			case 6: hsv.h = 300; break;	// purple
			default: break;
		}
		hsv_to_rgb(rgb, hsv);
        
        ledctrl_set_color_duty(RED,    rgb->r);
        ledctrl_set_color_duty(GREEN,  rgb->g);
        ledctrl_set_color_duty(BLUE,   rgb->b);
        ledctrl_update();

		++mm;
    	if	(mm == 7) mm = 0;        
        
        vTaskDelay(delay / portTICK_RATE_MS);
	}
	 free(rgb);
	vTaskDelete(NULL);
}
*/

/*
void set_color_effect__fade12(void *arg) {
	uint32_t delay = (uint32_t *)arg;

   static uint8_t mm = 0;
    uint8_t dir = 0;
    color_hsv_t hsv;
    color_rgb_t *rgb = (color_rgb_t *)malloc( sizeof(color_rgb_t));

    hsv.s = 255;
    	
	while( 1 ) {
		hsv.h = 30 * mm;
		calc_color_duty_and_dir(&hsv.v, &dir);
		hsv_to_rgb(rgb, hsv);

        if ( !rgb->r && !rgb->g && !rgb->b ) {
            dir =0;
            ++mm;
            if (mm == 12) mm = 0;        
        }
        ledctrl_set_color_duty(RED,    rgb->r);
        ledctrl_set_color_duty(GREEN,  rgb->g);
        ledctrl_set_color_duty(BLUE,   rgb->b);
        ledctrl_update();

        vTaskDelay(delay / portTICK_RATE_MS);
	}
	 free(rgb);
	vTaskDelete(NULL);
}
*/

/*

void set_color_effect__jump12(void *arg) {
	uint32_t delay = (uint32_t *)arg;

   	static uint8_t mm = 0;
    color_hsv_t hsv;
    color_rgb_t *rgb = (color_rgb_t *)malloc( sizeof(color_rgb_t));

    hsv.s = 255;
    hsv.v = 255;
    	
	while( 1 ) {
		hsv.h = 30 * mm;
		hsv_to_rgb(rgb, hsv);
        
        ledctrl_set_color_duty(RED,    rgb->r);
        ledctrl_set_color_duty(GREEN,  rgb->g);
        ledctrl_set_color_duty(BLUE,   rgb->b);
        ledctrl_update();

		++mm;
    	if	(mm == 12) mm = 0;        
        
        vTaskDelay(delay / portTICK_RATE_MS);
	}
	 free(rgb);
	vTaskDelete(NULL);
}

*/