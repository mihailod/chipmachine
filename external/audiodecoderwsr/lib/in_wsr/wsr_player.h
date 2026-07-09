#ifndef INC_WSR_PLAYER_H
#define INC_WSR_PLAYER_H

#ifndef WSR_PLAYER_API
#if defined _WIN32 || defined __CYGWIN__
#define WSR_PLAYER_API __declspec(dllimport)
#else
#define WSR_PLAYER_API __attribute__((visibility("default")))
#endif
#endif
#ifndef __stdcall
#define __stdcall
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
		int(*p_Load_WSR)(const void*, unsigned);
		int(*p_Get_FirstSong)(void);
		unsigned(*p_Set_Frequency)(unsigned int);
		void(*p_Set_ChannelMuting)(unsigned int);
		unsigned int(*p_Get_ChannelMuting)(void);
		void(*p_Reset_WSR)(unsigned);
		void(*p_Close_WSR)(void);
		int(*p_Update_WSR)(void*, unsigned, unsigned);
} WSRPlayerApi;

typedef WSRPlayerApi* (__stdcall * p_WSRPlayerSetUp)(void);

WSR_PLAYER_API WSRPlayerApi* WSRPlayerSetUp(void);

#ifdef __cplusplus
}
#endif

void Update_SampleData(void);
void ws_timer_reset(void);
void ws_timer_count(int Cycles);
void ws_timer_set(int no, int timer);
void ws_timer_update(void);
int ws_timer_min(int Cycles);

#endif
