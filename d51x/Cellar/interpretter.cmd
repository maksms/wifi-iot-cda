if(valdes[0]==0&&valdes[1]>60)
gpioset(220,1)
printw(Откачиваем)
else
if(valdes[0]==0&&valdes[1]<50)
gpioset(220,0)
printw(Не откачиваем)
endif
endif
