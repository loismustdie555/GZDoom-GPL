// Copyright 2010 Christoph Oelckers
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//
// 1. Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
// 3. The name of the author may not be used to endorse or promote products
//    derived from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
// IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
// OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
// IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
// INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
// NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
// THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// DESCRIPTION:
//      Parser for intermission definitions in MAPINFO (both new style and
//      old style 'ENDGAME' blocks).


#include "intermission/intermission.h"
#include "g_level.h"
#include "w_wad.h"
#include "gi.h"


static void ReplaceIntermission(FName intname,FIntermissionDescriptor *desc)
{
	FIntermissionDescriptor ** pDesc = IntermissionDescriptors.CheckKey(intname);
	if (pDesc != NULL && *pDesc != NULL) delete *pDesc;
	IntermissionDescriptors[intname] = desc;
}

void DeinitIntermissions()
{
	FIntermissionDescriptorList::Iterator it(IntermissionDescriptors);

	FIntermissionDescriptorList::Pair *pair;

	while (it.NextPair(pair))
	{
		delete pair->Value;
		pair->Value = NULL;
	}
	IntermissionDescriptors.Clear();
}

//==========================================================================
//
// FIntermissionAction 
//
//==========================================================================

FIntermissionAction::FIntermissionAction()
{
	mSize = sizeof(FIntermissionAction);
	mClass = RUNTIME_CLASS(DIntermissionScreen);
	mMusicOrder =
	mCdId = 
	mCdTrack = 
	mDuration = 0;
	mFlatfill = false;
	mMusicLooping = true;
}

bool FIntermissionAction::ParseKey(FScanner &sc)
{
	if (sc.Compare("music"))
	{
		sc.MustGetToken('=');
		sc.MustGetToken(TK_StringConst);
		mMusic = sc.String;
		mMusicOrder = 0;
		if (sc.CheckToken(','))
		{
			sc.MustGetToken(TK_IntConst);
			mMusicOrder = sc.Number;
		}
		return true;
	}
	else if (sc.Compare("cdmusic"))
	{
		sc.MustGetToken('=');
		sc.MustGetToken(TK_IntConst);
		mCdTrack = sc.Number;
		mCdId = 0;
		if (sc.CheckToken(','))
		{
			sc.MustGetToken(TK_IntConst);
			mCdId = sc.Number;
		}
		return true;
	}
	else if (sc.Compare("Time"))
	{
		sc.MustGetToken('=');
		if (!sc.CheckToken('-'))
		{
			sc.MustGetFloat();
			mDuration = xs_RoundToInt(sc.Float*TICRATE);
		}
		else
		{
			sc.MustGetToken(TK_IntConst);
			mDuration = sc.Number;
		}
		return true;
	}
	else if (sc.Compare("Background"))
	{
		sc.MustGetToken('=');
		sc.MustGetToken(TK_StringConst);
		mBackground = sc.String;
		mFlatfill = 0;
		if (sc.CheckToken(','))
		{
			sc.MustGetToken(TK_IntConst);
			mFlatfill = !!sc.Number;
			if (sc.CheckToken(','))
			{
				sc.MustGetToken(TK_StringConst);
				mPalette = sc.String;
			}
		}
		return true;
	}
	else if (sc.Compare("Sound"))
	{
		sc.MustGetToken('=');
		sc.MustGetToken(TK_StringConst);
		mSound = sc.String;
		return true;
	}
	else if (sc.Compare("Draw"))
	{
		FIntermissionPatch *pat = &mOverlays[mOverlays.Reserve(1)];
		sc.MustGetToken('=');
		sc.MustGetToken(TK_StringConst);
		pat->mName = sc.String;
		sc.MustGetToken(',');
		sc.MustGetToken(TK_IntConst);
		pat->x = sc.Number;
		sc.MustGetToken(',');
		sc.MustGetToken(TK_IntConst);
		pat->y = sc.Number;
		pat->mCondition = NAME_None;
		return true;
	}
	else if (sc.Compare("DrawConditional"))
	{
		FIntermissionPatch *pat = &mOverlays[mOverlays.Reserve(1)];
		sc.MustGetToken('=');
		sc.MustGetToken(TK_StringConst);
		pat->mCondition = sc.String;
		sc.MustGetToken(',');
		sc.MustGetToken(TK_StringConst);
		pat->mName = sc.String;
		sc.MustGetToken(',');
		sc.MustGetToken(TK_IntConst);
		pat->x = sc.Number;
		sc.MustGetToken(',');
		sc.MustGetToken(TK_IntConst);
		pat->y = sc.Number;
		return true;
	}
	else return false;
}

//==========================================================================
//
// FIntermissionActionFader
//
//==========================================================================

FIntermissionActionFader::FIntermissionActionFader()
{
	mSize = sizeof(FIntermissionActionFader);
	mClass = RUNTIME_CLASS(DIntermissionScreenFader);
	mFadeType = FADE_In;
}

bool FIntermissionActionFader::ParseKey(FScanner &sc)
{
	struct FadeType
	{
		const char *Name;
		EFadeType Type;
	}
	const FT[] = {
		{ "FadeIn", FADE_In },
		{ "FadeOut", FADE_Out },
		{ NULL, FADE_In }
	};

	if (sc.Compare("FadeType"))
	{
		sc.MustGetToken('=');
		sc.MustGetToken(TK_Identifier);
		int v = sc.MatchString(&FT[0].Name, sizeof(FT[0]));
		if (v != -1) mFadeType = FT[v].Type;
		return true;
	}
	else return Super::ParseKey(sc);
}

//==========================================================================
//
// FIntermissionActionWiper
//
//==========================================================================

FIntermissionActionWiper::FIntermissionActionWiper()
{
	mSize = sizeof(FIntermissionActionWiper);
	mClass = WIPER_ID;
	mWipeType = GS_FORCEWIPE;
}

bool FIntermissionActionWiper::ParseKey(FScanner &sc)
{
	struct WipeType
	{
		const char *Name;
		gamestate_t Type;
	}
	const FT[] = {
		{ "Crossfade", GS_FORCEWIPEFADE },
		{ "Melt", GS_FORCEWIPEMELT },
		{ "Burn", GS_FORCEWIPEBURN },
		{ "Default", GS_FORCEWIPE },
		{ NULL, GS_FORCEWIPE }
	};

	if (sc.Compare("WipeType"))
	{
		sc.MustGetToken('=');
		sc.MustGetToken(TK_Identifier);
		int v = sc.MatchString(&FT[0].Name, sizeof(FT[0]));
		if (v != -1) mWipeType = FT[v].Type;
		return true;
	}
	else return Super::ParseKey(sc);
}

//==========================================================================
//
// FIntermissionActionFader
//
//==========================================================================

FIntermissionActionTextscreen::FIntermissionActionTextscreen()
{
	mSize = sizeof(FIntermissionActionTextscreen);
	mClass = RUNTIME_CLASS(DIntermissionScreenText);
	mTextSpeed = 2;
	mTextX = -1;	// use gameinfo defaults
	mTextY = -1;
	mTextColor = CR_UNTRANSLATED;
	mTextDelay = 10;
}

bool FIntermissionActionTextscreen::ParseKey(FScanner &sc)
{
	if (sc.Compare("Position"))
	{
		sc.MustGetToken('=');
		sc.MustGetToken(TK_IntConst);
		mTextX = sc.Number;
		sc.MustGetToken(',');
		sc.MustGetToken(TK_IntConst);
		mTextY = sc.Number;
		return true;
	}
	else if (sc.Compare("TextLump"))
	{
		sc.MustGetToken('=');
		sc.MustGetToken(TK_StringConst);
		int lump = Wads.CheckNumForFullName(sc.String, true);
		if (lump > 0)
		{
			mText = Wads.ReadLump(lump).GetString();
		}
		else
		{
			// only print an error if coming from a PWAD
			if (Wads.GetLumpFile(sc.LumpNum) > 1)
				sc.ScriptMessage("Unknown text lump '%s'", sc.String);
			mText.Format("Unknown text lump '%s'", sc.String);
		}
		return true;
	}
	else if (sc.Compare("Text"))
	{
		sc.MustGetToken('=');
		do
		{
			sc.MustGetToken(TK_StringConst);
			mText << sc.String << '\n';
		}
		while (sc.CheckToken(','));
		return true;
	}
	else if (sc.Compare("TextColor"))
	{
		sc.MustGetToken('=');
		sc.MustGetToken(TK_StringConst);
		mTextColor = V_FindFontColor(sc.String);
		return true;
	}
	else if (sc.Compare("TextDelay"))
	{
		sc.MustGetToken('=');
		if (!sc.CheckToken('-'))
		{
			sc.MustGetFloat();
			mTextDelay = xs_RoundToInt(sc.Float*TICRATE);
		}
		else
		{
			sc.MustGetToken(TK_IntConst);
			mTextDelay = sc.Number;
		}
		return true;
	}
	else if (sc.Compare("textspeed"))
	{
		sc.MustGetToken('=');
		sc.MustGetToken(TK_IntConst);
		mTextSpeed = sc.Number;
		return true;
	}
	else return Super::ParseKey(sc);
}

//==========================================================================
//
// FIntermissionAction
//
//==========================================================================

FIntermissionActionCast::FIntermissionActionCast()
{
	mSize = sizeof(FIntermissionActionCast);
	mClass = RUNTIME_CLASS(DIntermissionScreenCast);
}

bool FIntermissionActionCast::ParseKey(FScanner &sc)
{
	if (sc.Compare("CastName"))
	{
		sc.MustGetToken('=');
		sc.MustGetToken(TK_StringConst);
		mName = sc.String;
		return true;
	}
	else if (sc.Compare("CastClass"))
	{
		sc.MustGetToken('=');
		sc.MustGetToken(TK_StringConst);
		mCastClass = sc.String;
		return true;
	}
	else if (sc.Compare("AttackSound"))
	{
		static const char *const seqs[] = {"Missile", "Melee", NULL};
		FCastSound *cs = &mCastSounds[mCastSounds.Reserve(1)];
		sc.MustGetToken('=');
		sc.MustGetToken(TK_StringConst);
		cs->mSequence = (BYTE)sc.MatchString(seqs);
		sc.MustGetToken(',');
		sc.MustGetToken(TK_IntConst);
		cs->mIndex = (BYTE)sc.Number;
		sc.MustGetToken(',');
		sc.MustGetToken(TK_StringConst);
		cs->mSound = sc.String;
		return true;
	}
	else return Super::ParseKey(sc);
}

//==========================================================================
//
// FIntermissionActionScroller
//
//==========================================================================

FIntermissionActionScroller::FIntermissionActionScroller()
{
	mSize = sizeof(FIntermissionActionScroller);
	mClass = RUNTIME_CLASS(DIntermissionScreenScroller);
	mScrollDelay = 0;
	mScrollTime = 640;
	mScrollDir = SCROLL_Right;
}

bool FIntermissionActionScroller::ParseKey(FScanner &sc)
{
	struct ScrollType
	{
		const char *Name;
		EScrollDir Type;
	}
	const ST[] = {
		{ "Left", SCROLL_Left },
		{ "Right", SCROLL_Right },
		{ "Up", SCROLL_Up },
		{ "Down", SCROLL_Down },
		{ NULL, SCROLL_Left }
	};

	if (sc.Compare("ScrollDirection"))
	{
		sc.MustGetToken('=');
		sc.MustGetToken(TK_Identifier);
		int v = sc.MatchString(&ST[0].Name, sizeof(ST[0]));
		if (v != -1) mScrollDir = ST[v].Type;
		return true;
	}
	else if (sc.Compare("InitialDelay"))
	{
		sc.MustGetToken('=');
		if (!sc.CheckToken('-'))
		{
			sc.MustGetFloat();
			mScrollDelay = xs_RoundToInt(sc.Float*TICRATE);
		}
		else
		{
			sc.MustGetToken(TK_IntConst);
			mScrollDelay = sc.Number;
		}
		return true;
	}
	else if (sc.Compare("ScrollTime"))
	{
		sc.MustGetToken('=');
		if (!sc.CheckToken('-'))
		{
			sc.MustGetFloat();
			mScrollTime = xs_RoundToInt(sc.Float*TICRATE);
		}
		else
		{
			sc.MustGetToken(TK_IntConst);
			mScrollTime = sc.Number;
		}
		return true;
	}
	else if (sc.Compare("Background2"))
	{
		sc.MustGetToken('=');
		sc.MustGetToken(TK_StringConst);
		mSecondPic = sc.String;
		return true;
	}
	else return Super::ParseKey(sc);
}

//==========================================================================
//
// ParseIntermission
//
//==========================================================================

void FMapInfoParser::ParseIntermissionAction(FIntermissionDescriptor *desc)
{
	FIntermissionAction *ac = NULL;

	sc.MustGetToken(TK_Identifier);
	if (sc.Compare("image"))
	{
		ac = new FIntermissionAction;
	}
	else if (sc.Compare("scroller"))
	{
		ac = new FIntermissionActionScroller;
	}
	else if (sc.Compare("cast"))
	{
		ac = new FIntermissionActionCast;
	}
	else if (sc.Compare("Fader"))
	{
		ac = new FIntermissionActionFader;
	}
	else if (sc.Compare("Wiper"))
	{
		ac = new FIntermissionActionWiper;
	}
	else if (sc.Compare("TextScreen"))
	{
		ac = new FIntermissionActionTextscreen;
	}
	else if (sc.Compare("GotoTitle"))
	{
		ac = new FIntermissionAction;
		ac->mClass = TITLE_ID;
	}
	else if (sc.Compare("Link"))
	{
		sc.MustGetToken('=');
		sc.MustGetToken(TK_Identifier);
		desc->mLink = sc.String;
		return;
	}
	else
	{
		sc.ScriptMessage("Unknown intermission type '%s'", sc.String);
	}

	sc.MustGetToken('{');
	while (!sc.CheckToken('}'))
	{
		bool success = false;
		if (!sc.CheckToken(TK_Sound))
		{
			sc.MustGetToken(TK_Identifier);
		}
		if (ac != NULL)
		{
			success = ac->ParseKey(sc);
			if (!success)
			{
				sc.ScriptMessage("Unknown key name '%s'\n", sc.String);
			}
		}
		if (!success) SkipToNext();
	}
	if (ac != NULL) desc->mActions.Push(ac);
}

//==========================================================================
//
// ParseIntermission
//
//==========================================================================

void FMapInfoParser::ParseIntermission()
{
	sc.MustGetString();
	FName intname = sc.String;
	FIntermissionDescriptor *desc = new FIntermissionDescriptor();

	ReplaceIntermission(intname, desc);
	sc.MustGetToken('{');
	while (!sc.CheckToken('}'))
	{
		ParseIntermissionAction(desc);
	}
}

//==========================================================================
//
// Parse old style endsequence
//
//==========================================================================

struct EndSequence
{
	SBYTE EndType;
	bool MusicLooping;
	bool PlayTheEnd;
	FString PicName;
	FString PicName2;
	FString Music;
};

enum EndTypes
{
	END_Pic,
	END_Bunny,
	END_Cast,
	END_Demon
};

FName FMapInfoParser::ParseEndGame()
{
	EndSequence newSeq;
	static int generated = 0;

	newSeq.EndType = -1;
	newSeq.PlayTheEnd = false;
	newSeq.MusicLooping = true;
	while (!sc.CheckString("}"))
	{
		sc.MustGetString();
		if (sc.Compare("pic"))
		{
			ParseAssign();
			sc.MustGetString();
			newSeq.EndType = END_Pic;
			newSeq.PicName = sc.String;
		}
		else if (sc.Compare("hscroll"))
		{
			ParseAssign();
			newSeq.EndType = END_Bunny;
			sc.MustGetString();
			newSeq.PicName = sc.String;
			ParseComma();
			sc.MustGetString();
			newSeq.PicName2 = sc.String;
			if (CheckNumber())
				newSeq.PlayTheEnd = !!sc.Number;
		}
		else if (sc.Compare("vscroll"))
		{
			ParseAssign();
			newSeq.EndType = END_Demon;
			sc.MustGetString();
			newSeq.PicName = sc.String;
			ParseComma();
			sc.MustGetString();
			newSeq.PicName2 = sc.String;
		}
		else if (sc.Compare("cast"))
		{
			newSeq.EndType = END_Cast;
			if (newSeq.PicName.IsEmpty()) newSeq.PicName = "$bgcastcall";
		}
		else if (sc.Compare("music"))
		{
			ParseAssign();
			sc.MustGetString();
			newSeq.Music = sc.String;
			if (CheckNumber())
			{
				newSeq.MusicLooping = !!sc.Number;
			}
		}
		else
		{
			if (format_type == FMT_New)
			{
				// Unknown
				sc.ScriptMessage("Unknown property '%s' found in endgame definition\n", sc.String);
				SkipToNext();
			}
			else
			{
				sc.ScriptError("Unknown property '%s' found in endgame definition\n", sc.String);
			}

		}
	}
	FIntermissionDescriptor *desc = new FIntermissionDescriptor;
	FIntermissionAction *action = NULL;

	switch (newSeq.EndType)
	{
	case END_Pic:
		action = new FIntermissionAction;
		break;

	case END_Bunny:
	{
		FIntermissionActionScroller *bunny = new FIntermissionActionScroller;
		bunny->mSecondPic = newSeq.PicName2;
		bunny->mScrollDir = SCROLL_Left;
		bunny->mScrollDelay = 230;
		bunny->mScrollTime = 640;
		bunny->mDuration = 1130;
		action = bunny;
		if (newSeq.PlayTheEnd) desc->mLink = "TheEnd";
		break;
	}

	case END_Demon:
	{
		FIntermissionActionScroller *demon = new FIntermissionActionScroller;
		demon->mSecondPic = newSeq.PicName2;
		demon->mScrollDir = SCROLL_Up;
		demon->mScrollDelay = 70;
		demon->mScrollTime = 600;
		action = demon;
		break;
	}

	case END_Cast:
		action = new FIntermissionAction;
		action->mDuration = 1;
		desc->mLink = "Doom2Cast";
		break;
	}

	if (action == NULL)
	{
		sc.ScriptError("Endgame type was not defined");
		return NAME_None;	// We won't really get here.
	}
	else
	{
		action->mBackground = newSeq.PicName;
		action->mMusic = newSeq.Music;
		action->mMusicLooping = newSeq.MusicLooping;
		desc->mActions.Push(action);

		FString seq;
		seq.Format("@EndSequence_%d_", generated++);
		ReplaceIntermission(seq, desc);
		return FName(seq);
	}
}

//==========================================================================
//
// Checks map name for end sequence
//
//==========================================================================

FName FMapInfoParser::CheckEndSequence()
{
	const char *seqname = NULL;

	if (sc.Compare("endgame"))
	{
		if (!sc.CheckString("{"))
		{
			// Make Demon Eclipse work again
			sc.UnGet();
			goto standard_endgame;
		}
		return ParseEndGame();
	}
	else if (strnicmp (sc.String, "EndGame", 7) == 0)
	{
		switch (sc.String[7])
		{
		case '1':	seqname = "Inter_Pic1";		break;
		case '2':	seqname = "Inter_Pic2";		break;
		case '3':	seqname = "Inter_Bunny";	break;
		case 'C':	seqname = "Inter_Cast";		break;
		case 'W':	seqname = "Inter_Underwater";	break;
		case 'S':	seqname = "Inter_Strife";	break;
	standard_endgame:
		default:	seqname = "Inter_Pic3";		break;
		}
	}
	else if (sc.Compare("endpic"))
	{
		ParseComma();
		sc.MustGetString ();
		FString seqname;
		seqname << "@EndPic_" << sc.String;
		FIntermissionDescriptor *desc = new FIntermissionDescriptor;
		FIntermissionAction *action = new FIntermissionAction;
		action->mBackground = sc.String;
		desc->mActions.Push(action);
		ReplaceIntermission(seqname, desc);
		return FName(seqname);
	}
	else if (sc.Compare("endbunny"))
	{
		seqname = "Inter_Bunny";
	}
	else if (sc.Compare("endcast"))
	{
		seqname = "Inter_Cast";
	}
	else if (sc.Compare("enddemon"))
	{
		seqname = "Inter_Demonscroll";
	}
	else if (sc.Compare("endchess"))
	{
		seqname = "Inter_Chess";
	}
	else if (sc.Compare("endunderwater"))
	{
		seqname = "Inter_Underwater";
	}
	else if (sc.Compare("endbuystrife"))
	{
		seqname = "Inter_BuyStrife";
	}
	else if (sc.Compare("endtitle"))
	{
		seqname = "Inter_Titlescreen";
	}
	else if (sc.Compare("endsequence"))
	{
		ParseComma();
		sc.MustGetString();
		seqname = sc.String;
	}

	if (seqname != NULL)
	{
		return FName(seqname);
	}
	return NAME_None;
}


//==========================================================================
//
// Creates an intermission from the cluster's finale info
//
//==========================================================================

void F_StartFinale (const char *music, int musicorder, int cdtrack, unsigned int cdid, const char *flat, 
					const char *text, INTBOOL textInLump, INTBOOL finalePic, INTBOOL lookupText, 
					bool ending, FName endsequence)
{
	// Hexen's chess ending doesn't have a text screen, even if the cluster has a message defined.
	if (text != NULL && *text != 0 && endsequence != NAME_Inter_Chess)
	{
		FIntermissionActionTextscreen *textscreen = new FIntermissionActionTextscreen;
		if (textInLump)
		{
			int lump = Wads.CheckNumForFullName(text, true);
			if (lump > 0)
			{
				textscreen->mText = Wads.ReadLump(lump).GetString();
			}
			else
			{
				textscreen->mText.Format("Unknown text lump '%s'", text);
			}
		}
		else if (!lookupText)
		{
			textscreen->mText = text;
		}
		else
		{
			textscreen->mText << '$' << text;
		}
		textscreen->mTextDelay = 10;
		if (flat != NULL && *flat != 0)
		{
			textscreen->mBackground = flat;
		}
		else
		{
			// force a black screen if no texture is set.
			textscreen->mBackground = "-";
		}
		textscreen->mFlatfill = !finalePic;

		if (music != NULL && *music != 0) 
		{
			textscreen->mMusic = music;
			textscreen->mMusicOrder = musicorder;
		}
		if (cdtrack > 0)
		{
			textscreen->mCdTrack = cdtrack;
			textscreen->mCdId = cdid;
		}
		FIntermissionDescriptor *desc = new FIntermissionDescriptor;
		desc->mActions.Push(textscreen);

		if (ending)
		{
			desc->mLink = endsequence;
			FIntermissionActionWiper *wiper = new FIntermissionActionWiper;
			desc->mActions.Push(wiper);
		}

		F_StartIntermission(desc, true, ending? FSTATE_EndingGame : FSTATE_ChangingLevel);
	}
	else if (ending)
	{
		FIntermissionDescriptor **pdesc = IntermissionDescriptors.CheckKey(endsequence);
		if (pdesc != NULL)
		{
			F_StartIntermission(*pdesc, false, ending? FSTATE_EndingGame : FSTATE_ChangingLevel);
		}
	}
}
