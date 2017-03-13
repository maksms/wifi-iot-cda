static os_timer_t esp_timer; static uint8_t data[255];
 char i,s,m, on,hsvhue,tim1,tim2,hsvsat,hsvval,t1,tim2,t2,shif,rgbr,rgbg,rgbb,napr, reset; static uint8_t  fram[22] = {100,220,70,250,170,240,110,210,80,242,159,230,79,234,160,210,77,177,72,231,170,110}; char setu[80];
char rgbr1,rgbr2,rgbg1,rgbg2,rgbb1,rgbb2, hsvhue1,hsvhue2,hsvsat1,hsvsat2,hsvval1,hsvval2,vala,lent,t, stey, doja;
char st, frpl,fir;	
char on1, m1, lent1, tim1b, hsvhue1b, hsvsat1b, hsvsat2b, hsvval1b, hsvval2b;
void ICACHE_FLASH_ATTR hsv2rgb_rainbow(); void fire (char fre);	void plam(char s);	void doj(char dok);	void timer (char time);    //  void calc_rgb ();
#define K255 255
#define K171 171
#define K170 170
#define K85  85
#define DOLY(var)		((((var)/m)*s)/100)
#define BEGU(var)		((fir*var)/255)
#define BEGI(var)		if (var>128) { var-=128; (255-(((var*2)*fir)/255))} else  { var-=128; ((((var*2)*fir)/255))} 
#define BEGA(var)		if (var>128) { var-=128; (255-(((var*2)*fir)/255))} else  { var-=128; ((((var*2)*fir)/255))} 
#define M4		(m/4)
#define DOLU(var)		((((var)/(m/4))*st)/100)
#define NULA(var)		(char v=var/2; var=v*2;)
#define FORCE_REFERENCE(var)  asm volatile( "" : : "r" (var) )
# define REN(var,bar,dar) 	if (var!=bar) {var=bar; valdes[dar]=bar; }
//#define FORCE_REFERENCE(var)  asm volatile( "" : : "r" (var) )
uint8_t  scale8dirt( uint8_t i, uint8_t scale)
{   uint8_t j= (((uint16_t)i) * ((uint16_t)(scale)+1)) >> 8;		return j;	}
uint8_t scale8video( uint8_t i, uint8_t scale)
{   uint8_t j= (((int)i * (int)scale) >> 8) + ((i&&scale)?1:0);	return	j;	}
uint8_t scale8( uint8_t i, uint8_t scale)
{   uint8_t j= (((uint16_t)i) * (1+(uint16_t)(scale))) >> 8;		return	j;	}

void webfunc(char *pbuf) {}

void ICACHE_FLASH_ATTR hsv2rgb_rainbow()
		{    	const uint8_t Y1 = 1;    const uint8_t Y2 = 0;
				const uint8_t G2 = 0;     const uint8_t Gscale = 0;
				uint8_t hue = hsvhue1, deli;    uint8_t sat = hsvsat1, delsat;  uint8_t val = hsvval1, delval; int minkol, byo;
			
	if (on==2) { minkol=25500; hue+=DOLY( minkol);} else
	if (on==1) 	{if (hsvhue1>hsvhue2) {minkol = (hsvhue1-hsvhue2)*100; napr=1;} 
		else  {minkol = (hsvhue2-hsvhue1)*100; napr=0;} 
		if (minkol >12800)   {napr^=1; minkol = 25600-minkol;} else napr^=0;//deli = ((minkol/m)*s);	
		if (napr==1) {hue-=DOLY( minkol);} else  {hue+=DOLY( minkol);} vala=hue;} else
	if (on==3 | on==4) {if (hsvhue1<255) hue=hsvhue1; else hue=st; } 

		
				uint8_t offset = hue & 0x1F; // 0..31
				uint8_t offset8 = offset;    offset8 <<= 3;
				uint8_t third = scale8( offset8, (256 / 3)); // max = 85
				uint8_t r, g, b;
				
if( ! (hue & 0x80) ) {   if( ! (hue & 0x40) ) {   if( ! (hue & 0x20) ) {    			
	r = K255; 	g = third;         		b = 0;    FORCE_REFERENCE(b);	} else   					//		0		// R -> O
{	r = K255 - third;          	g = K85 + third ; 	b = 0;    FORCE_REFERENCE(b);	 }  	} else       				//		32 	// O -> Y	   
{	if( !  (hue & 0x20) ) { uint8_t twothirds = scale8( offset8, ((256 * 2) / 3)); 
	r = K171 - twothirds;   g = K170 + third;   b = 0;     FORCE_REFERENCE(b);	 } else    						//		64  	// Y -> G		
{ 	r = 0;       FORCE_REFERENCE(r);	g = K255 - third;    b = third;  }  }    } else  				//		96	// G -> A	
{	if( ! (hue & 0x40) ) {if( ! ( hue & 0x20) ) {	uint8_t twothirds = scale8( offset8, ((256 * 2) / 3)); 		
	r = 0; 	FORCE_REFERENCE(r);		g = K171 - twothirds; 	b = K85  + twothirds;   } else //		128 	// A -> B
{	r = third;   			g = 0;   FORCE_REFERENCE(g);	b = K255 - third;    }  } else			//		160	// B -> P
{  if( !  (hue & 0x20)  ) 		
{	r = K85 + third;   	g = 0; FORCE_REFERENCE(g);		b = K171 - third;   } else 				// 	192 	// P -- K 
{	r = K170 + third;	g = 0; FORCE_REFERENCE(g);	b = K85 - third;   }   } } 				//		224	// K -> R				

if (hsvsat1>hsvsat2) {  sat-=DOLY( (hsvsat1*100)-(hsvsat2*100))  ; } 
	else { sat+=DOLY( (hsvsat2*100)-(hsvsat1 *100 )); }		

if( G2 ) g = g >> 1; if( Gscale ) g = scale8video( g, Gscale);
	if (on==4) {
		if (hsvsat1>128) { deli= hsvsat1-128; sat =255-(fir/((deli/15)+1));} else   
		if (hsvsat1==128) {if (fir<128) {sat = (fir*2); } else {deli-=127; sat = 255-(deli*2); }} else 
		  if (hsvsat1<128) {deli= hsvsat1; byo=(fir*deli)/100;	if (byo>255) byo=255; sat = byo;} }
		  
if( sat != 255 ) { if( sat == 0)
{ r = 255; b = 255; g = 255; } else {

if( r ) r = scale8dirt( r, sat);
if( g ) g = scale8dirt( g, sat);
if( b ) b = scale8dirt( b, sat);

uint8_t desat = 255 - sat;
desat = scale8( desat, desat);
uint8_t brightness_floor = desat;
r += brightness_floor;
g += brightness_floor;
b += brightness_floor;
}	}
	if (on==4 & hsvval1 !=0 ) { val=BEGU(hsvval1);} else {	// if (val<250) val/=2;
if (hsvval1>hsvval2) { val=(val-DOLY( (hsvval1*100)-(hsvval2*100)) );	}//(((hsvval1-hsvval2)/m)*s); } 
else {  val=(val+DOLY ((hsvval2*100)-(hsvval1*100))   );  }}

if( val != 255 ) { 	val = scale8video( val, val);
if( val == 0 ) {r=0; g=0; b=0;} else {
	
/*if( r ) r = scale8dirt( r, val);
if( g ) g = scale8dirt( g, val);
if( b ) b = scale8dirt( b, val); 		*/
r = r*val/256;
g = g*val/256;
b = b*val/256; 		}	}
rgbr = r;
rgbg = g;
rgbb = b;			}

void plam(char sa)
	{ char hit, sar, jok, sara,fre;  	hit  = setu[sa]; hit++;
		if(hit<1) setu[sa]=1; sar=sa+40; sara=setu[sar];
		if (sara>21) sara-=21;	setu[sar]=sara;	st=sa+sara;
		if (st>20) {st-=20; }  fre = fram[st];
if (fre<100) jok=1; else if (fre<200) jok=2; else jok=3;
if(hit<100) { fir= 30+(hit*jok);  if (fir>fre) {fir-=1; hit=101;}}  else
if(hit>100) { fir= fre-((hit-100)*jok); if (fir<30) {hit=0; sara+=1;setu[sar]=sara;} }
	 setu[sa]=hit; fire (fir); }// fir=200;
	 
void doj(char dok){hsvhue1=dok; hsvhue2=95+dok;  }
///////////////////////////////////////////////////////////////////////////////////
void ICACHE_FLASH_ATTR migal()
{char gok, h, ment=m*lent; 
if (on>0) { 
if (on==1) plam(gok);
s=0;
for (h=0;h<ment;h++){
s++; if (s==m ) {s=0; if (on==1) { gok++; plam(gok);}}	
if (h%3==0 & on==4) {

		if (s>(frpl) & s < (M4+(frpl)))		fir=(s-(frpl))*(255/M4); else fir=0; 
if (frpl > (m-M4) & s < (frpl-(m-M4))) 	fir=(m-(frpl-s))*(255/M4); 
}
 //if ((on==2) & (s==m/2)) {hsvhue1=0; hsvhue2=127;  } 
hsv2rgb_rainbow();
if (h%3==0) {data[h] = rgbg;} else 
if (h%3==1) {data[h] = rgbr;}  else
if (h%3==2) {data[h] = rgbb;}  

}
if (on==3 | on==4 ) { st++;if (st>254) st=0;} else
if (on==2) { hsvhue1++;} 
if (on==4) {frpl++; if (frpl>(m)) frpl=0;}

ws2812_push(data, ment);} gok=0;}

void fire (char fre)
{ int cio;
hsvval1=fre/10;  	hsvsat1=255;  // hsvhue1=0;
	cio=(fre/8); if (cio>255) cio-=255;
	hsvhue2=cio;	hsvsat2=255-(fre/5); hsvval2=fre;
}
 
void ICACHE_FLASH_ATTR startfunc(){// выполняется один раз при старте модуля.
char t;
os_timer_disarm(&esp_timer);
os_timer_setfn(&esp_timer, (os_timer_func_t *) migal, NULL);
os_timer_arm(&esp_timer, 12, 1);
for (t=0;t<80;t++){ setu[t]=0;}
}


void ICACHE_FLASH_ATTR
 timerfunc(uint32_t  timersrc) {
on= (sensors_param.cfgdes[0]);		REN(on1,on,0) 
m= (sensors_param.cfgdes[1])*3;	if (m1!=m) {m1=m; valdes[1]=m/3; }
lent= (sensors_param.cfgdes[2]);	REN(lent1,lent,2) 
tim1= (sensors_param.cfgdes[3]); REN(tim1b,tim1,3) 
if(on!=2) {hsvhue1= (sensors_param.cfgdes[4]);	REN(hsvhue1b,hsvhue1,4) }
hsvsat1= (sensors_param.cfgdes[5]);	REN(hsvsat1b,hsvsat1,5) 
hsvval1= (sensors_param.cfgdes[6]);	REN(hsvval1b,hsvval1,6) 
hsvsat2= (sensors_param.cfgdes[7]);	REN(hsvsat2b,hsvsat2,7) 
hsvval2= (sensors_param.cfgdes[8]);  REN(hsvval2b,hsvval2,8) 

on= valdes[0];
m=(valdes[1])*3;
lent=valdes[2];
tim1= valdes[3];
if(on!=2) hsvhue1= valdes[4];
hsvsat1= valdes[5];
hsvval1= valdes[6];
hsvsat2=valdes[7];
hsvval2= valdes[8];

(sensors_param.cfgdes[0])=on ;
(sensors_param.cfgdes[1]) = m/3;
(sensors_param.cfgdes[2])= lent;
 (sensors_param.cfgdes[3]) = tim1;
if(on!=2) (sensors_param.cfgdes[4])= hsvhue1;
(sensors_param.cfgdes[5])= hsvsat1;
(sensors_param.cfgdes[6]) = hsvval1;
(sensors_param.cfgdes[7]) = hsvsat2;
(sensors_param.cfgdes[8]) = hsvval2;

if (reset==20) {ws2812_init(); reset =0;} reset++;
if (tim1!=tim2) {tim2=tim1;	timer (tim1); }
 }
 void timer (char time) 
 {os_timer_disarm(&esp_timer);
os_timer_setfn(&esp_timer, (os_timer_func_t *) migal, NULL);
os_timer_arm(&esp_timer, time, 1); }
