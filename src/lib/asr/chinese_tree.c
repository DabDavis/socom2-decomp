/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 -fno-strict-aliasing -funroll-loops */
/* chinese_tree: the Mandarin second pass over a tree context, chinese.c's twin (OVL2
 * 0x4D5B18-0x4D8398, SOCOM 1's object at 0x345CD0: Build_Tree_ForChinese_TreeCtx,
 * Translate_Prons_Into_Items_And_Tones_TreeCtx, SplitUp_Finals_TreeCtx, Swap_Finals_TreeCtx,
 * FillOut_Syntax_Statedict_And_Wordprops_ForChinese_TreeCtx, CreateSecondPassTreeContext_ForChinese,
 * Tone_Of_Final_TreeCtx, Calculate_NbrOfStatesInDict_ForChinese_TreeCtx,
 * Calculate_NbrOfItemNodes_In_FinalSwappedTranslatedTree_TreeCtx,
 * FreeSecondPassContext_ForChinese_TreeCtx), ScanSoft's speech recogniser, prebuilt with ee-gcc
 * 2.96 at -O2 -G0 -fno-strict-aliasing -funroll-loops (README "The GCC front"). The file's name is
 * ours. Only Tone_Of_Final_TreeCtx is written. */

/* chinese.c's Tone_Of_Final again, under this object's name; nothing calls it */
int Tone_Of_Final_TreeCtx(unsigned short final, unsigned short n)
{
    if (final != 0 && final < n)
        return 1;
    return 0;
}
