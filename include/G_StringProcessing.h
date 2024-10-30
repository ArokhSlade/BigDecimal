#ifndef G_STRING_PROCESSING_H
#define G_STRING_PROCESSING_H

#include "G_Essentials.h"
#include "G_Miscellany_Utility.h"

inline int32 StringLength(char *String) {
	int32 Count = 0;
	while (*String++) ++Count;
	return(Count);
}

inline int32 StringLength(char const *String) {
	int32 Count = 0;
	while (*String++) ++Count;
	return(Count);
}

/* Src exptected to have space for Length + 1 chars */
//NOTE(ArokhSlade##2024 08 14): DEPRECATED
inline void StringCopy(uint32 Length, char *Src, char *Dst) {
  HardAssert(Src != nullptr && Dst != nullptr);
  uint32 Count = 0;
  for (char *Cur=Src ; *Cur != '\0' && Count < Length; ++Count)
  {
    *Dst++ = *Src++;
  }
  *Dst='\0';
};

inline i32 StringCopy(char *Src, char *Dst, i32 Limit = 0) {
    HardAssert(!!Src && !!Dst);
    i32 Count = 0;
    if (!Limit) {
        Limit = StringLength(Src);
    }
    while (Limit-- > 0) {
        *Dst++ = *Src++;
        ++Count;
    }
    *Dst = '\0';
    return Count;
}


void CatStrings( memory_index SourceACount, char *SourceA
                , memory_index SourceBCount, char *SourceB
                , memory_index DestCount, char *Dest);

void CatStrings(char *SourceA, char *SourceB, i32 DestCount, char *Dest);

void PutString(char *String, char *Dest, memory_index MaxLen, bool32 EnsureDestIsNullTerminated);

//TODO(##2023 11 09) const args
bool32 StringCompare(char *First, char *Second);


inline bool32 IsSpace(char Char) {
    const char Space[] = " \r\t\n";
    return Contains(Space,StringLength(Space),Char);
}
inline bool32 IsNonWhitespaceText(char Char) {
    return (Char >= 33 && Char <= 126);
}

inline bool32 IsLetter(char C) {
    return C >= 'a' && C<='z' || C >= 'A' && C <= 'Z';
}

inline bool IsLower(char C) {
    return C >= 'a' && C<='z';
}

inline bool IsUpper(char C) {
    return C >= 'A' && C <= 'Z';
}

inline char ToUpper(char C) {
    return (IsLower(C)) ? C + ('A'-'a') : C;
}

inline char ToLower(char C) {
    return (IsUpper(C)) ? C - ('A'-'a') : C;
}



inline bool32 IsSign(char C) {
    return ArrayContains("+-", C);
}

inline bool32 IsNum(char C){
    return C >= '0' && C <= '9';
}

inline bool32 IsNameChar(char C) {
    return (C == '_' || IsLetter(C) || IsNum(C) );
}

inline bool32 IsHexDigit(char C) {
    return IsNum(C) || C >= 'a' && C <= 'f' || C >= 'A' && C <= 'F';
}


/**
 *  \brief Helper Function for Parser
 */
char *MatchString(char *Text, char *String);


bool32 MatchString(char **Text, char *Name);


/**
 *  \brief  tells whether Char is contained within String
 *  \n      String MUST be ZERO-terminated
**/
inline bool Contains(char *String, char Char) {

    for ( ; *String ; ++String ) {
        if (*String == Char)
            return true;

    }
    return false;
}


/**
 *  \brief  tells whether any of Chars is Contained within String
 *  \n      all arguments MUST be ZERO-terminated string
**/
//TODO(ArokhSlade##2024 08 04):unit test
inline bool ContainsAny(char *String, char *Chars) {

    for ( ; *String ; ++String) {
        for ( char *CurChar = Chars; *CurChar ; ++CurChar ) {
            if (*String = *CurChar) return true;
        }
    }

    return false;
}

#endif //G_STRING_PROCESSING_H
