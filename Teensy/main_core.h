#ifndef MAIN_CORE_H
#define MAIN_CORE_H

struct BallData{uint16_t angle = 255; uint16_t possession = 255; bool valid = false; float Vx; float Vy;} ballData;
struct TopMaixData {int16_t x = 0;int16_t y = 0;uint8_t status = 0;bool valid = false;bool ball_found = false;uint16_t ball_angle = 0xFFFF;uint8_t ball_dist = 0;};

extern BallData ballData;
extern TopMaixData topmaixData;

#define COM_Pin1 37
#define COM_Pin2 36

#endif
