
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
//#include <sys/ioctl.h>
#include <sys/stat.h>
#include <string.h>
#include "sys/rtbio.h"
#include "gui/pw-X.h"
#include "app/common.h"
#include "app/system.h"
#include "app/UserFunc.h"
#include "app/RegFunc.h"
#include "app/ControlFunc.h"

#define MAX_SPELL_COUNT 	600
#define MAX_SPELL_INDEX 	6 * 2
#define T9_BACK_COLOR 	PW_RGB(255, 255, 255)
#define T9_FORE_COLOR 	PW_RGB(0, 0, 0)

int gSpellCount = 0;

PW_RECT gT9TextRect = {100, 80, 150, 30};
PW_RECT gT9LabelRect = {120, 200, 80, 30};
PW_RECT gT9InputRect = {10, 130, 300, 70};

PW_PIXELVAL	intable_label[80 * 30] = {0};

PW_GC_ID	t9_gc = 0;
PW_FONT_ID t9font = -1;

typedef struct __attribute__ ((packed)) tagKEY_SPELL {
	char key[MAX_SPELL_INDEX + 1];
	char spell[MAX_SPELL_INDEX + 1];
} KEY_SPELL, *PKEY_SPELL;

KEY_SPELL gKeySpellTable[MAX_SPELL_COUNT] = {{{0}, {0}}};

extern	char *gpFPWorkspace;

char * GetLineRecord(char * buf, char * id, char * str, int * plen)
{
	int flag = 0;
	unsigned short * pbuf = (unsigned short *)buf;
	unsigned char hByte;

	if (*pbuf == 0)
		return 0;

	while ((*pbuf != 0x0D) && (*pbuf != 0x0A) && (*pbuf != 0))
	{
		if (*pbuf == 0x09)/*Tab Code*/
		{
			flag = 1;
			*id = 0;
			pbuf ++;
			continue;
		}
		if (!flag)
		{
			*id = (*pbuf) & 0xFF;
			id ++;

			hByte = ((*pbuf)>>8) & 0xFF;
			if( hByte > 0 )
			{
				*id = hByte;
				id ++;
			}
			pbuf ++;
		}
		else
		{
			pbuf ++;
		}
	}

	while (*pbuf == 0x0D || *pbuf == 0x0A)
		pbuf ++;

	return (char *)pbuf;
}

void Spell2Key (KEY_SPELL *pKeySpell)
{
	memcpy( pKeySpell->key, pKeySpell->spell, MAX_SPELL_INDEX );
	/*
	int i = 0;
	
	for(i = 0 ; i <= MAX_SPELL_INDEX ; i ++)
	{
		switch (pKeySpell->spell[i])
		{
			case 'a':
			case 'b':
			case 'c':
				pKeySpell->key[i] = '2';
				break;
			case 'd':
			case 'e':
			case 'f':
				pKeySpell->key[i] = '3';
				break;
			case 'g':
			case 'h':
			case 'i':
				pKeySpell->key[i] = '4';
				break;
			case 'j':
			case 'k':
			case 'l':
				pKeySpell->key[i] = '5';
				break;
			case 'm':
			case 'n':
			case 'o':
				pKeySpell->key[i] = '6';
				break;
			case 'p':
			case 'q':
			case 'r':
			case 's':
				pKeySpell->key[i] = '7';
				break;
			case 't':
			case 'u':
			case 'v':
				pKeySpell->key[i] = '8';
				break;
			case 'w':
			case 'x':
			case 'y':
			case 'z':
				pKeySpell->key[i] = '9';
				break;
			default:
				pKeySpell->key[i] = 0;
				return;
		}
	}
	*/
	return;
}

int T9SelectProc(PW_WINDOW_ID parent_window, char *spell, unsigned short *T9, int select)
{
	PW_EVENT 	event;
	int ret = 0;
	int i = 0;
	int len = 0;
	int page_index = 0;
	unsigned short t9text[255] = {0};
	unsigned short t9tmp[2] = {0};
	char szTmp[30] = {0};
	GetStringFromDic_Traditional("Chinese2.t9", spell, t9text, 255);
	len = wslen(t9text);

	PW_SetGCForeground(t9_gc, T9_BACK_COLOR);
	PW_FillRect(parent_window, t9_gc, gT9InputRect.x, gT9InputRect.y + gT9InputRect.height / 2, gT9InputRect.width, gT9InputRect.height/2);
	PW_SetGCForeground(t9_gc, T9_FORE_COLOR);
	for(i = 0 ; i < ((len < 10) ? len : 10); i++)
	{
		if(t9text[i] < 0x4E00 || t9text[i] > 0x9FFF)
			goto ee;
		t9tmp[0] = t9text[i];
		t9tmp[1] = 0;
		PW_TextW(parent_window, t9_gc, gT9InputRect.x + i * 20, gT9InputRect.y + gT9InputRect.height/2 + 5, t9tmp, -1);
	}

	if(select != 1)
		goto ee;

	PW_SetGCForeground(t9_gc, T9_BACK_COLOR);
	PW_FillRect(parent_window, t9_gc, gT9InputRect.x, gT9InputRect.y, gT9InputRect.width, gT9InputRect.height/2);
	PW_SetGCForeground(t9_gc, T9_FORE_COLOR);
	for(i = 0 ; i < ((len < 10) ? len : 10); i++)
	{
		sprintf(szTmp, "%d", (i + 1) % 10);  
		PW_TextA(parent_window, t9_gc, gT9InputRect.x + 4 + i * 20, gT9InputRect.y + 5, szTmp, -1);
	}
	PW_MapGC();

	while(1)
	{
		PW_CheckNextEvent(&event);

		switch (event.type) 
		{
		case PW_EVENT_TYPE_MENU_TIMEOUT:
			ret = -1;
			goto ee;
			break;
		case PW_EVENT_TYPE_KEY_DOWN:
			Beep(BEEP_TYPE_KEY);
			switch (event.keystroke.ch)
			{
			    case 'A': //Enroll_Cancel
					ret = -1;
					goto ee;
					break;
				case 'D': // 'up arrow'
					if(page_index > 0)
					{
						page_index --;
					}
					break;
				case 'E': // 'down arrow'
					if(len > (page_index + 1) * 10)
						page_index ++;
					break;
				case '0': 
					if(len >= (page_index * 10 + event.keystroke.ch - '0' + 10))
					{
						*T9 = t9text[page_index * 10 + event.keystroke.ch - '0' - 1 + 10];
						ret = 1;
						goto ee;
					}
					break;
				case '1':
				case '2': 
				case '3': 
				case '4': 
				case '5': 
				case '6': 
				case '7': 
				case '8': 
				case '9':
					if(len >= (page_index * 10 + event.keystroke.ch - '0'))
					{
						*T9 = t9text[page_index * 10 + event.keystroke.ch - '0' - 1];
						ret = 1;
						goto ee;
					}
					break;
			}
			PW_SetGCForeground(t9_gc, T9_BACK_COLOR);
			PW_FillRect(parent_window, t9_gc, gT9InputRect.x, gT9InputRect.y, gT9InputRect.width, gT9InputRect.height);
			PW_SetGCForeground(t9_gc, T9_FORE_COLOR);
			for(i = 0 ; i < ((len - page_index * 10)< 10 ? (len - page_index * 10) : 10); i++)
			{
				t9tmp[0] = t9text[i + page_index * 10];
				t9tmp[1] = 0;
				sprintf(szTmp, "%d", (i + 1) % 10);  
				PW_TextA(parent_window, t9_gc, gT9InputRect.x + 4 + i * 20, gT9InputRect.y + 5, szTmp, -1);
				PW_TextW(parent_window, t9_gc, gT9InputRect.x + i * 20, gT9InputRect.y + gT9InputRect.height/2 + 5, t9tmp, -1);
			}
			PW_MapGC();
			break;
		case PW_EVENT_TYPE_NONE:
			break;
		}
	}
ee:
	if(ret == 1)
	{
		PW_SetGCForeground(t9_gc, T9_FORE_COLOR);
		PW_FillRect(parent_window, t9_gc, gT9InputRect.x, gT9InputRect.y, gT9InputRect.width, gT9InputRect.height);
		PW_MapGC();
	}

	return ret;
}

int T9InputProc(PW_WINDOW_ID parent_window, unsigned short *unicode, char initch)
{
	int ret = 0, i = 0, j = 0;
	PW_EVENT 	event;
	int key_index = -1;
	char key[MAX_SPELL_INDEX] = {0};
	unsigned short T9 = 0;
	int current_spell[30];
	int current_spell_index = 0;
	int current_spell_count = 0;
	int vDistance;
	signed short vKey = 0;
	signed short vHanzi = 0;
	unsigned short t9tmp[2] = {0};

	if(initch < '0' || initch > '9')
		goto ee;

	PW_SetGCForeground(t9_gc, T9_BACK_COLOR);
	PW_FillRect(parent_window, t9_gc, gT9InputRect.x, gT9InputRect.y, gT9InputRect.width, gT9InputRect.height);
	PW_MapGC();

	event.keystroke.ch = initch;
	goto edit;

	while(1)
	{
		PW_CheckNextEvent(&event);

		switch (event.type) 
		{
		case PW_EVENT_TYPE_MENU_TIMEOUT:
			ret = -1;
			goto ee;
			break;
		case PW_EVENT_TYPE_KEY_DOWN:
			Beep(BEEP_TYPE_KEY);
			switch (event.keystroke.ch)
			{
			    case 'A': //Cancel
				  ret = -1;
				  goto ee;
				  break;
				case 'C': // 'OK'
					if(T9SelectProc(parent_window, gKeySpellTable[current_spell[current_spell_index]].spell, &T9, 1) == 1)
					{
						*unicode = T9;
						ret = 1;
						goto ee;
					}
					break;
			    case 'B': //Backspace
					if(key_index < 0)
						break;

					key_index--;
					key_index--;
					key[key_index + 1] = 0;
					vKey = 0;

					memset(current_spell, 0, sizeof(current_spell));
					current_spell_count = 0;
					current_spell_index = 0;

					for(i = 0 ; i < gSpellCount ; i ++)
					{
						if(strlen(gKeySpellTable[i].key) > 0 && strcmp(key, gKeySpellTable[i].key) == 0 && current_spell_count <= 30)
						{
							current_spell[current_spell_count] = i;
							current_spell_count ++;
						}
					}
					break;
				case 'D': // 'Page up'
					if(current_spell_index <= 0)
						break;
					current_spell_index --;
					break;
				case 'E': // 'Page down'
					if(current_spell_index >= current_spell_count - 1)
						break;
					current_spell_index ++;
					break;
				case '0': 
				case '1': 
				case '2': 
				case '3': 
				case '4': 
				case '5': 
				case '6': 
				case '7': 
				case '8': 
				case '9':
edit:				
					switch (event.keystroke.ch)
					{
						case '1':
							if     ( vHanzi == 0x3105 )	vHanzi = 0x3106;
 							else if( vHanzi == 0x3106 )	vHanzi = 0x3107;
 							else if( vHanzi == 0x3107 )	vHanzi = 0x3108;
							else if( vHanzi == 0x3108 )	vHanzi = 0x3105;
							else						vHanzi = 0x3105;
							break;
						case '2':
							if	   ( vHanzi == 0x3109 )	vHanzi = 0x310A;
							else if( vHanzi == 0x310A )	vHanzi = 0x310B;
 							else if( vHanzi == 0x310B )	vHanzi = 0x310C;
 							else if( vHanzi == 0x310C )	vHanzi = 0x3109;
 							else						vHanzi = 0x3109;
							break;
						case '3':
							if	   ( vHanzi == 0x310D )	vHanzi = 0x310E;
							else if( vHanzi == 0x310E )	vHanzi = 0x310F;
 							else if( vHanzi == 0x310F )	vHanzi = 0x310D;
 							else						vHanzi = 0x310D;
							break;
						case '4':
							if	   ( vHanzi == 0x3110 )	vHanzi = 0x3111;
							else if( vHanzi == 0x3111 )	vHanzi = 0x3112;
 							else if( vHanzi == 0x3112 )	vHanzi = 0x3110;
 							else						vHanzi = 0x3110;
							break;
						case '5':
							if	   ( vHanzi == 0x3113 )	vHanzi = 0x3114;
							else if( vHanzi == 0x3114 )	vHanzi = 0x3115;
 							else if( vHanzi == 0x3115 )	vHanzi = 0x3116;
 							else if( vHanzi == 0x3116 )	vHanzi = 0x3113;
 							else						vHanzi = 0x3113;
							break;
						case '6':
							if	   ( vHanzi == 0x3117 )	vHanzi = 0x3118;
							else if( vHanzi == 0x3118 )	vHanzi = 0x3119;
 							else if( vHanzi == 0x3119 )	vHanzi = 0x3117;
 							else						vHanzi = 0x3117;
							break;
						case '7':
							if	   ( vHanzi == 0x311A )	vHanzi = 0x311B;
							else if( vHanzi == 0x311B )	vHanzi = 0x311C;
 							else if( vHanzi == 0x311C )	vHanzi = 0x311D;
 							else if( vHanzi == 0x311D )	vHanzi = 0x311A;
 							else						vHanzi = 0x311A;
							break;
						case '8':
							if	   ( vHanzi == 0x311E )	vHanzi = 0x311F;
							else if( vHanzi == 0x311F )	vHanzi = 0x3120;
 							else if( vHanzi == 0x3120 )	vHanzi = 0x3121;
 							else if( vHanzi == 0x3121 )	vHanzi = 0x311E;
 							else						vHanzi = 0x311E;
							break;
						case '9':
							if	   ( vHanzi == 0x3122 )	vHanzi = 0x3123;
							else if( vHanzi == 0x3123 )	vHanzi = 0x3124;
 							else if( vHanzi == 0x3124 )	vHanzi = 0x3125;
 							else if( vHanzi == 0x3125 )	vHanzi = 0x3126;
 							else if( vHanzi == 0x3126 )	vHanzi = 0x3122;
 							else						vHanzi = 0x3122;
							break;
						case '0':
							if	   ( vHanzi == 0x3127 )	vHanzi = 0x3128;
							else if( vHanzi == 0x3128 )	vHanzi = 0x3129;
 							else if( vHanzi == 0x3129 )	vHanzi = 0x3127;
 							else						vHanzi = 0x3127;
							break;
					}

					if(key_index >= MAX_SPELL_INDEX - 1)
						break;

					if( vKey == event.keystroke.ch ) key_index-=2;
					vKey = event.keystroke.ch;

					key_index++;
					key[key_index] = vHanzi & 0xFF;
					key_index++;
					key[key_index] = (vHanzi>>8) & 0xFF;
					key[key_index + 1] = 0;

					memset(current_spell, 0, sizeof(current_spell));
					current_spell_count = 0;
					current_spell_index = 0;

					for(i = 0 ; i < gSpellCount ; i ++)
					{
						if(strcmp(key, gKeySpellTable[i].key) == 0 && current_spell_count <= 30)
						{
							current_spell[current_spell_count] = i;
							current_spell_count ++;
						}
					}

					PW_MapGC();
					break;
			}

			PW_SetGCForeground(t9_gc, T9_BACK_COLOR);
			PW_FillRect(parent_window, t9_gc, gT9InputRect.x, gT9InputRect.y, gT9InputRect.width, gT9InputRect.height);
			
			for(i = 0 ; i < current_spell_count ; i++)
			{
				if(i == current_spell_index)
					PW_SetGCForeground(t9_gc, PW_RGB(220, 0, 0));
				else
					PW_SetGCForeground(t9_gc, T9_FORE_COLOR);
				for(j = 0 ; j < key_index + 1; j+=2)
				{
					t9tmp[0] = (gKeySpellTable[current_spell[i]].spell[j+1]<<8) + gKeySpellTable[current_spell[i]].spell[j];
					t9tmp[1] = 0;
					PW_TextW(parent_window, t9_gc, gT9InputRect.x + j * 8, gT9InputRect.y + 5, t9tmp, -1);
				}
			}
			T9SelectProc(parent_window, gKeySpellTable[current_spell[current_spell_index]].spell, &T9, 0);
			PW_MapGC();

			break;
		case PW_EVENT_TYPE_NONE:
			break;
		}
	}
ee:
	PW_SetGCForeground(t9_gc, T9_BACK_COLOR);
	PW_FillRect(parent_window, t9_gc, gT9InputRect.x, gT9InputRect.y, gT9InputRect.width, gT9InputRect.height);
	PW_MapGC();
	return ret;
}

int Key2Lower(char key, char *str)
{
	int ret = 0;

	if(!str)
		return -1;



	switch(key)
	{
		case '2':
			strcpy(str, "abc");
			break;
		case '3':
			strcpy(str, "def");
			break;
		case '4':
			strcpy(str, "ghi");
			break;
		case '5':
			strcpy(str, "jkl");
			break;
		case '6':
			strcpy(str, "mno");
			break;
		case '7':
			strcpy(str, "pqrs");
			break;
		case '8':
			strcpy(str, "tuv");
			break;
		case '9':
			strcpy(str, "wxyz");
			break;
		default:
			ret = -1;
	}


	return 1;
}

int Key2Upper(char key, char *str)
{
	int ret = 0;

	if(!str)
		return -1;

	switch(key)
	{
		case '2':
			strcpy(str, "ABC");
			break;
		case '3':
			strcpy(str, "DEF");
			break;
		case '4':
			strcpy(str, "GHI");
			break;
		case '5':
			strcpy(str, "JKL");
			break;
		case '6':
			strcpy(str, "MNO");
			break;
		case '7':
			strcpy(str, "PQRS");
			break;
		case '8':
			strcpy(str, "TUV");
			break;
		case '9':
			strcpy(str, "WXYZ");
			break;
		default:
			ret = -1;
	}
	return 1;
}

char Get_First_InputChar ( char aCurrentKey, unsigned int aCapsLock )
{
	char vChar = 0;

	switch (aCurrentKey)
	{
		case '2':
			vChar = 'a';
			break;
		case '3':
			vChar = 'd';
			break;
		case '4':
			vChar = 'g';
			break;
		case '5':
			vChar = 'j';
			break;
		case '6':
			vChar = 'm';
			break;
		case '7':
			vChar = 'p';
			break;
		case '8':
			vChar = 't';
			break;
		case '9':
			vChar = 'w';
			break;
		default:
			return 0;
	}
	if ( aCapsLock )
		vChar -= 32;
	
	return vChar;
}		

char Get_ReInputChar ( char aCurrentKey, char aPrivewChar, unsigned int aCapsLock )
{
	char vChar, vSampleChar;
	unsigned char vDiv;

	vDiv = 3;
	vChar = 0;
	if ( aCapsLock )
	{
		if ( aPrivewChar < 'A' ) return 0;
		if ( aPrivewChar > 'Z' ) return 0;	
		if ( (aPrivewChar >= 'P') && (aPrivewChar <= 'S') )
			vDiv = 4;
		if ( (aPrivewChar >= 'W') && (aPrivewChar <= 'Z') )
			vDiv = 4;
	}
	else
	{
		if ( aPrivewChar < 'a' ) return 0;
		if ( aPrivewChar > 'z' ) return 0;
		if ( (aPrivewChar >= 'p') && (aPrivewChar <= 's') )
			vDiv = 4;
		if ( (aPrivewChar >= 'w') && (aPrivewChar <= 'z') )
			vDiv = 4;
	}

	switch(aCurrentKey)
	{
		case '2':
			vSampleChar = 'a';
			if ( aCapsLock )
				vSampleChar -= 32;
			vChar = (aPrivewChar - vSampleChar + 1) % vDiv + vSampleChar;
			break;
		case '3':
			vSampleChar = 'd';
			if ( aCapsLock )
				vSampleChar -= 32;
			vChar = (aPrivewChar - vSampleChar + 1) % vDiv + vSampleChar;
			break;
		case '4':
			vSampleChar = 'g';
			if ( aCapsLock )
				vSampleChar -= 32;
			vChar = (aPrivewChar - vSampleChar + 1) % vDiv + vSampleChar;
			break;
		case '5':
			vSampleChar = 'j';
			if ( aCapsLock )
				vSampleChar -= 32;
			vChar = (aPrivewChar - vSampleChar + 1) % vDiv + vSampleChar;
			break;
		case '6':
			vSampleChar = 'm';
			if ( aCapsLock )
				vSampleChar -= 32;
			vChar = (aPrivewChar - vSampleChar + 1) % vDiv + vSampleChar;
			break;
		case '7':
			vSampleChar = 'p';
			if ( aCapsLock )
				vSampleChar -= 32;
			vChar = (aPrivewChar - vSampleChar + 1) % vDiv + vSampleChar;
			break;
		case '8':
			vSampleChar = 't';
			if ( aCapsLock )
				vSampleChar -= 32;
			vChar = (aPrivewChar - vSampleChar + 1) % vDiv + vSampleChar;
			break;
		case '9':
			vSampleChar = 'w';
			if ( aCapsLock )
				vSampleChar -= 32;
			vChar = (aPrivewChar - vSampleChar + 1) % vDiv + vSampleChar;
			break;
		default:
			return 0;
	}

	return vChar;
}

int Get_Eng_InputChar (char *aPrivewKey, char aCurrentKey, char *aPrivewChar, char *aCurrentChar, unsigned int aCapsLock)
{
	char *vPrivewChar = aPrivewChar;
	char *vCurrentChar = aCurrentChar;
	char *vPrivewKey = aPrivewKey;
	char vResult_Char = 0;

	if ( *vPrivewKey != aCurrentKey )
	{
		vResult_Char = Get_First_InputChar (aCurrentKey, aCapsLock);
		*vPrivewKey = aCurrentKey;
		if ( vResult_Char )
		{
			*vPrivewChar = vResult_Char;
			*vCurrentChar = vResult_Char;
			return 1;
		}
		else
			return -1;
	}
	else
	{
		vResult_Char = Get_ReInputChar (aCurrentKey,  *aPrivewChar, aCapsLock);
		if ( vResult_Char )
		{
			*vPrivewChar = vResult_Char;
			*vCurrentChar = vResult_Char;
		}
		else
			return -1;
	}

	return 0;
}


unsigned short Trans_KSC_Unicode (int aVal1, int aVal2)
{
  long vunicode;
  int vtabidx;

  vtabidx = ((int)aVal1 - 0x00a1) * 94 + (int)aVal2 - 0x00a1;
  if ( (vtabidx <= 0) || (vtabidx >= Get_KSC_5601MAX()) )
    return 0;
  vunicode = Get_KSC_Table_Value (vtabidx);

  return vunicode;
}


char Get_KorT9_Char (char aPriChar, char aCurrKey)
{
  char vChar = 0;

  switch (aCurrKey)
  {
    case '1':
      if ( aPriChar == 'r' )
        vChar = 'z';
      else
        vChar = 'r';
      break;
    case '2':
      if ( aPriChar == 'q' )
        vChar = 'v';
      else
        vChar = 'q';
      break;
    case '3':
      if ( aPriChar == 'k' )
        vChar = 'j';
      else
        vChar = 'k';
      break;
    case '4':
      if ( aPriChar == 's' )
        vChar = 'a';
      else
        vChar = 's';
      break;
    case '5':
      if ( aPriChar == 't' )
        vChar = 'w';
      else if ( aPriChar == 'w' )
        vChar = 'c';
      else
        vChar = 't';
      break;
    case '6':
      if (aPriChar == 'k')      // if previos char = た(k) for だ
        vChar = 'o';
      else if (aPriChar == 'j')   // if previos char = っ(j) for つ
        vChar = 'p';
      else if (aPriChar == 'i')   // if previos char = ち(i) for ぢ
        vChar = 'O';
      else if (aPriChar == 'u')   // if previos char = づ(u) for て
        vChar = 'P';
      else if (aPriChar == 'h')   // if previos char = で(h) for な
        vChar = 'l';
      else if (aPriChar == 'n')   // if previos char = ぬ(n) for は
        vChar = 'l';
      else if (aPriChar == 'm')   // if previos char = ぱ(n) for ひ
        vChar = 'l';
      else
      {
        vChar = 'l';
      }
      break;
    case '7':
      if ( aPriChar == 'e' )
        vChar = 'f';
      else if ( aPriChar == 'f' )
        vChar = 'x';
      else
        vChar = 'e';
      break;
    case '8':
      if ( aPriChar == 'd' )
        vChar = 'g';
      else
        vChar = 'd';
      break;
    case '9':
      if ( aPriChar == 'h' )
        vChar = 'n';
      else if ( aPriChar == 'n' )
        vChar = 'm';
      else
        vChar = 'h';
      break;

    case 'E': //KEY_DOWN :
      if (aPriChar != 'r' && aPriChar != 'R' &&
          aPriChar != 'e' && aPriChar != 'E' &&
          aPriChar != 'q' && aPriChar != 'Q' &&
          aPriChar != 't' && aPriChar != 'T' &&
          aPriChar != 'h' && aPriChar != 'y' &&
          aPriChar != 'n' && aPriChar != 'b' &&
          aPriChar != 'k' && aPriChar != 'i' &&
          aPriChar != 'j' && aPriChar != 'u' &&
          aPriChar != 'w' && aPriChar != 'W')
        break;
      if (aPriChar == 'r')
        vChar = 'R';
      else if (aPriChar == 'R')
        vChar = 'r';
      else if (aPriChar == 'e')
        vChar = 'E';
      else if (aPriChar == 'E')
        vChar = 'e';
      else if (aPriChar == 'q')
        vChar = 'Q';
      else if (aPriChar == 'Q')
        vChar = 'q';
      else if (aPriChar == 't')
        vChar = 'T';
      else if (aPriChar == 'T')
        vChar = 't';
      else if (aPriChar == 'w')
        vChar = 'W';
      else if (aPriChar == 'W')
        vChar = 'w';
      else if (aPriChar == 'h')
        vChar = 'y';
      else if (aPriChar == 'y')
        vChar = 'h';
      else if (aPriChar == 'n')
        vChar = 'b';
      else if (aPriChar == 'b')
        vChar = 'n';
      else if (aPriChar == 'k')
        vChar = 'i';
      else if (aPriChar == 'i')
        vChar = 'k';
      else if (aPriChar == 'j')
        vChar = 'u';
      else if (aPriChar == 'u')
        vChar = 'j';
      break;

    default:
      vChar = 0;
  }

  return vChar;

}


int Get_Korea_InputChar (char *aPrivewKey, char aCurrentKey, char *aPrivewChar, char *aCharList, unsigned short *aUnicode)
{
  char *vPrivewKey = aPrivewKey;
  char *vPrivewChar = aPrivewChar;
  char *vpCharList = aCharList;
  unsigned short *vUnicode = aUnicode;
  unsigned char vPart[8] = {0};
  unsigned char vTemp[8] = {0};
  char vChar;
  int vUni_Count = -1, vj;

  if ( (*vPrivewKey  != aCurrentKey) && (aCurrentKey != 'E') )
  {
    //*aPrivewChar = 0;
  }

  vChar = Get_KorT9_Char ( *aPrivewChar, aCurrentKey );

  if ( (*vPrivewKey  != aCurrentKey) && (aCurrentKey != 'E') && (vChar != 'o') && (vChar != 'p') && (vChar != 'O') && (vChar != 'P') )
  {
    vpCharList[strlen(vpCharList)] = vChar;
    vpCharList[strlen(vpCharList) + 1] = 0;

    vUni_Count = kr_automata (vChar, (char *)(vPart), (char *)(vTemp));
  }
  else
  {
    if ( strlen(vpCharList) != 0 )
    {
      vpCharList[strlen(vpCharList) - 1] = vChar;
      vpCharList[strlen(vpCharList)] = 0;
    }
    else
    {
      vpCharList[0] = vChar;
      vpCharList[1] = 0;
    }

    initKrState ();
        for (vj = 0; vj < strlen(vpCharList); vj++)
    {
      vUni_Count = kr_automata (vpCharList[vj], (char *)(vPart), (char *)(vTemp));
    }
  }

  *vPrivewChar = vChar;
  if ( aCurrentKey != 'E' )
    *vPrivewKey = aCurrentKey;


  if ( vUni_Count )
  {
    vUnicode[0] = Trans_KSC_Unicode (vPart[0], vPart[1]);
    vUnicode[1] = Trans_KSC_Unicode (vTemp[0], vTemp[1]);
  }
  else
  {
    vUnicode[0] = Trans_KSC_Unicode (vTemp[0], vTemp[1]);
  }

  return vUni_Count;
}

int CharInputProc(PW_WINDOW_ID parent_window, unsigned short *unicode, char initch, int nCase)
{
	int ret = 0, i = 0;
	PW_EVENT 	event;
	char key = 0;
	char str[5] = {0};
	char szTemp[5] = {0};

	if(initch < '2' || initch > '9')
		goto ee;

	PW_SetGCForeground(t9_gc, T9_BACK_COLOR);
	PW_FillRect(parent_window, t9_gc, gT9InputRect.x, gT9InputRect.y, gT9InputRect.width, gT9InputRect.height);


	key = initch;
	if(nCase == 0)
		Key2Lower(key, str);
	else
		Key2Upper(key, str);

	PW_SetGCForeground(t9_gc, T9_FORE_COLOR);
	for(i = 0 ; i < strlen(str) ; i ++)
	{
		PW_SetGCForeground(t9_gc, T9_FORE_COLOR);
		sprintf(szTemp, "%d:%c", i + 1, str[i]);
		PW_TextA(parent_window, t9_gc, gT9InputRect.x + i * 50, gT9InputRect.y + 5, szTemp, -1);
	}


	PW_MapGC();
	while(1)
	{
		PW_CheckNextEvent(&event);
		switch (event.type) 
		{
		case PW_EVENT_TYPE_MENU_TIMEOUT:
			ret = -1;
			goto ee;
			break;
		case PW_EVENT_TYPE_KEY_DOWN:
			Beep(BEEP_TYPE_KEY);
			switch (event.keystroke.ch)
			{
			    case 'A': //Enroll_Cancel
				  ret = -1;
				  goto ee;
				  break;
				case 'C': // 'OK'
					break;
			    case 'B': //Backspace
					break;
				case 'D': // 'Page up'
					break;
				case 'E': // 'Page down'
					break;
				case '0': 
					break;
				case '1': 
				case '2': 
				case '3': 
				case '4':
					i = event.keystroke.ch - '1';
					if(i == 3 && strlen(str) < 4)
						break;
					*unicode = str[i];
					ret = 1;
					goto ee;
					break;
				case '5': 
				case '6': 
				case '7': 
				case '8': 
				case '9':
					break;
			}
			break;
		case PW_EVENT_TYPE_NONE:
			break;
		}
	}
ee:
	PW_SetGCForeground(t9_gc, T9_BACK_COLOR);
	PW_FillRect(parent_window, t9_gc, gT9InputRect.x, gT9InputRect.y, gT9InputRect.width, gT9InputRect.height);
	PW_MapGC();
	return ret;
}
int CheckT9Enable()
{
	char  szString[MAX_ITEM_LENGTH] = {0};
	GetProfileString(PATH_USERSET_INI, "Language", "Chinese2", szString, MAX_ITEM_LENGTH);
	if(strcmp(szString, "Y") == 0)
		return 1;
	return 0;
}

int T9TextProc(PW_WINDOW_ID	parent_window, unsigned short *t9Text, int maxlen)
{ 
	PW_EVENT 	event;
	unsigned short T9 = 0;
	unsigned short text[64] = {0};
	unsigned short vtmptext[64] = {0};
	int text_index = -1;
	char strTmp1[50];
	char * szBuf = 0;
	char * pos = NULL;
	int fd = 0, nlen = 0, ret = 0, i = 0;
	int inputmode = 0;
	int t9enableflag;
	char vPrivewKey = 0;
	char vPrivewChar = 0; 
	char vResultChar = 0;
	unsigned int vWaitTime = 0;

	unsigned short modetext[4][5] = {
		//{0x62FC, 0},
		{0x3113, 0},
		{0x61, 0x62, 0x63, 0},
		{0x41, 0x42, 0x43, 0},
		{0x31, 0x32, 0x33, 0},
	};

	PW_ReadArea(parent_window, gT9LabelRect.x, gT9LabelRect.y, gT9LabelRect.width, gT9LabelRect.height, intable_label);

	int cursor = 0;

	if(GetProfileString(PATH_PRODUCT_INI, "Chinese2", "Font1", strTmp1, MAX_ITEM_LENGTH) == 0){
	  t9font = PW_CreateFont((PW_CHAR *)strTmp1, 16);
	}else{
	  GetProfileString(PATH_PRODUCT_INI, "DefaultHan", "Font1", strTmp1, MAX_ITEM_LENGTH);
	  t9font = PW_CreateFont((PW_CHAR *)strTmp1, 16);
	}

	t9enableflag = CheckT9Enable();
	if(t9enableflag)
	{
		inputmode = 0;
		fd = open("/mnt/Language/Chinese2.t9", O_RDONLY);
		if (fd < 0)
			goto ee;

		/*Get Filesize*/
		nlen = lseek(fd, 0, 2);
		if (nlen < 0)
			goto ee;

		lseek(fd, 0, 0);
		szBuf = (char*)(gpFPWorkspace + 640 * 480 + 1024 + 256 * 256);
		memset(szBuf, 0, nlen + 10);

		if (read(fd, szBuf, nlen) != nlen)
			goto ee;
		close(fd);

		pos = szBuf;
		gSpellCount = 0;

		memset(gKeySpellTable, 0x0, sizeof(gKeySpellTable));
		memset(strTmp1, 0x0, sizeof(strTmp1));
		while ((pos = GetLineRecord(pos, strTmp1, NULL, NULL)) > 0)
		{
			strncpy(gKeySpellTable[gSpellCount].spell, strTmp1, MAX_SPELL_INDEX);
			gSpellCount ++;
			memset(strTmp1, 0x0, sizeof(strTmp1));
		}

		for(i = 0 ; i < gSpellCount ; i++)
		{
			Spell2Key(&gKeySpellTable[i]);
		}
	}
	else
		inputmode = 1;

	t9_gc = PW_NewGC();
	PW_SetGCBackground(t9_gc, PW_RGB(255, 0, 0));
	PW_SetGCUseBackground(t9_gc, PW_FALSE);
	PW_SetGCForeground(t9_gc, T9_BACK_COLOR);
	PW_SetGCFont(t9_gc, t9font);

	PW_SetGCForeground(t9_gc, T9_BACK_COLOR);
	PW_FillRect(parent_window, t9_gc, gT9TextRect.x, gT9TextRect.y, gT9TextRect.width, gT9TextRect.height);
	PW_SetGCForeground(t9_gc, T9_BACK_COLOR);
	PW_FillRect(parent_window, t9_gc, gT9InputRect.x, gT9InputRect.y, gT9InputRect.width, gT9InputRect.height);

	nlen = wslen(t9Text);
	if(nlen > 0)
	{
		text_index += nlen;
		wscpy(text, t9Text);
		wscpy(vtmptext, t9Text);
		PW_SetGCForeground(t9_gc, PW_RGB(220, 0, 0));
		while(PW_GetTextWidth(vtmptext,t9font) > gT9TextRect.width){
		  wsncpy(&vtmptext[0],&vtmptext[1],wslen(vtmptext)-1);
		}
		PW_TextW(parent_window, t9_gc, gT9TextRect.x, gT9TextRect.y + 5, vtmptext, -1);
	}

	PW_SetGCForeground(t9_gc, T9_BACK_COLOR);
	PW_TextW(parent_window, t9_gc, gT9LabelRect.x + (gT9LabelRect.width - PW_GetTextWidth(modetext[inputmode], t9font)) / 2, gT9LabelRect.y + 5, modetext[inputmode], -1);	

	PW_MapGC();

	while(1)
	{
		PW_GetNextEventTimeout(&event, 500);
		switch (event.type) 
		{
		case PW_EVENT_TYPE_MENU_TIMEOUT:
			ret = -1;
			goto ee;
			break;
		case PW_EVENT_TYPE_KEY_DOWN:
			Beep(BEEP_TYPE_KEY);
			switch (event.keystroke.ch)
			{
				case 'F': //Power Off
				  if(t9enableflag)
					  inputmode = (inputmode + 1) % 4;
				  else
					  inputmode = inputmode % 3 + 1;
				  PW_Area(parent_window, t9_gc, gT9LabelRect.x, gT9LabelRect.y, gT9LabelRect.width, gT9LabelRect.height, intable_label, PWPF_PIXELVAL);
				  PW_SetGCForeground(t9_gc, T9_BACK_COLOR);
				  PW_TextW(parent_window, t9_gc, gT9LabelRect.x + (gT9LabelRect.width - PW_GetTextWidth(modetext[inputmode], t9font)) / 2, gT9LabelRect.y + 5, modetext[inputmode], -1);
				  PW_MapGC();

				  vWaitTime = 0;
				  vPrivewKey = 0; 
				  vPrivewChar = 0;
				  break;
				case 'A': //Enroll_Cancel
				  ret = -1;
				  goto ee;
				  break;
				case 'C': // 'OK'
				  ret = 1;
				  wscpy(t9Text, text);
				  goto ee;
				  break;
				case 'B': //Backspace
				  if(text_index < 0)
				    break;
				  text_index --;
				  if ( text_index >= (maxlen - 1) )
				  {
				    text_index = maxlen - 2;
				  }
				  text[text_index + 1] = 0;
				  vWaitTime = 0;
				  vPrivewKey = 0;
				  vPrivewChar = 0;
				  break;
				case 'D': // 'Page up'
					break;
				case 'E': // 'Page down'
					break;
				case '0':
				case '1': 
				case '2': 
				case '3': 
				case '4': 
				case '5': 
				case '6': 
				case '7': 
				case '8': 
				case '9':
					if ( (inputmode == 1) || (inputmode == 2) )
					{					
						if(text_index > maxlen -1)
							break;
					}
					else
					{
						if(text_index >= maxlen -1)
							break;
					}

					switch(inputmode)
					{
						case 0: // Chinese T9
							ret = T9InputProc(parent_window, &T9, event.keystroke.ch);
							break;
						case 1: // English lowercase
							ret = Get_Eng_InputChar (&vPrivewKey, event.keystroke.ch, &vPrivewChar, &vResultChar, 0);
							//ret = CharInputProc(parent_window, &T9, event.keystroke.ch, 0); //RePress 1,2,3 English Input Mode
							break;
						case 2: // English uppercase
							ret = Get_Eng_InputChar (&vPrivewKey, event.keystroke.ch, &vPrivewChar, &vResultChar, 1);
							//ret = CharInputProc(parent_window, &T9, event.keystroke.ch, 1); //RePress 1,2,3 English Input Mode
							break;
						case 3: // Number
							T9 = event.keystroke.ch;
							ret = 1;
							break;
					}

					if ( (inputmode == 1) || (inputmode == 2) )
					{
						if ( ret == 1 )
						{
							vWaitTime = 0;
							text_index ++;
							if(text_index <= maxlen -1)
							{
								text[text_index] = vResultChar;
								text[text_index + 1] = 0;
							}
						}
						else if ( ret == 0 )
						{
							if ( vWaitTime > 0 )
							{
								vWaitTime = 0;
								text_index ++;
							}
							if(text_index <= maxlen -1)
							{
								text[text_index] = vResultChar;
								text[text_index + 1] = 0;
							}
						}
						break;
					}
					else
					{					
						if (ret == 1)
						{
							text_index ++;
							text[text_index] = T9;
							text[text_index + 1] = 0;
						}
						break;					
					}
			}
			PW_SetGCForeground(t9_gc, T9_BACK_COLOR);
			PW_FillRect(parent_window, t9_gc, gT9TextRect.x, gT9TextRect.y, gT9TextRect.width, gT9TextRect.height);
			PW_SetGCForeground(t9_gc, PW_RGB(220, 0, 0));
			wscpy(vtmptext,text);
			while(PW_GetTextWidth(vtmptext,t9font) > gT9TextRect.width){
			  wsncpy(&vtmptext[0],&vtmptext[1],wslen(vtmptext)-1);
//			  printf("wslen(vtmptext) = %d\n",wslen(vtmptext));
			}
			PW_TextW(parent_window, t9_gc, gT9TextRect.x, gT9TextRect.y + 5, vtmptext, -1);
			PW_MapGC();
			break;
		case PW_EVENT_TYPE_NONE:
			vWaitTime = 1;
			cursor = 1 - cursor;
			if(cursor == 1)
			{
				PW_SetGCForeground(t9_gc, T9_FORE_COLOR);
				PW_FillRect(parent_window, t9_gc, gT9TextRect.x + PW_GetTextWidth(vtmptext, t9font) + 1, gT9TextRect.y + 5, 1, 16);
			}
			else
			{
				PW_SetGCForeground(t9_gc, T9_BACK_COLOR);
				PW_FillRect(parent_window, t9_gc, gT9TextRect.x + PW_GetTextWidth(vtmptext, t9font) + 1, gT9TextRect.y + 5, 1, 16);
			}
			PW_MapGC();
		}
	}
ee:
	PW_DestroyGC(t9_gc);
	return ret;
}

#define KOREAN_INPUT_MODE   0
#define ENG_BIG_INPUT_MODE    1
#define ENG_SMALL_INPUT_MODE  2
#define NUMBER_INPUT_MODE   3
#define CHINA_INPUT_MODE    4

int T9KorTextProc(PW_WINDOW_ID parent_window, unsigned short *t9Text, int maxlen)
{
  PW_EVENT  event;
  unsigned short T9[8];
  unsigned short text[64] = {0};
  int text_index = -1;
  char strTmp1[50];
  int nlen = 0, ret = 0;
  int inputmode = 0;
  char vPrivewKey = 0;
  char vPrivewChar = 0;
  unsigned int vWaitTime = 0;
  unsigned short vKorean_Temp_Buff[8] = {0};
  unsigned char vKor_TempBuff_Flag = 0;
  char vKorean_Char_List[50];
  int vSave_String_Len = -1;
  char vResultChar = 0;
  PW_WINDOW_ID  gT9_window;

  unsigned short modetext[5][5] = {
    {0xAC00, 0},
    {0x61, 0x62, 0x63, 0},
    {0x41, 0x42, 0x43, 0},
    {0x31, 0x32, 0x33, 0},
  };

  gT9_window = parent_window;

  PW_ReadArea(parent_window, gT9LabelRect.x, gT9LabelRect.y, gT9LabelRect.width, gT9LabelRect.height, intable_label);

  int cursor = 0;

  GetProfileString(PATH_PRODUCT_INI, "Korean", "Font1", strTmp1, MAX_ITEM_LENGTH);
  t9font = PW_CreateFont((PW_CHAR *)strTmp1, 16);

  initKrState ();
  inputmode = KOREAN_INPUT_MODE;

  t9_gc = PW_NewGC();
  PW_SetGCBackground(t9_gc, PW_RGB(255, 0, 0));
  PW_SetGCUseBackground(t9_gc, PW_FALSE);
  PW_SetGCForeground(t9_gc, T9_BACK_COLOR);
  PW_SetGCFont(t9_gc, t9font);

  PW_SetGCForeground(t9_gc, T9_BACK_COLOR);
  PW_FillRect(parent_window, t9_gc, gT9TextRect.x, gT9TextRect.y, gT9TextRect.width, gT9TextRect.height);
  PW_SetGCForeground(t9_gc, T9_BACK_COLOR);
  PW_FillRect(parent_window, t9_gc, gT9InputRect.x, gT9InputRect.y, gT9InputRect.width, gT9InputRect.height);

  nlen = wslen(t9Text);
  if(nlen > 0)
  {
    text_index += nlen;
    wscpy(text, t9Text);
    PW_SetGCForeground(t9_gc, PW_RGB(255, 0, 0));
    PW_TextW(parent_window, t9_gc, gT9TextRect.x, gT9TextRect.y + 5, text, -1);
  }

  PW_SetGCForeground(t9_gc, T9_BACK_COLOR);
  PW_TextW(parent_window, t9_gc, gT9LabelRect.x + (gT9LabelRect.width - PW_GetTextWidth(modetext[inputmode], t9font)) / 2, gT9LabelRect.y + 5, modetext[inputmode], -1);

  PW_MapGC();

  while(1)
  {
    PW_GetNextEventTimeout(&event, 500);
    switch (event.type)
    {
    case PW_EVENT_TYPE_MENU_TIMEOUT:
      ret = -1;
      goto ee;
      break;
    case PW_EVENT_TYPE_KEY_DOWN:
      Beep(BEEP_TYPE_KEY);
      switch (event.keystroke.ch)
      {
        case 'F': //Power Off
          inputmode = ((inputmode + 1) % 4);
          if ( inputmode == KOREAN_INPUT_MODE )
          {
            GetProfileString(PATH_PRODUCT_INI, "Korean", "Font1", strTmp1, MAX_ITEM_LENGTH);
            t9font = PW_CreateFont((PW_CHAR *)strTmp1, 16);
          }
          else
          {
            GetProfileString(PATH_PRODUCT_INI, "English", "Font1", strTmp1, MAX_ITEM_LENGTH);
            t9font = PW_CreateFont((PW_CHAR *)strTmp1, 16);
          }

          vKorean_Temp_Buff[0] = 0;
          vPrivewChar = 0;
          vPrivewKey = 0;
          initKrState ();
          memset (vKorean_Char_List, 0, sizeof(vKorean_Char_List));
          vWaitTime = 0;

          PW_Area(parent_window, t9_gc, gT9LabelRect.x, gT9LabelRect.y, gT9LabelRect.width, gT9LabelRect.height, intable_label, PWPF_PIXELVAL);
          PW_SetGCForeground(t9_gc, T9_BACK_COLOR);
          PW_TextW(parent_window, t9_gc, gT9LabelRect.x + (gT9LabelRect.width - PW_GetTextWidth(modetext[inputmode], t9font)) / 2, gT9LabelRect.y + 5, modetext[inputmode], -1);
          PW_MapGC();
          break;
          case 'A': //Enroll_Cancel
          ret = -1;
          goto ee;
          break;
        case 'C': // 'OK'
          ret = 1;
          if ( (vKorean_Temp_Buff[0] != 0) && (inputmode == KOREAN_INPUT_MODE) )
          {
            if(text_index < maxlen -1)
            {
              text_index ++;
              text[text_index] = vKorean_Temp_Buff[0];
              text[text_index + 1] = 0;
            }
          }
          wscpy(t9Text, text);
          goto ee;
          break;
          case 'B': //Backspace
          if(text_index < 0)
          {
            if ( inputmode == KOREAN_INPUT_MODE )
            {
              if ( vKorean_Temp_Buff[0] != 0 )
                vKorean_Temp_Buff[0] = 0;
              initKrState ();
              memset (vKorean_Char_List, 0, sizeof(vKorean_Char_List));
              vWaitTime = 0;
              vPrivewKey = 0;
              vPrivewChar = 0;
            }
            break;
          }

          if ( inputmode == KOREAN_INPUT_MODE )
          {
            if ( vKorean_Temp_Buff[0] == 0 )
              text_index --;
            else
              vKorean_Temp_Buff[0] = 0;

            vPrivewChar = 0;
            initKrState ();
            memset (vKorean_Char_List, 0, sizeof(vKorean_Char_List));
          }
          else
            text_index --;

          if ( text_index >= (maxlen - 1) )
          {
            text_index = maxlen - 2;
          }
          text[text_index + 1] = 0;
          memset (vKorean_Char_List, 0, sizeof(vKorean_Char_List));
          vWaitTime = 0;
          vPrivewKey = 0;
          vPrivewChar = 0;
          break;
        case 'D': // 'Page up'
          break;
        case 'E': // 'Page down'
          if ( (inputmode == KOREAN_INPUT_MODE) && (vKorean_Temp_Buff[0] != 0) )
            goto Goto_Korean_T9;
          break;
        case '0':
          if(text_index >= maxlen -1)
            break;
          text_index ++;
          if(inputmode == 3)
            text[text_index] = event.keystroke.ch;
          else
            text[text_index] = ' ';
          text[text_index + 1] = 0;
          break;
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
Goto_Korean_T9:
          if ( inputmode == KOREAN_INPUT_MODE )
          {
            if(text_index > maxlen -1)
              break;

            vWaitTime = 0;

            if ( (vPrivewKey != event.keystroke.ch) && (event.keystroke.ch != 'E') )
              vSave_String_Len = text_index;
            else
            {
              text_index = vSave_String_Len;
              text[text_index + 1] = 0;
              text[text_index + 2] = 0;
              text[text_index + 3] = 0;
            }

            memset (T9, 0, sizeof(T9));
            ret = Get_Korea_InputChar (&vPrivewKey, event.keystroke.ch, &vPrivewChar, vKorean_Char_List, T9);
            if (ret == 2)
            {
              if(text_index < maxlen -1)
              {
                text_index ++;
                text[text_index] = T9[0];
                text[text_index + 1] = 0;
              }

              vKorean_Temp_Buff[0] = T9[1];
              vKorean_Temp_Buff[1] = 0;
            }
            else
            {
              vKorean_Temp_Buff[0] = T9[0];
              vKorean_Temp_Buff[1] = 0;
            }

            break;
          }
          else
          {
            if ( (inputmode == ENG_BIG_INPUT_MODE) || (inputmode == ENG_SMALL_INPUT_MODE) )
            {
              if(text_index > maxlen -1)
                break;
            }
            else
            {
              if(text_index >= maxlen -1)
                break;
            }

            switch(inputmode)
            {
              case ENG_BIG_INPUT_MODE: // English lowercase
                ret = Get_Eng_InputChar (&vPrivewKey, event.keystroke.ch, &vPrivewChar, &vResultChar, 0);
                //ret = CharInputProc(parent_window, &T9, event.keystroke.ch, 0); //RePress 1,2,3 English Input Mode
                break;
              case ENG_SMALL_INPUT_MODE: // English uppercase
                ret = Get_Eng_InputChar (&vPrivewKey, event.keystroke.ch, &vPrivewChar, &vResultChar, 1);
                //ret = CharInputProc(parent_window, &T9, event.keystroke.ch, 1); //RePress 1,2,3 English Input Mode
                break;
              case NUMBER_INPUT_MODE: // Number
                T9[0] = event.keystroke.ch;
                ret = 1;
                break;
            }

            if ( (inputmode == ENG_BIG_INPUT_MODE) || (inputmode == ENG_SMALL_INPUT_MODE) )
            {
              if ( ret == 1 )
              {
                vWaitTime = 0;
                text_index ++;
                if(text_index <= maxlen -1)
                {
                  text[text_index] = vResultChar;
                  text[text_index + 1] = 0;
                }
              }
              else if ( ret == 0 )
              {
                if ( vWaitTime > 0 )
                {
                  vWaitTime = 0;
                  text_index ++;
                }
                if(text_index <= maxlen -1)
                {
                  text[text_index] = vResultChar;
                  text[text_index + 1] = 0;
                }
              }
              break;
            }
            else
            {
              if (ret == 1)
              {
                text_index ++;
                text[text_index] = T9[0];
                text[text_index + 1] = 0;
              }
              break;
            }
          }
      }
      PW_SetGCForeground(t9_gc, T9_BACK_COLOR);
      PW_FillRect(parent_window, t9_gc, gT9TextRect.x, gT9TextRect.y, gT9TextRect.width, gT9TextRect.height);
      PW_SetGCForeground(t9_gc, PW_RGB(255, 0, 0));
      PW_TextW(parent_window, t9_gc, gT9TextRect.x, gT9TextRect.y + 5, text, -1);
      if ( (inputmode == KOREAN_INPUT_MODE) && (vKorean_Temp_Buff[0] != 0) )
      {
        PW_TextW(parent_window, t9_gc, gT9TextRect.x + PW_GetTextWidth(text, t9font), gT9TextRect.y + 5, vKorean_Temp_Buff, -1);
        vKor_TempBuff_Flag = 0;
      }
      PW_MapGC();
      break;
    case PW_EVENT_TYPE_NONE:
      if ( inputmode == KOREAN_INPUT_MODE )
      {
        PW_SetGCForeground(t9_gc, T9_BACK_COLOR);
        PW_FillRect(parent_window, t9_gc, gT9TextRect.x + PW_GetTextWidth(text, t9font) + 1, gT9TextRect.y + 5, 1, 16);
        vWaitTime ++;
        if ( (inputmode == KOREAN_INPUT_MODE) && (vKorean_Temp_Buff[0] != 0) && (vWaitTime >= 6) )
        {
          vWaitTime = 0;
          if(text_index < maxlen -1)
          {
            text_index ++;
            text[text_index] = vKorean_Temp_Buff[0];
            text[text_index + 1] = 0;
          }
          memset (vKorean_Temp_Buff, 0, sizeof(vKorean_Temp_Buff));
          memset (vKorean_Char_List, 0, sizeof(vKorean_Char_List));

          vPrivewChar = 0;
          vPrivewKey = 0;
          initKrState ();

          PW_SetGCForeground(t9_gc, T9_FORE_COLOR);
          PW_FillRect(parent_window, t9_gc, gT9TextRect.x + PW_GetTextWidth(text, t9font) + 1, gT9TextRect.y + 5, 1, 16);
          PW_MapGC();
        }
      }
      else
      {
        vWaitTime = 1;
        cursor = 1 - cursor;
        if(cursor == 1)
        {
          PW_SetGCForeground(t9_gc, T9_FORE_COLOR);
          PW_FillRect(parent_window, t9_gc, gT9TextRect.x + PW_GetTextWidth(text, t9font) + 1, gT9TextRect.y + 5, 1, 16);
        }
        else
        {
          PW_SetGCForeground(t9_gc, T9_BACK_COLOR);
          PW_FillRect(parent_window, t9_gc, gT9TextRect.x + PW_GetTextWidth(text, t9font) + 1, gT9TextRect.y + 5, 1, 16);
        }
        PW_MapGC();
      }

    }
  }
ee:
  PW_DestroyGC(t9_gc);
  return ret;
}
