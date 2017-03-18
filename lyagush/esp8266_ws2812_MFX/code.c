static os_timer_t esp_timer; static uint8_t data[255];
 char i,s,m, on,hsvhue,rgbr,rgbg,rgbb,napr, reset; 
 static uint8_t  fram[22] = {190,220,70,250,170,240,110,120,80,242,159,230,250,234,160,210,77,120,72,231,220,110}; char setu[80];
char rgbr1,rgbr1,rgbg1,rgbg2,rgbb2,rgbb2, vhu1,vhu2,vsat1,vsat2,vval1,vval2,vala1, sta=100, asa, vala2,lent;
char st, frpl,fir;	int tim1,tim2, tim1b;
char on1, m1, lent1, vhu1b, vsat1b, vsat2b, vval1b, vval2b;
void ICACHE_FLASH_ATTR hsv2rgb_rainbow(); void fire (char fre);	void plam(char s);	void timer (int time);  
#define K255 255
#define K171 171
#define K170 170
#define K85  85
#define DOLY(var)		((((var)/m)*s)/100)
#define BEGU(var)		((fir*var)/255)
#define BEGI(var)		if (var>128) { var-=128; (255-(((var*2)*fir)/255))} else  { var-=128; ((((var*2)*fir)/255))} 
#define BEGA(var)		if (var>128) { var-=128; (255-(((var*2)*fir)/255))} else  { var-=128; ((((var*2)*fir)/255))} 
#define M4		(m/3)
#define DOLU(var)		((((var)/(m/4))*st)/100)
#define NULA(var)		(char v=var/2; var=v*2;)
#define FORCE_REFERENCE(var)  asm volatile( "" : : "r" (var) )
# define REN(var,bar,dar) 	if (var!=bar) {var=bar; valdes[dar]=bar; }
#define CONF(var)		(sensors_param.cfgdes[var])
//#define FORCE_REFERENCE(var)  asm volatile( "" : : "r" (var) )
uint8_t  scale8dirt( uint8_t i, uint8_t scale)
{   uint8_t j= (((uint16_t)i) * ((uint16_t)(scale)+1)) >> 8;		return j;	}
uint8_t scale8video( uint8_t i, uint8_t scale)
{   uint8_t j= (((int)i * (int)scale) >> 8) + ((i&&scale)?1:0);	return	j;	}
uint8_t scale8( uint8_t i, uint8_t scale)
{   uint8_t j= (((uint16_t)i) * (1+(uint16_t)(scale))) >> 8;		return	j;	}

 
void webfunc(char *pbuf) {
os_sprintf(HTTPBUFF,"<br>frpl: %03d  s:%03d",frpl,s);  }

void ICACHE_FLASH_ATTR hsv2rgb_rainbow()
		{    	const uint8_t Y1 = 1;    const uint8_t Y2 = 0;	const uint8_t G2 = 0;     const uint8_t Gscale = 0;
				uint8_t hue = vhu1, deli;    uint8_t sat = vsat1, usat1, usat2;  uint8_t val = vval1, uval1, uval2, delval; int minkol, byo;
			
	if (on==2) { minkol=25500; hue+=DOLY( minkol);} else
	if (on==1) {//vhu1= vala1; vhu2 = vala2;
		if (vhu1>vhu2) {minkol = (vhu1-vhu2)*100; napr=1;} 
		else  {minkol = (vhu2-vhu1)*100; napr=0;} 
		if (minkol >12800)   {napr^=1; minkol = 25600-minkol;} else napr^=0;//deli = ((minkol/m)*s);	
		if (napr==1) {hue-=DOLY( minkol);} else  {hue+=DOLY( minkol);} } else
	if (on==3 | on==4) {if (vhu1>0) hue=vhu1; else hue=st; } 
	if (on==3 |on==2) { 
	uval1 = vval1 ? vval1 : sta; 	uval2 = vval2 ? vval2 : sta; 
	usat1 = vsat1 ? vsat1 : sta; 	usat2 = vsat2 ? vsat2 : sta; }
	
		uint8_t offset = hue & 0x1F; // 0..31
		uint8_t offset8 = offset;    offset8 <<= 3;
		uint8_t third = scale8( offset8, (256 / 3));  uint8_t r, g, b;
				
if( ! (hue & 0x80) ) {   if( ! (hue & 0x40) ) {   if( ! (hue & 0x20) ) {    			//				цветовой тон
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

if (on==1 | on==4)  { usat1= vsat1; uval1=vval1; usat2= vsat2; uval2=vval2;	}
sat = usat1; 	val = uval1;
if (usat1>usat2) {  sat-=DOLY( (usat1*100)-(usat2*100))  ; } 				//			насыщеность
	else { sat+=DOLY( (usat2*100)-(usat1 *100 )); }		

if( G2 ) g = g >> 1; if( Gscale ) g = scale8video( g, Gscale);
if (on==4) {	if (vsat1>128) { deli= vsat1-128; sat =255-(fir/((deli/15)+1));} else   
	if (vsat1==128) {if (fir<128) {sat = (fir*2); } else {deli-=127; sat = 255-(deli*2); }} else 
	if (vsat1<128) {deli= vsat1; byo=(fir*deli)/100;	if (byo>255) byo=255; sat = byo;} }
		  
if( sat != 255 ) { if( sat == 0)
{ r = 255; b = 255; g = 255; } else {
if( r ) r = scale8dirt( r, sat);	if( g ) g = scale8dirt( g, sat);	if( b ) b = scale8dirt( b, sat);
uint8_t desat = 255 - sat;	desat = scale8( desat, desat);	uint8_t brightness_floor = desat;
r += brightness_floor;	g += brightness_floor;	b += brightness_floor;
}	}

	if (on==4 & vval1 !=0 ) { val=BEGU(vval1);} else {					//				яркость
if (uval1>uval2) { val=(val-DOLY( (uval1*100)-(uval2*100)) );	}
else {  val=(val+DOLY ((uval2*100)-(uval1*100))   );  }}

if( val != 255 ) { 	val = scale8video( val, val);
if( val == 0 ) {r=0; g=0; b=0;} else 
{	r = r*val/256;	g = g*val/256;	b = b*val/256; 		}	}
rgbr = r;	rgbg = g;	rgbb = b;			}

//////////////////////////////////////////////////////////////////////////////

void plam(char sa)			//			камин
	{ char hit, sar, jok, sara,fre;  	hit  = setu[sa]; hit++;
		if(hit<1) setu[sa]=1; sar=sa+40; sara=setu[sar];
		if (sara>21) sara-=21;	setu[sar]=sara;	st=sa+sara;
		if (st>20) {st-=20; }  fre = fram[st];
		if (fre<100) jok=1; else if (fre<200) jok=2; else jok=3;
		if(hit<100) { fir= 30+(hit*jok);  if (fir>fre) {fir-=1; hit=101;}}  else
		if(hit>100) { fir= fre-((hit-100)*jok); if (fir<30) {hit=0; sara+=1;setu[sar]=sara;} }
	setu[sa]=hit; fire (fir); }
	
	void fire (char fre)
{ int  cio; cio=vhu1+10+(fre/5); if (cio>255) cio-=255;
vval1=fre/10;  	vsat1=255;  // vhu1=0;
	cio=(fre/8); if (cio>255) cio-=255;
	vhu2=cio;	vsat2=255-(fre/5); vval2=fre;	}
	
 ///////////////////////////////////////////////////////////////////////////////////
void ICACHE_FLASH_ATTR migal()
{char gok, h, ment=m*lent, mo; 
if (on>0) { if (on==1) plam(gok); s=0;
for (h=0;h<ment;h++){
	mo = lent-1 ? m : m-1;
if (s==m & on==1) 	{ s=0; gok++; plam(gok);} else
		if (s==mo & on!=1 ) 		{ s=0; if ((on==4) & ((st%3)==0)) {frpl++;  if (frpl==m) frpl=0;}}	
if (h%3==0 & on==4) {
if (s>(frpl) & s < (M4+(frpl)))		fir=(s-(frpl))*(255/M4); else fir=0; 	// M4	(m/4)
if (frpl > (m-M4) & s < (frpl-(m-M4))) 	fir=(m-(frpl-s))*(255/M4); }

hsv2rgb_rainbow();
if (h%3==0) {data[h] = rgbg;} else 
if (h%3==1) {data[h] = rgbr;}  else
if (h%3==2) {data[h] = rgbb;}  s++; 
}

if (on==3 | on==4 ) { st++;if (st>254) st=0;} else
if (on==2) { vhu1++;} 
 if (on==3 | on==2) { if (sta>254) asa = 0; if (sta<2) asa =1;
 if (asa==0) sta--; else sta++;}
ws2812_push(data, ment);} gok=0;}

//  ///   ////     ///////        //////////        ////////////////              //////////////////
void ICACHE_FLASH_ATTR startfunc(){// выполняется один раз при старте модуля.
char t;
os_timer_disarm(&esp_timer);
os_timer_setfn(&esp_timer, (os_timer_func_t *) migal, NULL);
os_timer_arm(&esp_timer, 12, 1);
for (t=0;t<80;t++){ setu[t]=0;}
}

void ICACHE_FLASH_ATTR
 timerfunc(uint32_t  timersrc) {
on= CONF(0);		REN(on1,on,0) 
m= CONF(1)*3;	if (m1!=m) {m1=m; valdes[1]=m/3; }
lent= CONF(2);	REN(lent1,lent,2) 
tim1= CONF(3); REN(tim1b,tim1,3) 
if(on!=2) {vhu1= CONF(4);	REN(vhu1b,vhu1,4) }
vsat1= CONF(5);	REN(vsat1b,vsat1,5) 
vval1= CONF(6);	REN(vval1b,vval1,6) 
vsat2= CONF(7);	REN(vsat2b,vsat2,7) 
vval2= CONF(8);  REN(vval2b,vval2,8) 

on= valdes[0];
m=(valdes[1])*3;
lent=valdes[2];
tim1= valdes[3];
if(on!=2) vhu1= valdes[4];
vsat1= valdes[5];
vval1= valdes[6];
vsat2=valdes[7];
vval2= valdes[8];

CONF(0)=on ;
CONF(1) = m/3;
CONF(2)= lent;
 CONF(3) = tim1;
if(on!=2) CONF(4)= vhu1;
CONF(5)= vsat1;
CONF(6) = vval1;
CONF(7) = vsat2;
CONF(8) = vval2;

if (reset==20) {ws2812_init(); reset =0;} reset++;
if (tim1!=tim2) {tim2=tim1;	timer (tim1); }
 }
 void timer (int time) 
 {os_timer_disarm(&esp_timer);
os_timer_setfn(&esp_timer, (os_timer_func_t *) migal, NULL);
os_timer_arm(&esp_timer, time, 1); }
