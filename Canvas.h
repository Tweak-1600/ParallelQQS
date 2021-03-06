#pragma once
#include "Global.h"
#include "FFMpeg.h"
#include "RenderOptions.h"
#include <sstream>
#include <math.h>
class Canvas
{
private:
	int _Width;
	int _Height;
	int _FPS;
	int _CRF;
	int _KeyHeight;
	unsigned int _LineColor;
	size_t _FrameSize;
	FFMpeg _Pipe;
	int _KeyX[128];
	int _NoteX[128];
	int _KeyWidth[128];
	int _NoteWidth[128];
	unsigned int* _Frame;
	unsigned int** _FrameIndexes;

	unsigned int* _EmptyFrame;
	INLINE_VAR static unsigned int _InitKeyColors[128];

	INLINE_CONST static struct __KeyColorsInitializer__
	{
		__KeyColorsInitializer__()
		{
			int i;
			for (i = 0; i != 128; ++i)
			{
				switch (i % 12)
				{
				case(1):
				case(3):
				case(6):
				case(8):
				case(10):
					_InitKeyColors[i] = 0xFF000000;
					break;
				default:
					_InitKeyColors[i] = 0xFFFFFFFF;
				}
			}
		}
	} __KCInitializer__;
public:
	unsigned int KeyColors[128];
	bool KeyPressed[128];
	Canvas() = default;
	void Initialize(const RenderOptions& _Opt)
	{
		_Width = _Opt.Width;
		_Height = _Opt.Height;
		_FPS = _Opt.FPS;
		_CRF = _Opt.CRF;
		_LineColor = _Opt.SeparateBarColor;
		_KeyHeight = _Opt.KeyboardHeight;

		std::stringstream ffargs;
		ffargs << "-y -hide_banner -f rawvideo -pix_fmt rgba -s " << _Width << "x" << _Height << 
			" -r " << _FPS << " -i - -pix_fmt yuv420p -filter_threads 32 -preset medium -crf " << _CRF << " \"" << _Opt.OutputPath.c_str() <<
			"\"";
		std::string ffres = ffargs.str();
		_Pipe.Initialize(ffres.c_str(), _Width, _Height);

		_FrameSize = (size_t)_Width * (size_t)_Height;
		_Frame = new unsigned int[_FrameSize + _Width];
		memset(_Frame, 0, _FrameSize * 4);

		_FrameIndexes = new unsigned int* [_Height];

		for (int i = 0; i < _Height; ++i)
		{
			_FrameIndexes[i] = _Frame + ((size_t)_Height - 1 - i) * _Width;
		}
		for (int i = 0; i != 128; ++i)
		{
			_KeyX[i] = (i / 12 * 126 + GenKeyX[i % 12]) * _Width / 1350;
		}
		for (int i = 0; i != 127; ++i)
		{
			int val;
			switch (i % 12)
			{
			case 1:
			case 3:
			case 6:
			case 8:
			case 10:
				val = _Width * 9 / 1350;
				break;
			case 4:
			case 11:
				val = _KeyX[i + 1] - _KeyX[i];
				break;
			default:
				val = _KeyX[i + 2] - _KeyX[i];
				break;
			}
			if (val < 0)
			{
				cerr << "!" << endl;
			}
			_KeyWidth[i] = val;
		}
		_KeyWidth[127] = _Width - _KeyX[127];

		if (_Opt.FitNotes) // failed
		{
			// ????????????????
			for (int i = 0; i != 127; ++i)
			{
				switch (i % 12)
				{
				case 1:
				case 3:
				case 6:
				case 8:
				case 10:
					_NoteWidth[i] = _KeyWidth[i];
					break;
				case 0:
				case 5:
					_NoteWidth[i] = _KeyX[i + 1] - _KeyX[i];
					break;
				default:
					_NoteWidth[i] = _KeyX[i + 1] - _KeyX[i - 1] - _KeyWidth[i - 1];
					break;
				}
			}
		}
		else
		{
			memcpy(_NoteWidth, _KeyWidth, 512);
		}
		if (_Opt.FitNotes)
		{
			for (int i = 0; i != 127; ++i)
			{
				switch (i % 12)
				{
				case 0:
				case 5:
				case 1:
				case 3:
				case 6:
				case 8:
				case 10:
					_NoteX[i] = _KeyX[i];
					break;
				default:
					_NoteX[i] = _KeyX[i - 1] + _NoteWidth[i - 1];
					break;
				}
			}
			_NoteX[127] = _KeyX[126] + _KeyWidth[126];
			_NoteWidth[127] = _Width - _NoteX[127];
		}
		else
		{
			memcpy(_NoteX, _KeyX, 512);
		}

		_EmptyFrame = new unsigned int[_FrameSize];
		for (int i = 0; i != _FrameSize; ++i)
		{
			_EmptyFrame[i] = 0xFF000000;
		}
		memcpy(_Frame, _EmptyFrame, _FrameSize * 4);
		/*for (int j,i=75;i!=128;++i)
		{
		j=DrawMap[i];
		_KeyX[j]-= _KeyX[1]-(_KeyWidth[0] / 2) ;
		_KeyWidth[j]+=_KeyX[1]+_KeyWidth[1]-((_KeyX[3]-_KeyX[1]+(_KeyWidth[1])) / 2);
		}*/
	}
     
	void Destroy()
	{
		if (_Frame)
		{
			delete[] _Frame;
		}
		if (_FrameIndexes)
		{
			delete[] _FrameIndexes;
		}
		if (_EmptyFrame)
		{
			delete[] _EmptyFrame;
		}
		_Pipe.Close();
		_Frame = nullptr;
		_FrameIndexes = nullptr;
		_EmptyFrame = nullptr;
	}

	~Canvas()
	{
		if (_Frame)
		{
			delete[] _Frame;
		}
		if (_FrameIndexes)
		{
			delete[] _FrameIndexes;
		}
		if (_EmptyFrame)
		{
			delete[] _EmptyFrame;
		}
		_Pipe.Close();
		_Frame = nullptr;
		_FrameIndexes = nullptr;
		_EmptyFrame = nullptr;
	}

	void Clear()
	{
		memcpy(KeyColors, _InitKeyColors, 512);
		memcpy(_Frame, _EmptyFrame, _FrameSize * 4);
		memset(KeyPressed, 0, 128);
/*#ifdef _DEBUG_
		FillRectangle(0, 0, _Width, _Height, 0xFFc0c0c0);
#else
        FillRectangle(0, 0, _Width, _Height, 0xFF303030);
#endif
*/
	}

	void WriteFrame()
	{
		_Pipe.WriteFrame(_Frame);
	}

	void DrawKeys()
	{
		int i, j;
		int t=(_Width/640)-2;
		const int bh = _KeyHeight * 64 / 100;
		const int bgr = _KeyHeight / 18;
		const int diff = _KeyHeight - bh;
		Draw3Drect2(0, _KeyHeight - 2, _Width, _KeyHeight / 16, _LineColor,1.5);
		for (i = 0; i != 75; ++i)
		{
			j = DrawMap[i];
			DrawGrayRect(_KeyX[j], 0, _KeyWidth[j], _KeyHeight,KeyColors[j],1.05);
			DrawRectangle(_KeyX[j], 0, _KeyWidth[j] + 1, _KeyHeight, 0xFF000000);
			DrawGrayRect(_KeyX[j]+1, 0, _KeyWidth[j]-1, bgr/3+1,KeyColors[j],2);
			
			 // ?????????????????????????????????????????
			if (!KeyPressed[j])
			{
				DrawRectangle(_KeyX[j], 0, _KeyWidth[j]+1, bgr, 0xFF000000);
				Draw3Drect2(_KeyX[j]+1, 1, _KeyWidth[j]-1, bgr-2, 0xFFAFAFAF,1.3); // ???????????????????????????????????????????? (????????????????????????)
				//FillRectangle(_KeyX[60]+(_KeyWidth[60] / 4), bgr+(_KeyWidth[60]/4),(_KeyX[61] - (_KeyWidth[60]/4))-_KeyX[60]+(_KeyWidth[60] / 4), (_KeyX[61] - (_KeyWidth[60]/4))-_KeyX[60]+(_KeyWidth[60] / 4), 0xFFa0a0a0); 
			}
			
		}
		/*if (!KeyPressed[60]){
		FillRectangle(_KeyX[60]+(_KeyWidth[60] / 3.5), bgr+(_KeyWidth[60]/3.6),(_KeyWidth[60] / 1.9), (_KeyWidth[60]/1.9), 0xFFAAAAAA); 
		}
		else
		{
		DrawGrayRect(_KeyX[60]+(_KeyWidth[60] / 3.5),_KeyWidth[60]/3.5,(_KeyWidth[60] /1.9),(_KeyWidth[60] / 1.9), KeyColors[60],2.0); 
		}*/
		
		for (;i != 128; ++i)
		{
			j = DrawMap[i];
			Draw3Drect(_KeyX[j]-t, diff, _KeyWidth[j]+2*t,bh, KeyColors[j],1.1);
			DrawNoteRectangle(_KeyX[j]-t, diff, _KeyWidth[j]+1+2*t, bh, KeyColors[j],t); // ????????????????????????????????????????????
			DrawRectangle(_KeyX[j]-t, diff, _KeyWidth[j]+1+2*t, bh, 0xFF000000);
		}
		Draw3Drect2(0, _KeyHeight - 2, _Width, _KeyHeight / 18, _LineColor,1.5);
		for (i=75;i!= 128;++i)
		{
		    j=DrawMap[i];
		    if (!KeyPressed[j])
			{
				Draw3Drect(_KeyX[j]+1,diff, _KeyWidth[j]-1,bh+(_KeyHeight / 38), 0xFF151515,1.1);
				//DSJX(_KeyX[j]-t,_KeyX[j]+1,_KeyHeight,_KeyHeight / 38,0xFF000000);
			}
		}
		
		
		
	//	DSJX(50,150,50,40,0xFF000000);
	//	DSJX(300,200,0,100,0xFFAAAAAA);
	}

	void DrawNote(const short k, const int y, int h, const unsigned int c)
	{
		if (h > 5) --h;
		if (h < 1) h = 1;
		unsigned short r=c&0xFF;
    	unsigned short g=(c&0xFF00)>>8;
	    unsigned short b=(c&0xFF0000)>>16;
		r=r/6;
		g=g/6;
		b=b/6;
		unsigned int l;
		l=0xFF000000|r|g<<8|b<<16;
		int t=(_Width/640);
		//FillRectangle(_KeyX[k] + 1, y, _KeyWidth[k] - 1, h, c);
		Draw3Drect(_KeyX[k] + 1, y+1+t, _KeyWidth[k], h+2, c,1.7);
		DrawNoteRectangle(_KeyX[k],y+1+t,_KeyWidth[k]+1 ,h+2,l,t);
	}
	void DrawNote2(const short k, const int y, int h, const unsigned int c)
	{
		if (h > 5) --h;
		if (h < 1) h = 1;
		unsigned short r=c&0xFF;
    	unsigned short g=(c&0xFF00)>>8;
	    unsigned short b=(c&0xFF0000)>>16;
		r=r/6;
		g=g/6;
		b=b/6;
		unsigned int l;
		l=0xFF000000|r|g<<8|b<<16;
		int t=(_Width/640);
		//FillRectangle(_KeyX[k] + 1, y, _KeyWidth[k] - 1, h, c);
		Draw3Drect(_KeyX[k]+1, y+1+t, _KeyWidth[k]-1, h+2, c,1.7);
		DrawNoteRectangle(_KeyX[k]-t+1,y+1+t,_KeyWidth[k]+(2*t)-1 ,h+2,l,t);
	}
	 
	 
	 void DSJX(const int x1,const int x2,const int y,const int h,const unsigned int c)
	 {
	    int s;
	    if (x1<x2)
	    {
	        s=h/(x2-x1);
	        for(int a=x1,b=0;a!=x2;++a,++b)
	        {
	           for(int j=y;j<y+s*b;++j)
	           {
	              FI(j,a,c);
	              }
	              }
	              }
	    else
	    {
	    s=h/(x1-x2);
	        for(int a=x1,b=0;a!=x2;--a,++b)
	        {
	           for(int j=y;j<y+s*b;++j)
	           {
	              FI(j,a,c);
	              }
	              }
	    }
	              }
	
	void Draw3Drect(const int x, const int y, const int w, const int h, const unsigned int c,double q)
{
    int d;
    int p[w];
    unsigned short r=((c&0xFF));
	unsigned short g=(((c&0xFF00)>>8));
	unsigned short b=(((c&0xFF0000)>>16));
	double n;
	n = pow(q,1.0/(w-1));
//	r=r*2;
//	g=g*2;
//	b=b*2;
    for (int e=0;e!=w;++e)
    {
		r=r/n;
		g=g/n;
		b=b/n;
		unsigned int l;
	    l=0xFF000000|r|g<<8|b<<16;
	    p[e]=l;
	}
	int i;
	{
		for (int i = x,d=0,xend = x + w; i != xend; ++i,++d)
		{
			for (int j = y, yend = y + h; j != yend; ++j)
			{
				FI(j,i,p[d]);
			}
		}
	}
}

	void Draw3Drect2(const int x, const int y, const int w, const int h, const unsigned int c,double q)
{
    int d;
    int p[h];
    unsigned short r=((c&0xFF));
	unsigned short g=(((c&0xFF00)>>8));
	unsigned short b=(((c&0xFF0000)>>16));
	double n;
	n = pow(q,1.0/(h-1));
//	r=r*2;
//	g=g*2;
//	b=b*2;
    for (int e=0;e!=h;++e)
    {
		r=r/n;
		g=g/n;
		b=b/n;
		unsigned int l;
	    l=0xFF000000|r|g<<8|b<<16;
	    p[e]=l;
	}
	int i;
	{
		for (int i = x,xend = x + w; i != xend; ++i)
		{
			for (int j = y,d=0, yend = y + h; j != yend; ++j,++d)
			{
				FI(j,i,p[d]);
			}
		}
	}
}

	void DrawRectangle(const int x, const int y, const int w, const int h, const unsigned int c)
	{
		int i;
		//if (x < _Width)
		for (i = y; i < y + h; ++i)
			FI(i,x,c);
		//if (y < _Height)
		for (i = x; i < x + w; ++i)
			FI(y,i,c);
		//if (w > 1)
		for (i = y; i < y + h; ++i)
			FI(i,x+w-1,c);
		//if (h > 1)
		for (i = x; i < x + w; ++i)
			FI(y+h-1,i,c);
	}
	
   void DrawNoteRectangle(const int x, const int y, const int w, const int h, const unsigned int c , int t)
	{
		int i,j;
		if(t<1)t=1;
		for (j=0;j!=t;++j)
		{
	    DrawRectangle(x+j, y+j,w-2*j, h-2*j, c);
	    }
	}
	void DrawGrayRect(const int x, const int y, const int w, const int h, const unsigned int c , double n)
	{
    unsigned short r=((c&0xFF));
	unsigned short g=(((c&0xFF00)>>8));
	unsigned short b=(((c&0xFF0000)>>16));
    r=r/n;
	g=g/n;
	b=b/n;
	unsigned int l;
	l=0xFF000000|r|g<<8|b<<16;
    FillRectangle(x,y,w,h,l);
	}
	void FillRectangle(const int x, const int y, const int w, const int h, const unsigned int c)
	{
		for (int i = x, xend = x + w; i != xend; ++i)
		{
			for (int j = y, yend = y + h; j != yend; ++j)
			{
				FI(j,i,c);
			}
		}
	}
	
		void FI(const int x, const int y, const unsigned int c)
	{
	   if(x<_Height && y < _Width)
	   _FrameIndexes[x][y] = c;
	}
	
};