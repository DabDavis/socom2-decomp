/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 -fno-strict-aliasing -funroll-loops */
/* chinese: the recogniser's second pass for Mandarin -- the context built from the first pass's
 * words with their tones (OVL2 0x4CE5F0-0x4D0BD8, SOCOM 1's object at 0x3404E8: Build_Tree_ForChinese,
 * Translate_Prons_Into_Items_And_Tones, SplitUp_Finals, Swap_Finals,
 * FillOut_Syntax_Statedict_And_Wordprops_ForChinese, FreeSecondPassContext_ForChinese,
 * CreateSecondPassContext_ForChinese, Tone_Of_Final, Calculate_NbrOfStatesInDict_ForChinese,
 * Calculate_NbrOfItemNodes_In_FinalSwappedTranslatedTree; the five before the free and the last
 * two LOCAL there), ScanSoft's speech recogniser, prebuilt with ee-gcc 2.96 at -O2 -G0
 * -fno-strict-aliasing -funroll-loops (README "The GCC front"). The file's name is ours. Only
 * Tone_Of_Final is written; the rest of the object is not (FreeSecondPassContext_ForChinese
 * stands as a matched draft). */

/* a final's tone is valid: nonzero and below the count (SOCOM II's body; SOCOM 1's returned the
 * compare itself after the zero test). Nothing calls it in either build. */
int Tone_Of_Final(unsigned short final, unsigned short n)
{
    if (final != 0 && final < n)
        return 1;
    return 0;
}
