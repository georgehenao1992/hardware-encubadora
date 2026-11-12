#include "../../src/config.h"
#include "../../src/lcd_wake.h"
#include <assert.h>
#include <stdio.h>

using namespace paulett;
int main() {
 LcdWake screen;
 assert(!screen.tick(false,0));
 assert(!screen.tick(true,10));
 assert(!screen.tick(false,20)); // rebote no enciende
 assert(!screen.tick(true,30));
 assert(!screen.tick(true,69));
 assert(screen.tick(true,70));
 assert(screen.tick(true,30069));
 assert(!screen.tick(true,30070)); // mantenido no prolonga la luz
 assert(!screen.tick(false,30080));
 assert(!screen.tick(false,30120));
 assert(!screen.tick(true,30130));
 assert(screen.tick(true,30170));
 assert(screen.tick(false,30200));
 assert(screen.tick(false,30240));
 assert(screen.tick(true,40000));
 assert(screen.tick(true,40040)); // nueva pulsacion reinicia 30 s
 assert(screen.tick(false,60170));
 assert(!screen.tick(false,70040));
 LcdWake rollover;
 assert(!rollover.tick(true,UINT32_MAX-100));
 assert(rollover.tick(true,UINT32_MAX-60));
 assert(rollover.tick(false,100));
 assert(rollover.tick(false,29938));
 assert(!rollover.tick(false,29939)); // desbordamiento millis
 Config c;
 assert(valid(c));
 assert(stageAt(0,c)==State::INCUBANDO);
 assert(stageAt(HATCH-1,c)==State::INCUBANDO);
 assert(stageAt(HATCH,c)==State::NACIMIENTO);
 assert(stageAt(END-1,c)==State::NACIMIENTO);
 assert(stageAt(END,c)==State::FINALIZADO);
 c.finishAt18=true; assert(stageAt(HATCH,c)==State::FINALIZADO); c.finishAt18=false;
 Cycle cycle; cycle.state=State::INCUBANDO; cycle.resumedAt=1000;
 assert(elapsed(cycle,1000+DAY)==DAY);
 cycle.elapsedSaved=elapsed(cycle,1000+DAY);cycle.state=State::PAUSADO;
 assert(elapsed(cycle,1000+10*DAY)==DAY);
 cycle.resumedAt=1000+10*DAY;cycle.state=State::INCUBANDO;
 assert(elapsed(cycle,1000+11*DAY)==2*DAY);
 assert(elapsed(cycle,cycle.resumedAt-1)==DAY); // RTC atrasado nunca subdesborda
 assert(futureSlot(10800,0,10800)==10800);
 assert(futureSlot(10800,10800,10800)==21600); // arranque exactamente en ranura: omitir
 assert(futureSlot(10800,6*10800+15,10800)==7*10800); // apagón largo: sin acumulación
 assert(futureSlot(0,HATCH,10800)>HATCH);
 for(uint32_t e=0;e<END;e+=107) {
  auto slot=futureSlot(10800,e,10800);assert(slot>e);assert(slot%10800==0);
 }
 auto d=regulate(c.incubation,37,45,{false,false,false});assert(d.heat&&!d.fan&&d.humid);
 d=regulate(c.incubation,37.6,55,d);assert(d.heat&&d.humid); // retiene histéresis
 d=regulate(c.incubation,37.8,60,d);assert(!d.heat&&!d.humid);
 d=regulate(c.incubation,38.1,55,d);assert(d.fan&&!d.heat);
 d=regulate(c.incubation,37.9,55,d);assert(d.fan&&!d.heat);
 d=regulate(c.incubation,37.8,55,d);assert(!d.fan&&!d.heat);
 for(int t=0;t<=600;t++)for(int old=0;old<8;old++) {
  d=regulate(c.incubation,t/10.f,55,{bool(old&1),bool(old&2),bool(old&4)});
  assert(!(d.heat&&d.fan));
 }
 c.motorMaxSeconds=121;assert(!valid(c));c=Config{};
 c.motorSeconds=31;assert(!valid(c));c=Config{};
 c.intervalSeconds=599;assert(!valid(c));c=Config{};
 c.incubation.heatOff=c.incubation.heatOn;assert(!valid(c));c=Config{};
 c.incubation.fanOff=37.0;assert(!valid(c));c=Config{};
 c.birth.humidOff=c.birth.humidOn;assert(!valid(c));c=Config{};
 c.overtemp=c.incubation.fanOn;assert(!valid(c));c=Config{};
 c.incubation.heatOn=NAN;assert(!valid(c));
 assert(plausible(37.8,60));assert(!plausible(NAN,60));assert(!plausible(37,0));assert(!plausible(61,50));assert(!plausible(37,101));
 puts("OK: etapas 1/18/21, pausas, recuperacion de ranuras, histeresis, prioridades y limites");
}
